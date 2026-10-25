#include "gameplay/module/rules/EpicOffenseRules.h"

#include <algorithm>

namespace vega
{
	// Strongest value wins across placements and side features; flags record that some placement provides the rule. Blood Contract
	// multipliers and the cost fraction start from their first authored value, so a zero default never caps them.
	void MergeEpicOffenseRules(EpicOffenseEffects& effects, const ModuleEpicOffenseRules& rules, int level)
	{
		for (const auto& rule : rules.monochromeCores)
		{
			effects.monochromePairMultiplier = std::max(effects.monochromePairMultiplier, rule.pairMultiplierByLevel.ValueAtLevel(rule.pairMultiplier, level));
			effects.monochromeAdditionalAbilityBonus = std::max(effects.monochromeAdditionalAbilityBonus, rule.additionalAbilityBonus);
		}
		for (const ModulePrismCoreRule& rule : rules.prismCores)
		{
			effects.prismCore = true;
			effects.prismDurationMultiplier = std::max(effects.prismDurationMultiplier, rule.statusDurationMultiplierByLevel.ValueAtLevel(rule.statusDurationMultiplier, level));
			effects.prismExtraStacks = std::max(effects.prismExtraStacks, rule.extraStacks);
		}
		for (const ModuleStormCapacitorRule& rule : rules.stormCapacitors)
		{
			effects.stormCapacitor = true;
			effects.stormChargesRequired = std::max(effects.stormChargesRequired, rule.chargesRequired);
			effects.stormTargetCount = std::max(effects.stormTargetCount, rule.targetCount);
			effects.stormElectricStacks = std::max(effects.stormElectricStacks, rule.electricStacks);
			effects.stormRadius = std::max(effects.stormRadius, rule.radius);
			effects.stormPrimaryDamageFraction = std::max(effects.stormPrimaryDamageFraction, rule.primaryDamageFraction);
		}
		for (const ModuleAlternatingCurrentRule& rule : rules.alternatingCurrents)
		{
			effects.alternatingCurrent = true;
			effects.alternatingMultiplier = std::max(effects.alternatingMultiplier, rule.damageMultiplier);
		}
		for (const ModuleBloodContractRule& rule : rules.bloodContracts)
		{
			const bool first = !effects.bloodContract;
			effects.bloodContract = true;
			const auto merge = [first](float& target, float value) { target = first ? value : std::max(target, value); };
			merge(effects.bloodCostFraction, rule.costFraction);
			merge(effects.bloodHealFraction, rule.healFraction);
			merge(effects.bloodHealWindowSeconds, rule.healWindowSeconds);
			merge(effects.bloodFullHealthMultiplier, rule.fullHealthMultiplierByLevel.ValueAtLevel(rule.fullHealthMultiplier, level));
			merge(effects.bloodEmptyHealthMultiplier, rule.emptyHealthMultiplierByLevel.ValueAtLevel(rule.emptyHealthMultiplier, level));
		}
		for (const ModuleClotArmorRule& rule : rules.clotArmors)
		{
			effects.clotArmorPerCost = std::max(effects.clotArmorPerCost, rule.armorPerCost);
			effects.clotArmorSeconds = std::max(effects.clotArmorSeconds, rule.seconds);
		}
		for (const ModuleDebtCollectionRule& rule : rules.debtCollections)
		{
			effects.debtHealWindowSeconds = std::max(effects.debtHealWindowSeconds, rule.healWindowSeconds);
		}
	}

	void VisitEpicOffenseDescriptionValues(const ModuleEpicOffenseRules& rules, int level, const std::function<void(const std::string&, float)>& visitor)
	{
		for (std::size_t i = 0; i < rules.monochromeCores.size(); ++i)
		{
			const auto& rule = rules.monochromeCores[i];
			const std::string key = "monochrome." + std::to_string(i) + ".";
			visitor(key + "pairMultiplier", rule.pairMultiplierByLevel.ValueAtLevel(rule.pairMultiplier, level));
			visitor(key + "additionalAbilityBonus", rule.additionalAbilityBonus);
		}
		for (std::size_t i = 0; i < rules.prismCores.size(); ++i)
		{
			const ModulePrismCoreRule& rule = rules.prismCores[i];
			const std::string key = "prism." + std::to_string(i) + ".";
			const float multiplier = rule.statusDurationMultiplierByLevel.ValueAtLevel(rule.statusDurationMultiplier, level);
			visitor(key + "durationBonusPercent", (multiplier - 1.f) * 100.f);
			visitor(key + "durationMultiplier", multiplier);
			visitor(key + "extraStacks", static_cast<float>(rule.extraStacks));
		}
		for (std::size_t i = 0; i < rules.stormCapacitors.size(); ++i)
		{
			const ModuleStormCapacitorRule& rule = rules.stormCapacitors[i];
			const std::string key = "storm." + std::to_string(i) + ".";
			visitor(key + "chargesRequired", static_cast<float>(rule.chargesRequired));
			visitor(key + "targetCount", static_cast<float>(rule.targetCount));
			visitor(key + "electricStacks", static_cast<float>(rule.electricStacks));
			visitor(key + "radius", rule.radius);
			visitor(key + "primaryDamagePercent", rule.primaryDamageFraction * 100.f);
		}
		for (std::size_t i = 0; i < rules.alternatingCurrents.size(); ++i)
		{
			visitor("alternating." + std::to_string(i) + ".bonusPercent", (rules.alternatingCurrents[i].damageMultiplier - 1.f) * 100.f);
		}
		for (std::size_t i = 0; i < rules.bloodContracts.size(); ++i)
		{
			const ModuleBloodContractRule& rule = rules.bloodContracts[i];
			const std::string key = "blood." + std::to_string(i) + ".";
			visitor(key + "costPercent", rule.costFraction * 100.f);
			visitor(key + "healPercent", rule.healFraction * 100.f);
			visitor(key + "healWindowSeconds", rule.healWindowSeconds);
			visitor(key + "fullBonusPercent", (rule.fullHealthMultiplierByLevel.ValueAtLevel(rule.fullHealthMultiplier, level) - 1.f) * 100.f);
			visitor(key + "emptyBonusPercent", (rule.emptyHealthMultiplierByLevel.ValueAtLevel(rule.emptyHealthMultiplier, level) - 1.f) * 100.f);
		}
		for (std::size_t i = 0; i < rules.clotArmors.size(); ++i)
		{
			const std::string key = "clot." + std::to_string(i) + ".";
			visitor(key + "armorPerCost", rules.clotArmors[i].armorPerCost);
			visitor(key + "seconds", rules.clotArmors[i].seconds);
		}
		for (std::size_t i = 0; i < rules.debtCollections.size(); ++i)
		{
			visitor("debt." + std::to_string(i) + ".healWindowSeconds", rules.debtCollections[i].healWindowSeconds);
		}
	}
}
