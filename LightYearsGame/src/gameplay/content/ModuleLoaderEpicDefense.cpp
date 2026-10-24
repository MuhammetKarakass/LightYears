#include "ModuleLoaderParsing.h"

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace vega::content
{
	namespace module_loader_detail
	{
		// Epic Defense keys. reviveRules is a main package rule (Last Stand). healthThresholdRules are main package rules (cooldown
		// recovery) or side feature rules (ability damage, movement speed); a side feature never grows with level, so it takes no table.
		void ParseEpicDefenseRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature)
		{
			const Json revives = object.value("reviveRules", Json::array());
			if (!revives.is_array()) throw std::runtime_error(context + ".reviveRules must be an array");
			std::size_t index = 0;
			for (const Json& entry : revives)
			{
				const std::string entryContext = context + ".reviveRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				if (sideFeature) throw std::runtime_error(entryContext + " revive is a main package rule");
				if (!entry.at("healthFraction").is_array() || !entry.at("charges").is_array()) throw std::runtime_error(entryContext + " healthFraction and charges must be level tables");
				ModuleLevelTable<float> healthTable;
				ParseLevelFloat(entry, "healthFraction", entryContext + ".healthFraction", minimumLevelCount, healthTable);
				ValidateTable(healthTable, entryContext + ".healthFraction", [](float value) { return value > 0.f && value <= 1.f; }, "must be in (0, 1]");
				ModuleLevelTable<int> chargeTable;
				ParsePositiveLevelInt(entry, "charges", entryContext + ".charges", minimumLevelCount, chargeTable);
				ModuleReviveRule rule;
				rule.healthFractionByLevel = std::move(healthTable);
				rule.chargesByLevel = std::move(chargeTable);
				rule.bonusSeconds = ParseFiniteFloat(entry.at("bonusSeconds"), entryContext + ".bonusSeconds");
				if (rule.bonusSeconds <= 0.f) throw std::runtime_error(entryContext + " bonusSeconds must be positive");
				rule.bonusMultiplier = ParseFiniteFloat(entry.at("bonusMultiplier"), entryContext + ".bonusMultiplier");
				if (rule.bonusMultiplier <= 1.f) throw std::runtime_error(entryContext + " bonusMultiplier must exceed one");
				rules.reviveRules.push_back(std::move(rule));
			}

			const Json thresholds = object.value("healthThresholdRules", Json::array());
			if (!thresholds.is_array()) throw std::runtime_error(context + ".healthThresholdRules must be an array");
			index = 0;
			for (const Json& entry : thresholds)
			{
				const std::string entryContext = context + ".healthThresholdRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				ModuleHealthThresholdRule rule;
				rule.healthAtMost = ParseFiniteFloat(entry.at("healthAtMost"), entryContext + ".healthAtMost");
				if (rule.healthAtMost <= 0.f || rule.healthAtMost >= 1.f) throw std::runtime_error(entryContext + " healthAtMost must be in (0, 1)");
				const bool hasCooldown = entry.contains("cooldownRecoveryMultiplier");
				const bool hasDamage = entry.contains("abilityDamageMultiplier");
				const bool hasMovement = entry.contains("movementSpeedMultiplier");
				if (!hasCooldown && !hasDamage && !hasMovement) throw std::runtime_error(entryContext + " needs at least one multiplier");
				if (hasCooldown)
				{
					if (sideFeature) throw std::runtime_error(entryContext + " cooldown recovery is a main package rule");
					if (!entry.at("cooldownRecoveryMultiplier").is_array()) throw std::runtime_error(entryContext + ".cooldownRecoveryMultiplier must be a level table");
					ModuleLevelTable<float> table;
					ParseLevelFloat(entry, "cooldownRecoveryMultiplier", entryContext + ".cooldownRecoveryMultiplier", minimumLevelCount, table);
					ValidateTable(table, entryContext + ".cooldownRecoveryMultiplier", [](float value) { return value > 1.f; }, "must exceed one");
					rule.cooldownRecoveryMultiplierByLevel = std::move(table);
				}
				if (hasDamage)
				{
					rule.abilityDamageMultiplier = ParseFiniteFloat(entry.at("abilityDamageMultiplier"), entryContext + ".abilityDamageMultiplier");
					if (rule.abilityDamageMultiplier <= 1.f) throw std::runtime_error(entryContext + " abilityDamageMultiplier must exceed one");
				}
				if (hasMovement)
				{
					rule.movementSpeedMultiplier = ParseFiniteFloat(entry.at("movementSpeedMultiplier"), entryContext + ".movementSpeedMultiplier");
					if (rule.movementSpeedMultiplier <= 1.f) throw std::runtime_error(entryContext + " movementSpeedMultiplier must exceed one");
				}
				rules.healthThresholdRules.push_back(std::move(rule));
			}
		}
	}
}
