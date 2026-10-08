#include "gameplay/module/ModuleDescriptionValues.h"
#include <set>
#include <stdexcept>

namespace ly
{
	namespace
	{
		struct NumericValues
		{
			const ModuleDescriptionValueVisitor& visitor;
			void Number(const std::string& key, float value) const { visitor(key, value); }
			void Modifier(const sas::AttributeModifier& modifier, const ModuleLevelTable<float>& table, int level) const
			{
				const float value = table.ValueAtLevel(modifier.magnitude, level); const std::string key{ modifier.attributeId.GetName() };
				Number(key, value); Number(key + ".percent", value * 100.f); Number(key + ".bonusPercent", (value - 1.f) * 100.f); Number(key + ".reductionPercent", (1.f - value) * 100.f);
			}
		};
	}
	void VisitModuleDescriptionValues(const ModuleRulePackage& rules, int level, const ModuleDescriptionValueVisitor& visitor)
	{
		const NumericValues out{ visitor };
		for (const auto& entry : rules.attributeModifiers) out.Modifier(entry.modifier, entry.magnitudeByLevel, level);
		for (const auto& entry : rules.abilityModifiers) out.Modifier(entry.modifier, entry.magnitudeByLevel, level);
		for (std::size_t i = 0; i < rules.triggerBuffs.size(); ++i)
		{
			const auto& buff = rules.triggerBuffs[i]; const std::string key = "trigger." + std::to_string(i) + ".";
			out.Number(key + "durationSeconds", buff.durationSecondsByLevel.ValueAtLevel(buff.durationSeconds, level));
			out.Number(key + "movementSpeedMultiplier", buff.movementSpeedMultiplierByLevel.ValueAtLevel(buff.movementSpeedMultiplier, level));
			for (const auto& entry : buff.modifiers) out.Modifier(entry.modifier, entry.magnitudeByLevel, level);
		}
		for (std::size_t i = 0; i < rules.damageRules.size(); ++i)
		{
			const auto& rule = rules.damageRules[i]; const std::string key = "damage." + std::to_string(i) + ".";
			const float multiplier = rule.damageMultiplierByLevel.ValueAtLevel(rule.damageMultiplier, level);
			out.Number(key + "damageMultiplier", multiplier); out.Number(key + "bonusPercent", (multiplier - 1.f) * 100.f); out.Number(key + "reductionPercent", (1.f - multiplier) * 100.f);
			if (rule.condition == ModuleDamageCondition::FullMagazineShot) out.Number(key + "openingShotCount", static_cast<float>(rule.openingShotCountByLevel.ValueAtLevel(rule.openingShotCount, level)));
			if (rule.condition == ModuleDamageCondition::TargetHealthAtMost || rule.condition == ModuleDamageCondition::OwnerHealthAtMost) out.Number(key + "conditionPercent", rule.conditionValueByLevel.ValueAtLevel(rule.conditionValue, level) * 100.f);
			if (rule.bonusDamageFraction > 0.f) out.Number(key + "bonusDamagePercent", rule.bonusDamageFractionByLevel.ValueAtLevel(rule.bonusDamageFraction, level) * 100.f);
			if (rule.maxHealthDamageFraction > 0.f) out.Number(key + "maxHealthDamagePercent", rule.maxHealthDamageFractionByLevel.ValueAtLevel(rule.maxHealthDamageFraction, level) * 100.f);
			if (rule.activeWhileTrigger != ModuleTriggerKind::None) out.Number(key + "activeDurationSeconds", rule.activeDurationSecondsByLevel.ValueAtLevel(rule.activeDurationSeconds, level));
		}
		for (const auto& rule : rules.zoneEffects) out.Number("cooldown." + rule.zone.name + ".multiplier", rule.multiplierByLevel.ValueAtLevel(rule.multiplier, level));
		for (std::size_t i = 0; i < rules.eventActions.size(); ++i)
		{
			const auto& rule = rules.eventActions[i]; const std::string key = "action." + std::to_string(i) + ".";
			const float amount = rule.amountByLevel.ValueAtLevel(rule.amount, level);
			out.Number(key + "amount", amount); out.Number(key + "amountPercent", amount * 100.f); out.Number(key + "chancePerLuckPercent", rule.chancePerLuckByLevel.ValueAtLevel(rule.chancePerLuck, level) * 100.f);
			out.Number(key + "everyCount", static_cast<float>(rule.everyCount)); out.Number(key + "targetCount", static_cast<float>(rule.targetCountByLevel.ValueAtLevel(rule.targetCount, level)));
			out.Number(key + "radius", rule.radius); out.Number(key + "cooldownSeconds", rule.cooldownSeconds);
		}
		for (std::size_t i = 0; i < rules.ownerAttributeRules.size(); ++i)
		{
			const auto& rule = rules.ownerAttributeRules[i]; const std::string key = "owner." + std::to_string(i) + ".";
			out.Number(key + "amount", rule.amountByLevel.ValueAtLevel(rule.amount, level)); out.Number(key + "healthPercent", rule.healthAtMost * 100.f);
		}
		for (std::size_t i = 0; i < rules.movementFadeRules.size(); ++i) out.Number("movementFade." + std::to_string(i) + ".fadeSeconds", rules.movementFadeRules[i].fadeSeconds);
		for (std::size_t i = 0; i < rules.statusRules.durationBonuses.size(); ++i)
		{
			const auto& rule = rules.statusRules.durationBonuses[i]; out.Number("statusDuration." + std::to_string(i) + ".bonusSeconds", rule.bonusSecondsByLevel.ValueAtLevel(rule.bonusSeconds, level));
		}
		for (std::size_t i = 0; i < rules.shieldReflections.size(); ++i)
		{
			const auto& rule = rules.shieldReflections[i]; const std::string key = "reflection." + std::to_string(i) + ".";
			out.Number(key + "percent", rule.fractionByLevel.ValueAtLevel(rule.fraction, level) * 100.f); out.Number(key + "durationSeconds", rule.durationSeconds);
		}
		for (std::size_t i = 0; i < rules.statusRules.electricOverrides.size(); ++i)
		{
			const auto& rule = rules.statusRules.electricOverrides[i];
			for (std::size_t j = 0; j < rule.damageTakenByStack.size(); ++j)
			{
				const float value = j < rule.damageTakenByStackByLevel.size() ? rule.damageTakenByStackByLevel[j].ValueAtLevel(rule.damageTakenByStack[j], level) : rule.damageTakenByStack[j];
				out.Number("electric." + std::to_string(i) + ".stack." + std::to_string(j) + ".percent", value * 100.f);
			}
		}
		for (std::size_t i = 0; i < rules.statusRules.cryoFreezes.size(); ++i)
		{
			const auto& rule = rules.statusRules.cryoFreezes[i]; const std::string key = "freeze." + std::to_string(i) + ".";
			out.Number(key + "threshold", static_cast<float>(rule.threshold)); out.Number(key + "durationSeconds", rule.durationSecondsByLevel.ValueAtLevel(rule.durationSeconds, level)); out.Number(key + "cooldownSeconds", rule.cooldownSecondsByLevel.ValueAtLevel(rule.cooldownSeconds, level));
		}
		for (std::size_t i = 0; i < rules.statusRules.thermalCryoDurationBonuses.size(); ++i)
		{
			const auto& rule = rules.statusRules.thermalCryoDurationBonuses[i]; out.Number("duration." + std::to_string(i) + ".bonusSeconds", rule.bonusSecondsByLevel.ValueAtLevel(rule.bonusSeconds, level));
		}
		for (std::size_t i = 0; i < rules.statusRules.freezeSpreads.size(); ++i)
		{
			const auto& rule = rules.statusRules.freezeSpreads[i]; const std::string key = "spread." + std::to_string(i) + ".";
			out.Number(key + "radius", rule.radius); out.Number(key + "stacks", static_cast<float>(rule.stacks));
		}
		for (std::size_t i = 0; i < rules.generatedRules.ricochets.size(); ++i)
		{
			const auto& rule = rules.generatedRules.ricochets[i]; const std::string key = "ricochet." + std::to_string(i) + ".";
			out.Number(key + "damagePercent", rule.damageFractionByLevel.ValueAtLevel(rule.damageFraction, level) * 100.f); out.Number(key + "secondaryPercent", rule.secondaryFraction * 100.f); out.Number(key + "range", rule.range);
		}
		for (std::size_t i = 0; i < rules.generatedRules.guidedSalvos.size(); ++i)
		{
			const auto& rule = rules.generatedRules.guidedSalvos[i]; const std::string key = "salvo." + std::to_string(i) + ".";
			out.Number(key + "damagePercent", rule.damageFractionByLevel.ValueAtLevel(rule.damageFraction, level) * 100.f); out.Number(key + "projectileCount", static_cast<float>(rule.projectileCount)); out.Number(key + "speed", rule.speed); out.Number(key + "lifetime", rule.lifetime);
		}
		for (std::size_t i = 0; i < rules.generatedRules.delayedExplosions.size(); ++i)
		{
			const auto& rule = rules.generatedRules.delayedExplosions[i]; const std::string key = "aftershock." + std::to_string(i) + ".";
			out.Number(key + "damagePercent", rule.damageFractionByLevel.ValueAtLevel(rule.damageFraction, level) * 100.f); out.Number(key + "delaySeconds", rule.delaySeconds); out.Number(key + "radius", rule.radius);
		}
		for (std::size_t i = 0; i < rules.generatedRules.trailMines.size(); ++i)
		{
			const auto& rule = rules.generatedRules.trailMines[i]; const std::string key = "mine." + std::to_string(i) + ".";
			out.Number(key + "damage", rule.damageByLevel.ValueAtLevel(rule.damage, level)); out.Number(key + "maxActive", static_cast<float>(rule.maxActiveByLevel.ValueAtLevel(rule.maxActive, level)));
			out.Number(key + "spacing", rule.spacing); out.Number(key + "windowSeconds", rule.windowSeconds); out.Number(key + "fuseSeconds", rule.fuseSeconds); out.Number(key + "radius", rule.radius); out.Number(key + "additionalLimit", static_cast<float>(rule.additionalLimit)); out.Number(key + "cryoStacks", static_cast<float>(rule.cryoStacks));
		}
		for (std::size_t i = 0; i < rules.generatedRules.killExplosions.size(); ++i)
		{
			const auto& rule = rules.generatedRules.killExplosions[i]; const std::string key = "burst." + std::to_string(i) + ".";
			out.Number(key + "healthPercent", rule.maxHealthFractionByLevel.ValueAtLevel(rule.maxHealthFraction, level) * 100.f); out.Number(key + "radius", rule.radius);
		}
	}
	void ValidateModuleDescriptionTemplate(const std::string& text, const ModuleRulePackage& rules, const std::string& context)
	{
		std::set<std::string> keys;
		VisitModuleDescriptionValues(rules, 1, [&keys](const std::string& key, float) { keys.insert(key); });
		std::size_t at = 0;
		while (true)
		{
			const auto open = text.find('{', at); const auto strayClose = text.find('}', at);
			if (open == std::string::npos)
			{
				if (strayClose != std::string::npos) throw std::runtime_error{ context + " description has an unmatched closing brace" };
				return;
			}
			if (strayClose != std::string::npos && strayClose < open) throw std::runtime_error{ context + " description has an unmatched closing brace" };
			const auto close = text.find('}', open + 1);
			if (close == std::string::npos || close == open + 1) throw std::runtime_error{ context + " description has an invalid attribute placeholder" };
			const auto token = text.substr(open + 1, close - open - 1);
			if (!keys.count(token)) throw std::runtime_error{ context + " description references a missing numeric value: " + token };
			at = close + 1;
		}
	}
}
