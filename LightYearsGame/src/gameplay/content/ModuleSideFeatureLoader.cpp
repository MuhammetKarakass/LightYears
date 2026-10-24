#include "gameplay/module/ModuleDescriptionValues.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"

#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/content/ModuleLoader.h"
#include "framework/JsonDocumentLoader.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>

namespace vega::content
{
	ModuleSideFeatureLoader::Result ModuleSideFeatureLoader::LoadFromFile(const std::filesystem::path& filePath)
	{
		const JsonDocumentLoader::Result documentResult = JsonDocumentLoader::LoadFromFile(filePath);
		if (!documentResult.Succeeded()) return Result{ {}, {}, documentResult.error };

		try
		{
			const nlohmann::json& root = *documentResult.document;
			if (!root.is_object() || !root.contains("schemaVersion"))
				throw std::runtime_error("Unsupported module side-feature schema version");
			const nlohmann::json& schemaVersion = root.at("schemaVersion");
			if (!schemaVersion.is_number_integer() || schemaVersion != 1)
				throw std::runtime_error("Unsupported module side-feature schema version");

			Result result;
			std::set<std::string> poolIds;
			const nlohmann::json& pools = root.at("sharedPools");
			if (!pools.is_array()) throw std::runtime_error("sharedPools must be an array");
			for (std::size_t index = 0; index < pools.size(); ++index)
			{
				const nlohmann::json& entry = pools[index];
				const std::string context = "sharedPools[" + std::to_string(index) + "]";
				if (!entry.is_object()) throw std::runtime_error(context + " must be an object");
				const nlohmann::json& idValue = entry.at("id");
				if (!idValue.is_string()) throw std::runtime_error(context + ".id must be a string");
				const std::string id = idValue.get<std::string>();
				std::string failure;
				if (!ContentIdSchema::ValidateContentId(id, &failure)) throw std::runtime_error(context + ".id: " + failure);
				if (!poolIds.insert(id).second) throw std::runtime_error("Duplicate side-feature pool ID: " + id);

				ModuleSideFeaturePool pool;
				pool.poolId = sas::ContentId{ id };
				const nlohmann::json& choices = entry.at("choices");
				if (!choices.is_array() || choices.empty()) throw std::runtime_error(context + ".choices must be a non-empty array");
				std::set<std::string> seenChoices;
				for (std::size_t choiceIndex = 0; choiceIndex < choices.size(); ++choiceIndex)
				{
					const nlohmann::json& choice = choices[choiceIndex];
					const std::string choiceContext = context + ".choices[" + std::to_string(choiceIndex) + "]";
					if (!choice.is_object() || !choice.contains("sideFeatureId") || !choice.at("sideFeatureId").is_string())
						throw std::runtime_error(choiceContext + ".sideFeatureId must be a string");
					const std::string featureId = choice.at("sideFeatureId").get<std::string>();
					if (!ContentIdSchema::ValidateContentId(featureId, &failure))
						throw std::runtime_error(choiceContext + ".sideFeatureId: " + failure);
					if (!seenChoices.insert(featureId).second) throw std::runtime_error(choiceContext + " duplicates a feature");
					const nlohmann::json& weightValue = choice.at("weight");
					if (!weightValue.is_number()) throw std::runtime_error(choiceContext + ".weight must be numeric");
					const double weight = weightValue.get<double>();
					if (!std::isfinite(weight) || weight <= 0.0) throw std::runtime_error(choiceContext + ".weight must be finite and positive");
					pool.choices.push_back({ sas::ContentId{ featureId }, weight });
				}
				result.pools.push_back(std::move(pool));
			}

			std::set<std::string> featureIds;
			const nlohmann::json& features = root.at("sideFeatures");
			if (!features.is_array()) throw std::runtime_error("sideFeatures must be an array");
			for (std::size_t index = 0; index < features.size(); ++index)
			{
				const nlohmann::json& entry = features[index];
				const std::string context = "sideFeatures[" + std::to_string(index) + "]";
				if (!entry.is_object()) throw std::runtime_error(context + " must be an object");
				const nlohmann::json& idValue = entry.at("id");
				if (!idValue.is_string()) throw std::runtime_error(context + ".id must be a string");
				const std::string id = idValue.get<std::string>();
				std::string failure;
				if (!ContentIdSchema::ValidateContentId(id, &failure)) throw std::runtime_error(context + ".id: " + failure);
				if (!featureIds.insert(id).second) throw std::runtime_error("Duplicate side-feature ID: " + id);

				ModuleSideFeatureDefinition definition;
				definition.sideFeatureId = sas::ContentId{ id };
				if (entry.contains("displayName"))
				{
					if (!entry.at("displayName").is_string()) throw std::runtime_error(context + ".displayName must be a string");
					definition.displayName = entry.at("displayName").get<std::string>();
				}
				else definition.displayName = id;
				if (entry.contains("description"))
				{
					if (!entry.at("description").is_string()) throw std::runtime_error(context + ".description must be a string");
					definition.description = entry.at("description").get<std::string>();
				}
				if (entry.contains("poolId"))
				{
					const nlohmann::json& poolValue = entry.at("poolId");
					if (!poolValue.is_string()) throw std::runtime_error(context + ".poolId must be a string");
					const std::string poolId = poolValue.get<std::string>();
					if (!ContentIdSchema::ValidateContentId(poolId, &failure)) throw std::runtime_error(context + ".poolId: " + failure);
					definition.poolId = sas::ContentId{ poolId };
				}
				if (definition.poolId.IsValid() && entry.contains("rollWeight"))
					throw std::runtime_error(context + " shared feature weight must be owned by its pool choice");
				if (entry.contains("rollWeight"))
				{
					const nlohmann::json& weightValue = entry.at("rollWeight");
					if (!weightValue.is_number()) throw std::runtime_error(context + ".rollWeight must be numeric");
					definition.rollWeight = weightValue.get<double>();
					if (!std::isfinite(definition.rollWeight) || definition.rollWeight <= 0.0)
						throw std::runtime_error(context + ".rollWeight must be finite and positive");
				}
				if (entry.contains("levelCount"))
				{
					const nlohmann::json& levelValue = entry.at("levelCount");
					if (!levelValue.is_number_integer() || (levelValue.is_number_unsigned() &&
					levelValue.get<std::uint64_t>() > static_cast<std::uint64_t>(std::numeric_limits<int>::max())))
						throw std::runtime_error(context + ".levelCount must be a positive 32-bit integer");
					const std::int64_t levelCount = levelValue.get<std::int64_t>();
					if (levelCount <= 0 || levelCount > std::numeric_limits<int>::max())
						throw std::runtime_error(context + ".levelCount must be a positive 32-bit integer");
					definition.levelCount = static_cast<int>(levelCount);
				}
				definition.rules = ModuleLoader::ParseRulePackage(entry.at("rules"), context + ".rules", definition.levelCount, false, true);
				// Activation bursts may be independent side features; shield and kill bursts patch the main source.
				if (std::any_of(definition.rules.ownerBurstRules.begin(), definition.rules.ownerBurstRules.end(), [](const ModuleOwnerBurstRule& rule) { return rule.trigger != ModuleTriggerKind::AbilityActivated; })) throw std::runtime_error(context + ".rules.ownerBurstRules side sources require AbilityActivated; other bursts use modifiers");
				ValidateModuleDescriptionTemplate(definition.description, definition.rules, context);
				if (!ModuleLoader::ValidateRulePackageLevelCount(definition.rules, definition.levelCount, context,
					&failure)) throw std::runtime_error(failure);
				if (definition.rules.attributeModifiers.empty() && definition.rules.abilityModifiers.empty() &&
					definition.rules.triggerBuffs.empty() && definition.rules.damageRules.empty() && definition.rules.zoneEffects.empty() &&
					definition.rules.eventActions.empty() && definition.rules.ownerAttributeRules.empty() && definition.rules.movementFadeRules.empty() && definition.rules.conditionalMovementRules.empty() && definition.rules.shieldReflections.empty() && definition.rules.shieldRules.empty() && definition.rules.statusRules.Empty() && definition.rules.generatedRules.Empty() && definition.rules.targetSlowRules.empty() &&
					definition.rules.killStackCooldownRules.empty() && definition.rules.killStackDamageRules.empty() && definition.rules.sustainedSpeedRules.empty() && definition.rules.ownerBurstRules.empty() && definition.rules.ownerBurstModifiers.empty() && definition.rules.chargeRules.empty() && definition.rules.reviveRules.empty() && definition.rules.healthThresholdRules.empty() && !definition.rules.fragmentation)
					throw std::runtime_error(context + " rules must not be empty");
				result.definitions.push_back(std::move(definition));
			}
			return result;
		}
		catch (const std::exception& exception)
		{
			return Result{ {}, {}, "Failed to load module side features from '" + filePath.string() + "': " + exception.what() };
		}
	}
}
