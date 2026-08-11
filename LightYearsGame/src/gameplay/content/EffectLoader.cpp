#include "gameplay/content/EffectLoader.h"

#include "attributes/AttributeId.h"
#include "framework/JsonDocumentLoader.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace ly::content
{
	namespace
	{
		using Json = JsonDocumentLoader::Json;

		const sas::GameplayEffectDefinition* FindFallback(
			const List<const sas::GameplayEffectDefinition*>& fallbackDefinitions,
			const std::string& effectId
		)
		{
			for (const sas::GameplayEffectDefinition* definition : fallbackDefinitions)
			{
				if (definition && definition->effectId == effectId)
				{
					return definition;
				}
			}
			return nullptr;
		}

		sas::GameplayEffectDurationPolicy ParseDurationPolicy(const std::string& value)
		{
			if (value == "Instant")
			{
				return sas::GameplayEffectDurationPolicy::Instant;
			}
			if (value == "Duration")
			{
				return sas::GameplayEffectDurationPolicy::Duration;
			}
			if (value == "Infinite")
			{
				return sas::GameplayEffectDurationPolicy::Infinite;
			}
			throw std::runtime_error("Unknown gameplay effect duration policy: " + value);
		}

		sas::GameplayEffectStackingPolicy ParseStackingPolicy(const std::string& value)
		{
			if (value == "None")
			{
				return sas::GameplayEffectStackingPolicy::None;
			}
			if (value == "RefreshDuration")
			{
				return sas::GameplayEffectStackingPolicy::RefreshDuration;
			}
			if (value == "Stack")
			{
				return sas::GameplayEffectStackingPolicy::Stack;
			}
			throw std::runtime_error("Unknown gameplay effect stacking policy: " + value);
		}

		sas::GameplayEffectDisposition ParseDisposition(const std::string& value)
		{
			if (value == "Beneficial")
			{
				return sas::GameplayEffectDisposition::Beneficial;
			}
			if (value == "Harmful")
			{
				return sas::GameplayEffectDisposition::Harmful;
			}
			if (value == "Neutral")
			{
				return sas::GameplayEffectDisposition::Neutral;
			}
			throw std::runtime_error("Unknown gameplay effect disposition: " + value);
		}

		List<GameplayTag> ParseTags(const Json& values)
		{
			List<GameplayTag> tags;
			for (const Json& value : values)
			{
				tags.emplace_back(GameplayTag{ value.get<std::string>() });
			}
			return tags;
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
			throw std::runtime_error("Unknown gameplay effect modifier operation: " + value);
		}

		List<sas::AttributeModifier> ParseModifiers(const Json& values)
		{
			List<sas::AttributeModifier> modifiers;
			for (const Json& value : values)
			{
				modifiers.push_back(sas::AttributeModifier{
					sas::AttributeId{ value.at("attributeId").get<std::string>() },
					ParseOperation(value.at("operation").get<std::string>()),
					value.at("magnitude").get<float>(),
					value.value("priority", 0)
				});
			}
			return modifiers;
		}

		sas::GameplayAttributeList ParseAttributes(const Json& values)
		{
			sas::GameplayAttributeList attributes;
			for (const Json& value : values)
			{
				attributes.push_back(sas::GameplayAttribute{
					sas::AttributeId{ value.at("id").get<std::string>() },
					value.at("baseValue").get<float>(),
					value.value("minValue", 0.f),
					value.value("maxValue", std::numeric_limits<float>::max())
				});
			}
			return attributes;
		}

		EffectLoader::LoadedDefinition ParseEffect(
			const Json& object,
			const List<const sas::GameplayEffectDefinition*>& fallbackDefinitions
		)
		{
			const std::string id = object.at("id").get<std::string>();
			std::string idFailure;
			if (!ContentIdSchema::ValidateEffectId(id, &idFailure))
			{
				throw std::runtime_error(
					"Invalid gameplay effect ID '" + id + "': " + idFailure
				);
			}
			for (const char* requiredField : { "durationPolicy", "stackingPolicy", "sourceParameterized" })
			{
				if (!object.contains(requiredField))
				{
					throw std::runtime_error(
						"Gameplay effect '" + id +
						"' requires JSON field '" + requiredField + "'"
					);
				}
			}
			const sas::GameplayEffectDefinition* fallback = FindFallback(
				fallbackDefinitions,
				id
			);
			if (!fallback && !object.contains("behaviorKey"))
			{
				throw std::runtime_error(
					"Gameplay effect '" + id +
					"' requires either JSON behaviorKey or a C++ behavior base"
				);
			}

			EffectLoader::LoadedDefinition loaded{ id, {} };
			// The fallback contributes only the executable behavior selector. Every
			// other effect-content field is reset and must come from JSON. This keeps
			// the C++ record a typed behavior fallback instead of a second balance or
			// metadata catalog.
			loaded.definition.effectId = id;
			if (fallback)
			{
				loaded.definition.behaviorKey = fallback->behaviorKey;
			}
			loaded.definition.sourceParameterized = object.value(
				"sourceParameterized",
				false
			);
			if (loaded.definition.sourceParameterized)
			{
				for (const char* forbiddenField : { "duration", "maxStacks", "modifiers", "attributes" })
				{
					if (object.contains(forbiddenField))
					{
						throw std::runtime_error(
							"Source-parameterized effect '" + id +
							"' cannot own numeric field '" + forbiddenField + "'"
						);
					}
				}
				loaded.definition.duration = 0.f;
				loaded.definition.maxStacks = 1;
				loaded.definition.modifiers.clear();
				loaded.definition.attributes.clear();
			}
			if (object.contains("behaviorKey"))
			{
				loaded.definition.behaviorKey = sas::GameplayEffectBehaviorKey{
					object.at("behaviorKey").get<std::string>()
				};
			}
			if (object.contains("durationPolicy"))
			{
				loaded.definition.durationPolicy = ParseDurationPolicy(
					object.at("durationPolicy").get<std::string>()
				);
			}
			if (object.contains("stackingPolicy"))
			{
				loaded.definition.stackingPolicy = ParseStackingPolicy(
					object.at("stackingPolicy").get<std::string>()
				);
			}
			if (object.contains("duration"))
			{
				loaded.definition.duration = object.at("duration").get<float>();
			}
			if (object.contains("maxStacks"))
			{
				loaded.definition.maxStacks = object.at("maxStacks").get<int>();
			}
			if (object.contains("grantedTags"))
			{
				loaded.definition.grantedTags = ParseTags(object.at("grantedTags"));
				for (const GameplayTag& tag : loaded.definition.grantedTags)
				{
					if (!GameplayTagSchema::ValidateEffectGrantedTag(tag, &idFailure))
					{
						throw std::runtime_error(
							"Gameplay effect '" + id + "' has invalid granted tag '" +
							tag.ToString() + "': " + idFailure
						);
					}
					if (tag.ToString() == id)
					{
						throw std::runtime_error(
							"Gameplay effect ID must not also be used as a granted tag: " + id
						);
					}
				}
			}
			if (object.contains("modifiers"))
			{
				loaded.definition.modifiers = ParseModifiers(object.at("modifiers"));
			}
			if (object.contains("attributes"))
			{
				loaded.definition.attributes = ParseAttributes(object.at("attributes"));
			}
			if (object.contains("activeVisualId"))
			{
				loaded.definition.activeVisualId = object.at("activeVisualId").get<std::string>();
				if (!ContentIdSchema::ValidateGameplayEffectVisualId(
					loaded.definition.activeVisualId,
					&idFailure
				))
				{
					throw std::runtime_error(
						"Gameplay effect '" + id + "' has invalid active visual ID: " +
						idFailure
					);
				}
			}
			if (object.contains("applicationRequiredTags"))
			{
				loaded.definition.applicationRequiredTags = ParseTags(
					object.at("applicationRequiredTags")
				);
				for (const GameplayTag& tag : loaded.definition.applicationRequiredTags)
				{
					if (!GameplayTagSchema::ValidateEffectApplicationTag(tag, &idFailure))
					{
						throw std::runtime_error(
							"Gameplay effect '" + id + "' has invalid required tag '" +
							tag.ToString() + "': " + idFailure
						);
					}
				}
			}
			if (object.contains("applicationBlockedTags"))
			{
				loaded.definition.applicationBlockedTags = ParseTags(
					object.at("applicationBlockedTags")
				);
				for (const GameplayTag& tag : loaded.definition.applicationBlockedTags)
				{
					if (!GameplayTagSchema::ValidateEffectApplicationTag(tag, &idFailure))
					{
						throw std::runtime_error(
							"Gameplay effect '" + id + "' has invalid blocked tag '" +
							tag.ToString() + "': " + idFailure
						);
					}
				}
			}
			if (object.contains("sourceScopedApplication"))
			{
				loaded.definition.sourceScopedApplication = object.at(
					"sourceScopedApplication"
				).get<bool>();
			}
			if (object.contains("disposition"))
			{
				loaded.definition.disposition = ParseDisposition(
					object.at("disposition").get<std::string>()
				);
			}
			if (object.contains("cleanseable"))
			{
				loaded.definition.cleanseable = object.at("cleanseable").get<bool>();
			}
			if (object.contains("category"))
			{
				loaded.definition.category = object.at("category").get<std::string>();
			}
			if (object.contains("immunityCategory"))
			{
				loaded.definition.immunityCategory = object.at(
					"immunityCategory"
				).get<std::string>();
			}
			if (object.contains("grantedImmunityCategory"))
			{
				loaded.definition.grantedImmunityCategory = object.at(
					"grantedImmunityCategory"
				).get<std::string>();
			}
			return loaded;
		}
	}

	EffectLoader::Result EffectLoader::LoadFromFile(
		const std::filesystem::path& filePath,
		const List<const sas::GameplayEffectDefinition*>& fallbackDefinitions
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
			const Json& root = *documentResult.document;
			if (root.at("schemaVersion").get<int>() != 1)
			{
				return Result{ {}, "Unsupported gameplay effect schema version" };
			}

			Result result;
			std::set<std::string> effectIds;
			for (const Json& effect : root.at("effects"))
			{
				LoadedDefinition loaded = ParseEffect(effect, fallbackDefinitions);
				if (loaded.id.empty())
				{
					throw std::runtime_error("Gameplay effect ID cannot be empty");
				}
				if (!effectIds.insert(loaded.id).second)
				{
					throw std::runtime_error("Duplicate gameplay effect ID: " + loaded.id);
				}
				result.definitions.push_back(std::move(loaded));
			}
			if (result.definitions.empty())
			{
				return Result{ {}, "Gameplay effect catalog is empty" };
			}
			return result;
		}
		catch (const std::exception& exception)
		{
			return Result{
				{},
				"Failed to load gameplay effects from '" + filePath.string() + "': " + exception.what()
			};
		}
	}
}
