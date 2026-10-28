#include "gameplay/content/AbilityLoader.h"
#include "AbilityDefinitionJsonParser.h"

#include "framework/JsonDocumentLoader.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/content/GameAbilitySettingContracts.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/ability/content/GameAbilityContentRegistration.h"
#include "gameplay/ability/validation/GameAbilityDefinitionValidator.h"
#include "gameplay/ability/validation/GameplayEffectDefinitionValidator.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"

#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <algorithm>

namespace vega::content
{
	namespace
	{
		using Json = JsonDocumentLoader::Json;

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

		void AttachConfiguration(AbilityLoader::LoadedDefinition& loaded)
		{
			auto configuration = std::make_shared<AbilityConfigurationContent>();
			configuration->numericSettings = loaded.numericSettings;
			configuration->actors = loaded.actorDefinitions;
			loaded.definition.configurationContent = std::move(configuration);
			std::set<std::string> ids;
			loaded.definition.actorDefinitionIds.clear();
			for (const auto& actor : loaded.actorDefinitions)
			{
				if (!ids.insert(actor.actorDefinitionId.ToString()).second) throw std::runtime_error("Duplicate combination actor ID: " + actor.actorDefinitionId.ToString());
				loaded.definition.actorDefinitionIds.push_back(actor.actorDefinitionId);
			}
		}

		std::string CombinationLabel(const GameAbilityDefinition& definition)
		{
			std::string label = "Ability '" + definition.abilityId + "' evolves [";
			for (const auto& id : definition.activeEvolveIds) label += id + " ";
			return label + "]: ";
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

		return LoadFromJson(*documentResult.document, fallbackDefinitions, fallbackActorDefinitions);
	}

	AbilityLoader::Result AbilityLoader::LoadFromJson(const nlohmann::json& root, const List<const GameAbilityDefinition*>& fallbackDefinitions, const List<const AbilityActorDefinition*>& fallbackActorDefinitions, const std::string& selectedId)
	{
		std::string failureContext;
		try
		{
			std::string contractFailure;
			if (!RegisterGameAbilitySettingContracts(&contractFailure))
			{
				return Result{ {}, contractFailure };
			}


			if (root.at("schemaVersion").get<int>() != 1)
			{
				return Result{ {}, "Unsupported ability schema version" };
			}

			Result result;
			std::map<std::string, const Json*> sourceAbilities;
			std::set<std::string> abilityIds;
			std::set<std::string> actorIds;
			std::set<std::string> evolveIds;
			for (const Json& ability : root.at("abilities"))
			{
				const std::string abilityId = ability_loader_detail::RequiredString(ability, "id");
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
				const std::string abilityId = ability_loader_detail::RequiredString(ability, "id");
				if (!selectedId.empty() && abilityId != selectedId) continue;
				failureContext = "Ability '" + abilityId + "': ";
				materializeAbility(abilityId);
				LoadedDefinition definition = ability_loader_detail::ParseAbility(
					materializedAbilities.at(abilityId),
					fallbackDefinitions,
					fallbackActorDefinitions,
					cppBaseIds.at(abilityId)
				);
				for (const AbilityActorDefinition& actor : definition.actorDefinitions)
				{
					definition.definition.actorDefinitionIds.push_back(actor.actorDefinitionId);
					if (!actorIds.insert(actor.actorDefinitionId.ToString()).second)
					{
						throw std::runtime_error(
							"Duplicate ability actor ID: " +
							actor.actorDefinitionId.ToString()
						);
					}
				}
				if (definition.definition.actorDefinitionIds.empty())
				{
					const Json* actorOwner = &ability;
					while (!actorOwner->contains("actors") && actorOwner->contains("baseId")) actorOwner = sourceAbilities.at(actorOwner->at("baseId").get<std::string>());
					if (actorOwner->contains("actors")) for (const auto& actor : actorOwner->at("actors")) definition.definition.actorDefinitionIds.emplace_back(actor.at("id").get<std::string>());
				}
				Json source = materializedAbilities.at(abilityId);
				if (source.contains("evolves") && !source.at("evolves").is_array()) throw std::runtime_error("Ability '" + abilityId + "': evolves must be an array");
				const Json evolves = source.value("evolves", Json::array());
				if (!evolves.empty())
				{
					// Variants share global actor records, but their private combinations need the inherited JSON values.
					if (!source.contains("actors"))
					{
						const Json* actorOwner = &ability;
						while (!actorOwner->contains("actors") && actorOwner->contains("baseId")) actorOwner = sourceAbilities.at(actorOwner->at("baseId").get<std::string>());
						if (actorOwner->contains("actors")) source["actors"] = actorOwner->at("actors");
					}
					if (evolves.size() != 3) throw std::runtime_error("Ability '" + abilityId + "': evolves must contain zero or exactly three entries");
					auto content = std::make_shared<AbilityEvolveContent>();
					for (const auto& evolve : evolves)
					{
						const std::string id = ability_loader_detail::RequiredString(evolve, "id");
						failureContext = "Ability '" + abilityId + "' evolve '" + id + "': ";
						std::string reason;
						ParsedFamilyRoleId parsedEvolve;
						if (!ContentIdSchema::ParseAbilityEvolveId(id, parsedEvolve, &reason) || !evolveIds.insert(id).second) throw std::runtime_error("Ability '" + abilityId + "' evolve '" + id + "': invalid or duplicate ID. " + reason);
						ParsedAbilityId parsed;
						ContentIdSchema::ParseAbilityId(abilityId, parsed);
						const std::string family = parsed.family.substr(parsed.family.find_last_of('.') + 1);
						if (parsedEvolve.family != family) throw std::runtime_error("Ability '" + abilityId + "' evolve '" + id + "': family must match ability");
						const std::string name = ability_loader_detail::RequiredString(evolve, "name"), description = ability_loader_detail::RequiredString(evolve, "description");
						if (name.find_first_not_of(" \t\r\n") == std::string::npos || description.find_first_not_of(" \t\r\n") == std::string::npos) throw std::runtime_error("Ability '" + abilityId + "' evolve '" + id + "': name and description must be nonempty English text");
						const Json& patch = evolve.at("patch");
						if (!patch.is_object()) throw std::runtime_error("Ability '" + abilityId + "' evolve '" + id + "': patch must be an object");
						const std::set<std::string> allowed{ "cooldown", "duration", "maxCharges", "chargeUseInterval", "settings", "attributes", "scalingRules", "progression", "effectSpecs", "damageTags", "actors", "attributeProfiles", "behaviorId", "invocationOutputAttributes", "mainEffectAttributes" };
						for (const auto& [field, value] : patch.items()) if (!allowed.count(field)) throw std::runtime_error("Ability '" + abilityId + "' evolve '" + id + "': forbidden patch field '" + field + "'");
						content->options.push_back({ id, name, description });
					}
					auto baseCombination = ability_loader_detail::ParseAbility(source, fallbackDefinitions, fallbackActorDefinitions, cppBaseIds.at(abilityId));
					AttachConfiguration(baseCombination);
					definition.definition.configurationContent = baseCombination.definition.configurationContent;
					definition.definition.actorDefinitionIds = baseCombination.definition.actorDefinitionIds;
					content->combinations.push_back(std::move(baseCombination.definition));
					const auto addCombination = [&](std::size_t first, std::optional<std::size_t> second)
					{
						List<std::string> selection{ content->options[first].id };
						if (second) selection.push_back(content->options[*second].id);
						try
						{
							Json combined = MergeJsonObjects(source, evolves[first].at("patch"));
							if (second) combined = MergeJsonObjects(std::move(combined), evolves[*second].at("patch"));
							const bool replaceActors = evolves[first].at("patch").contains("actors") || (second && evolves[*second].at("patch").contains("actors"));
							auto parsed = ability_loader_detail::ParseAbility(combined, fallbackDefinitions, fallbackActorDefinitions, cppBaseIds.at(abilityId), replaceActors);
							parsed.definition.activeEvolveIds = selection;
							AttachConfiguration(parsed);
							content->combinations.push_back(std::move(parsed.definition));
						}
						catch (const std::exception& error)
						{
							std::string ids;
							for (const auto& id : selection) ids += id + " ";
							throw std::runtime_error("Ability '" + abilityId + "' evolves [" + ids + "]: " + error.what());
						}
					};
					for (std::size_t first = 0; first < evolves.size(); ++first)
					{
						addCombination(first, std::nullopt);
						for (std::size_t second = 0; second < evolves.size(); ++second) if (first != second) addCombination(first, second);
					}
					definition.definition.evolveContent = std::move(content);
				}
				result.definitions.push_back(std::move(definition));
			}
			if (result.definitions.empty())
			{
				return Result{ {}, "Ability catalog is empty" };
			}
			ScopedAbilityValidationView catalogView{ result.definitions };
			for (const auto& loaded : result.definitions)
			{
				if (!loaded.definition.evolveContent) continue;
				if (!RegisterGameAbilityContent()) throw std::runtime_error("Could not register ability content for evolve validation");
				for (const auto& combination : loaded.definition.evolveContent->combinations)
				{
					ScopedAbilityDefinitionView combinationView{ combination };
					std::string reason;
					const auto validateEffect = [](const sas::GameplayEffectDefinition& effect, std::string* error) { return GameplayEffectDefinitionValidator::Validate(effect, [](const auto& key) { return GetEffectBehaviorRuntime().IsRegistered(key); }, error); };
					if (!GameAbilityDefinitionValidator::Validate(combination, validateEffect, &reason)) throw std::runtime_error(CombinationLabel(combination) + reason);
					for (const auto& actor : combination.configurationContent->actors)
					{
						const auto registered = std::find_if(fallbackActorDefinitions.begin(), fallbackActorDefinitions.end(), [&](const auto* candidate) { return candidate && candidate->actorDefinitionId == actor.actorDefinitionId; });
						if (registered == fallbackActorDefinitions.end()) throw std::runtime_error(CombinationLabel(combination) + "actor ID is not registered in C++: " + actor.actorDefinitionId.ToString());
						const auto validation = AbilityActorRegistry::ValidateDefinition(actor);
						if (!validation.isValid) throw std::runtime_error(CombinationLabel(combination) + "actor '" + actor.actorDefinitionId.ToString() + "': " + validation.reason);
					}
				}
			}
			return result;
		}
		catch (const std::exception& exception)
		{
			const std::string message = exception.what();
			return Result{
				{},
				"Failed to load abilities from 'content document': " + (message.find("Ability '") == std::string::npos ? failureContext : "") + message
			};
		}
	}
}
