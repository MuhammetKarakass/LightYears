#include "gameplay/content/AbilityLoader.h"

#include "framework/JsonDocumentLoader.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/content/NumericSettingContractRegistry.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/content/GameAbilitySettingContracts.h"

#include <limits>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>

namespace ly::content
{
	namespace
	{
		using Json = JsonDocumentLoader::Json;

		std::string RequiredString(const Json& object, const char* fieldName)
		{
			return object.at(fieldName).get<std::string>();
		}

		sas::AttributeModifierOperation ParseOperation(const std::string& value)
		{
			if (value == "Add")
			{
				return sas::AttributeModifierOperation::Add;
			}
			if (value == "Multiply")
			{
				return sas::AttributeModifierOperation::Multiply;
			}
			if (value == "Override")
			{
				return sas::AttributeModifierOperation::Override;
			}
			throw std::runtime_error("Unknown ability modifier operation: " + value);
		}

		sas::AttributeModifier ParseModifier(const Json& object)
		{
			return sas::AttributeModifier{
				GameplayTag{ RequiredString(object, "attributeId") },
				ParseOperation(RequiredString(object, "operation")),
				object.at("magnitude").get<float>(),
				object.value("priority", 0)
			};
		}

		sas::GameplayAttribute ParseAttribute(const Json& object)
		{
			return sas::GameplayAttribute{
				GameplayTag{ RequiredString(object, "id") },
				object.at("baseValue").get<float>(),
				object.value("minValue", 0.f),
				object.value("maxValue", std::numeric_limits<float>::max())
			};
		}

		List<sas::AttributeModifier> ParseModifiers(const Json& values)
		{
			List<sas::AttributeModifier> modifiers;
			for (const Json& value : values)
			{
				modifiers.push_back(ParseModifier(value));
			}
			return modifiers;
		}

		List<sas::AttributeScalingRule> ParseScalingRules(const Json& values)
		{
			List<sas::AttributeScalingRule> rules;
			for (const Json& value : values)
			{
				rules.push_back(sas::AttributeScalingRule{
					GameplayTag{ value.at("targetAttributeId").get<std::string>() },
					GameplayTag{ value.at("sourceAttributeId").get<std::string>() },
					ParseOperation(value.at("operation").get<std::string>()),
					value.at("coefficient").get<float>()
				});
			}
			return rules;
		}

		List<AbilityEffectSpecDefinition> ParseEffectSpecs(const Json& values)
		{
			List<AbilityEffectSpecDefinition> specs;
			std::set<std::string> effectIds;
			for (const Json& value : values)
			{
				AbilityEffectSpecDefinition spec;
				spec.effectId = RequiredString(value, "effectId");
				std::string idFailure;
				if (!ContentIdSchema::ValidateEffectId(spec.effectId, &idFailure))
				{
					throw std::runtime_error(
						"Ability effect spec has invalid effect ID '" + spec.effectId + "': " + idFailure
					);
				}
				if (!effectIds.insert(spec.effectId).second)
				{
					throw std::runtime_error(
						"Duplicate ability effect spec for '" + spec.effectId + "'"
					);
				}
				spec.useAbilityDuration = value.value("useAbilityDuration", false);
				if (value.contains("duration"))
				{
					spec.duration = value.at("duration").get<float>();
				}
				if (spec.useAbilityDuration && spec.duration.has_value())
				{
					throw std::runtime_error(
						"Ability effect spec '" + spec.effectId +
						"' cannot declare both duration and useAbilityDuration"
					);
				}
				if (value.contains("maxStacks"))
				{
					spec.maxStacks = value.at("maxStacks").get<int>();
				}
				spec.modifiers = ParseModifiers(value.value("modifiers", Json::array()));
				for (const Json& attribute : value.value("attributes", Json::array()))
				{
					spec.attributes.push_back(ParseAttribute(attribute));
				}
				specs.push_back(std::move(spec));
			}
			return specs;
		}

		List<AbilityLevelStep> ParseLevelProgression(const Json& progression)
		{
			List<AbilityLevelStep> levels;
			if (progression.contains("repeat"))
			{
				const Json& repeated = progression.at("repeat");
				const List<sas::AttributeModifier> repeatedModifiers = ParseModifiers(
					repeated.value("attributeModifiers", Json::array())
				);
				const std::size_t repeatCount = repeated.at("count").get<std::size_t>();
				for (std::size_t index = 0; index < repeatCount; ++index)
				{
					AbilityLevelStep step;
					step.attributeModifiers = repeatedModifiers;
					levels.push_back(std::move(step));
				}
			}
			for (const Json& level : progression.value("levels", Json::array()))
			{
				AbilityLevelStep step;
				step.attributeModifiers = ParseModifiers(
					level.value("attributeModifiers", Json::array())
				);
				for (const Json& upgradeId : level.value("unlockedUpgradeIds", Json::array()))
				{
					step.unlockedUpgradeIds.emplace_back(GameplayTag{ upgradeId.get<std::string>() });
				}
				levels.push_back(std::move(step));
			}
			return levels;
		}

		const GameAbilityDefinition* FindFallback(
			const List<const GameAbilityDefinition*>& fallbackDefinitions,
			const std::string& abilityId
		)
		{
			for (const GameAbilityDefinition* definition : fallbackDefinitions)
			{
				if (definition && definition->abilityId == abilityId)
				{
					return definition;
				}
			}
			return nullptr;
		}

		const AbilityActorDefinition* FindActorFallback(
			const List<const AbilityActorDefinition*>& fallbackActorDefinitions,
			const std::string& actorDefinitionId
		)
		{
			for (const AbilityActorDefinition* definition : fallbackActorDefinitions)
			{
				if (definition && definition->actorDefinitionId == actorDefinitionId)
				{
					return definition;
				}
			}
			return nullptr;
		}

		Json MergeJsonObjects(Json base, const Json& overrides)
		{
			if (!base.is_object() || !overrides.is_object())
			{
				return overrides;
			}

			for (const auto& [name, value] : overrides.items())
			{
				if (base.contains(name) && base.at(name).is_object() && value.is_object())
				{
					base[name] = MergeJsonObjects(base.at(name), value);
				}
				else
				{
					base[name] = value;
				}
			}
			return base;
		}

		AbilityActorDefinition ParseActor(
			const Json& object,
			const List<const AbilityActorDefinition*>& fallbackActorDefinitions,
			const std::map<std::string, sas::GameplayAttributeList>& attributeProfiles
		)
		{
			const std::string actorDefinitionId = RequiredString(object, "id");
			std::string actorIdFailure;
			if (!ContentIdSchema::ValidateAbilityActorDefinitionId(
				actorDefinitionId,
				&actorIdFailure
			))
			{
				throw std::runtime_error(actorIdFailure);
			}
			if (!object.contains("spawnDistance") || !object.at("spawnDistance").is_number())
			{
				throw std::runtime_error(
					"Ability actor '" + actorDefinitionId +
					"' requires numeric JSON field 'spawnDistance'"
				);
			}
			if (object.contains("lifeTime") && !object.at("lifeTime").is_number())
			{
				throw std::runtime_error(
					"Ability actor '" + actorDefinitionId +
					"' JSON field 'lifeTime' must be numeric"
				);
			}
			const std::string fallbackActorDefinitionId = object.value(
				"baseId",
				actorDefinitionId
			);
			const AbilityActorDefinition* fallback = FindActorFallback(
				fallbackActorDefinitions,
				fallbackActorDefinitionId
			);
			if (!fallback)
			{
				throw std::runtime_error(
					"No C++ presentation base exists for ability actor '" +
					actorDefinitionId + "'"
				);
			}

			AbilityActorDefinition definition = *fallback;
			definition.actorDefinitionId = actorDefinitionId;
			// The C++ record supplies the actor type/presentation skeleton only.
			// Runtime numeric values must come from the authoritative JSON record.
			definition.lifeTime = object.value("lifeTime", 0.f);
			definition.spawnDistance = object.value("spawnDistance", 0.f);
			definition.attributes.clear();
			std::set<GameplayTag> attributeIds;
			for (const Json& profileIdValue : object.value("attributeProfileIds", Json::array()))
			{
				const std::string profileId = profileIdValue.get<std::string>();
				std::string profileIdFailure;
				if (!ContentIdSchema::ValidateAbilityAttributeProfileId(
					profileId,
					&profileIdFailure
				))
				{
					throw std::runtime_error(profileIdFailure);
				}
				const auto profile = attributeProfiles.find(profileId);
				if (profile == attributeProfiles.end())
				{
					throw std::runtime_error(
						"Ability actor '" + actorDefinitionId +
						"' references missing attribute profile '" + profileId + "'"
					);
				}
				for (const sas::GameplayAttribute& attribute : profile->second)
				{
					if (!attributeIds.insert(attribute.id).second)
					{
						throw std::runtime_error("Duplicate inherited actor attribute '" + attribute.id.ToString() + "'");
					}
					definition.attributes.push_back(attribute);
				}
			}
			for (const Json& attribute : object.value("attributes", Json::array()))
			{
				sas::GameplayAttribute parsed = ParseAttribute(attribute);
				if (!attributeIds.insert(parsed.id).second)
				{
					throw std::runtime_error("Duplicate actor attribute '" + parsed.id.ToString() + "'");
				}
				definition.attributes.push_back(std::move(parsed));
			}
			if (!object.contains("lifeTime"))
			{
				if (const sas::GameplayAttribute* duration =
						sas::FindGameplayAttribute(definition.attributes, CommonAttributeIds::Duration))
				{
					definition.lifeTime = duration->baseValue;
				}
				else
				{
					throw std::runtime_error(
						"Ability actor '" + actorDefinitionId +
						"' requires 'lifeTime' or an " + CommonAttributeIds::Duration.ToString() +
						" value in JSON"
					);
				}
			}
			return definition;
		}

		AbilityLoader::LoadedDefinition ParseAbility(
			const Json& object,
			const List<const GameAbilityDefinition*>& fallbackDefinitions,
			const List<const AbilityActorDefinition*>& fallbackActorDefinitions,
			const std::string& fallbackAbilityId
		)
		{
			AbilityLoader::LoadedDefinition loaded;
			loaded.id = RequiredString(object, "id");
			const GameAbilityDefinition* fallback = FindFallback(
				fallbackDefinitions,
				fallbackAbilityId
			);
			if (!fallback)
			{
				throw std::runtime_error(
					"No C++ behavior base exists for ability '" +
					fallbackAbilityId + "'"
				);
			}

			loaded.definition = *fallback;
			loaded.definition.abilityId = loaded.id;
			for (const char* requiredField : { "cooldown", "duration", "maxCharges" })
			{
				if (!object.contains(requiredField) || !object.at(requiredField).is_number())
				{
					throw std::runtime_error(
						"Ability '" + loaded.id +
						"' requires numeric JSON field '" + requiredField + "'"
					);
				}
			}
			if (!object.contains("settings") || !object.at("settings").is_object())
			{
				throw std::runtime_error(
					"Ability '" + loaded.id + "' requires a JSON object named 'settings'"
				);
			}
			loaded.definition.cooldown = object.at("cooldown").get<float>();
			// Do not inherit numeric balance from the C++ fallback definition.
			// BaseId materialization already supplies inherited values from JSON.
			loaded.definition.duration = object.value("duration", 0.f);
			loaded.definition.maxCharges = object.value("maxCharges", 0);
			loaded.definition.scalingRules.clear();
			loaded.definition.levelProgression.clear();
			loaded.definition.levelUpgradeScrapCosts.clear();
			loaded.definition.effectSpecs.clear();
			if (object.contains("scalingRules"))
			{
				loaded.definition.scalingRules = ParseScalingRules(object.at("scalingRules"));
			}
			if (object.contains("effectSpecs"))
			{
				loaded.definition.effectSpecs = ParseEffectSpecs(object.at("effectSpecs"));
			}

			if (object.contains("progression"))
			{
				const Json& progression = object.at("progression");
				loaded.definition.levelProgression = ParseLevelProgression(progression);
				loaded.definition.levelUpgradeScrapCosts = progression.value(
					"levelUpgradeScrapCosts",
					List<unsigned int>{}
				);
			}

			const Json settings = object.value("settings", Json::object());
			const NumericSettingContract& settingsContract =
				NumericSettingContractRegistry::Find(fallback->behaviorTag);
			for (const auto& [name, value] : settings.items())
			{
				if (settingsContract.allowed.find(name) == settingsContract.allowed.end())
				{
					throw std::runtime_error(
						"Unknown numeric setting '" + name +
						"' for ability '" + loaded.id + "'"
					);
				}
				if (!value.is_number())
				{
					throw std::runtime_error("Ability numeric setting '" + name + "' must be a number");
				}
				loaded.numericSettings.emplace(name, value.get<float>());
			}
			for (const std::string& required : settingsContract.required)
			{
				if (loaded.numericSettings.find(required) == loaded.numericSettings.end())
				{
					throw std::runtime_error(
						"Missing required numeric setting '" + required +
						"' for ability '" + loaded.id + "'"
					);
				}
			}

			std::map<std::string, sas::GameplayAttributeList> attributeProfiles;
			for (const Json& profile : object.value("attributeProfiles", Json::array()))
			{
				const std::string profileId = RequiredString(profile, "id");
				std::string profileIdFailure;
				if (!ContentIdSchema::ValidateAbilityAttributeProfileId(
					profileId,
					&profileIdFailure
				))
				{
					throw std::runtime_error(profileIdFailure);
				}
				sas::GameplayAttributeList attributes;
				std::set<GameplayTag> attributeIds;
				for (const Json& attribute : profile.value("attributes", Json::array()))
				{
					sas::GameplayAttribute parsed = ParseAttribute(attribute);
					if (!attributeIds.insert(parsed.id).second)
					{
						throw std::runtime_error("Duplicate attribute profile value '" + parsed.id.ToString() + "'");
					}
					attributes.push_back(std::move(parsed));
				}
				if (!attributeProfiles.emplace(profileId, std::move(attributes)).second)
				{
					throw std::runtime_error("Duplicate ability attribute profile ID: " + profileId);
				}
			}

			for (const Json& actor : object.value("actors", Json::array()))
			{
				loaded.actorDefinitions.push_back(ParseActor(
					actor,
					fallbackActorDefinitions,
					attributeProfiles
				));
			}

			return loaded;
		}
	}

	AbilityLoader::Result AbilityLoader::LoadFromFile(
		const std::filesystem::path& filePath,
		const List<const GameAbilityDefinition*>& fallbackDefinitions,
		const List<const AbilityActorDefinition*>& fallbackActorDefinitions
	)
	{
		const JsonDocumentLoader::Result documentResult =
			JsonDocumentLoader::LoadFromFile(filePath);
		if (!documentResult.Succeeded())
		{
			return Result{ {}, documentResult.error };
		}

		try
		{
			std::string contractFailure;
			if (!RegisterGameAbilitySettingContracts(&contractFailure))
			{
				return Result{ {}, contractFailure };
			}

			const Json& root = *documentResult.document;
			if (root.at("schemaVersion").get<int>() != 1)
			{
				return Result{ {}, "Unsupported ability schema version" };
			}

			Result result;
			std::map<std::string, const Json*> sourceAbilities;
			std::set<std::string> abilityIds;
			std::set<std::string> actorIds;
			for (const Json& ability : root.at("abilities"))
			{
				const std::string abilityId = RequiredString(ability, "id");
				std::string idFailure;
				if (!ContentIdSchema::ValidateAbilityId(abilityId, &idFailure))
				{
					throw std::runtime_error(
						"Invalid ability ID '" + abilityId + "': " + idFailure
					);
				}
				if (ability.contains("baseId"))
				{
					const std::string baseId = ability.at("baseId").get<std::string>();
					if (!ContentIdSchema::ValidateAbilityId(baseId, &idFailure))
					{
						throw std::runtime_error(
							"Invalid ability baseId '" + baseId + "': " + idFailure
						);
					}
				}
				if (!abilityIds.insert(abilityId).second)
				{
					throw std::runtime_error("Duplicate ability ID: " + abilityId);
				}
				sourceAbilities.emplace(abilityId, &ability);
			}

			std::map<std::string, Json> materializedAbilities;
			std::map<std::string, std::string> cppBaseIds;
			std::set<std::string> resolvingAbilities;
			std::function<void(const std::string&)> materializeAbility;
			materializeAbility = [&](const std::string& abilityId)
			{
				if (materializedAbilities.find(abilityId) != materializedAbilities.end())
				{
					return;
				}
				if (!resolvingAbilities.insert(abilityId).second)
				{
					throw std::runtime_error(
						"Cyclic ability baseId reference involving '" + abilityId + "'"
					);
				}

				const auto source = sourceAbilities.find(abilityId);
				if (source == sourceAbilities.end())
				{
					throw std::runtime_error(
						"Ability baseId references missing ability '" + abilityId + "'"
					);
				}

				const Json& object = *source->second;
				const std::string baseId = object.value("baseId", abilityId);
				if (baseId == abilityId)
				{
					materializedAbilities.emplace(abilityId, object);
					cppBaseIds.emplace(abilityId, abilityId);
				}
				else
				{
					materializeAbility(baseId);
					const Json& baseObject = materializedAbilities.at(baseId);
					Json materialized = MergeJsonObjects(baseObject, object);
					// Actor definitions are global runtime records. A value-only
					// variant reuses the base ability's actors; inheriting them into
					// the variant would create duplicate global actor IDs.
					if (!object.contains("actors"))
					{
						materialized.erase("actors");
					}
					materialized["id"] = abilityId;
					materialized.erase("baseId");
					materializedAbilities.emplace(abilityId, std::move(materialized));
					cppBaseIds.emplace(abilityId, cppBaseIds.at(baseId));
				}
				resolvingAbilities.erase(abilityId);
			};

			for (const Json& ability : root.at("abilities"))
			{
				const std::string abilityId = RequiredString(ability, "id");
				materializeAbility(abilityId);
				LoadedDefinition definition = ParseAbility(
					materializedAbilities.at(abilityId),
					fallbackDefinitions,
					fallbackActorDefinitions,
					cppBaseIds.at(abilityId)
				);
				for (const AbilityActorDefinition& actor : definition.actorDefinitions)
				{
					if (!actorIds.insert(actor.actorDefinitionId).second)
					{
						throw std::runtime_error(
							"Duplicate ability actor ID: " +
							actor.actorDefinitionId
						);
					}
				}
				result.definitions.push_back(std::move(definition));
			}
			if (result.definitions.empty())
			{
				return Result{ {}, "Ability catalog is empty" };
			}
			return result;
		}
		catch (const std::exception& exception)
		{
			return Result{
				{},
				"Failed to load abilities from '" + filePath.string() + "': " + exception.what()
			};
		}
	}
}
