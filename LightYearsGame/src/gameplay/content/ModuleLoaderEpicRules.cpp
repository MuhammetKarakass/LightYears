#include "ModuleLoaderParsing.h"

#include <cstddef>
#include <stdexcept>
#include <string>

namespace vega::content
{
	namespace module_loader_detail
	{
		namespace
		{
			// Per-level float table that must be an array of levelCount entries (a scalar would not reach the table).
			void ParseLevelFloats(const Json& object, const std::string& field, const std::string& context, int minimumLevelCount, ModuleLevelTable<float>& table)
			{
				if (!object.at(field).is_array()) throw std::runtime_error(context + " must be a per-level array");
				ParseLevelFloat(object, field, context, minimumLevelCount, table);
				ValidateTable(table, context, [](float number) { return number >= 0.f; }, "must not be negative");
			}
		}

		void ParseEpicUtilityMovementRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature)
		{
			const Json cascades = object.value("cascadeRules", Json::array());
			if (!cascades.is_array()) throw std::runtime_error(context + ".cascadeRules must be an array");
			std::size_t index = 0;
			for (const Json& entry : cascades)
			{
				const std::string entryContext = context + ".cascadeRules[" + std::to_string(index++) + "]";
				if (sideFeature) throw std::runtime_error(entryContext + " is main-package only");
				ModuleCascadeRule rule;
				ParseLevelFloats(entry, "stepBonus", entryContext + ".stepBonus", minimumLevelCount, rule.stepBonusByLevel);
				rule.maxSteps = ParsePositiveInt(entry.at("maxSteps"), entryContext + ".maxSteps");
				rule.windowSeconds = ParseFiniteFloat(entry.at("windowSeconds"), entryContext + ".windowSeconds");
				if (rule.windowSeconds <= 0.f) throw std::runtime_error(entryContext + " needs a positive window");
				rules.cascadeRules.push_back(rule);
			}

			const Json fieldMastery = object.value("fieldMasteryRules", Json::array());
			if (!fieldMastery.is_array()) throw std::runtime_error(context + ".fieldMasteryRules must be an array");
			index = 0;
			for (const Json& entry : fieldMastery)
			{
				const std::string entryContext = context + ".fieldMasteryRules[" + std::to_string(index++) + "]";
				const bool mainRule = entry.contains("usesPerLevelUp");
				const bool transferRule = entry.contains("transferUses");
				const bool overflowRule = entry.contains("overflowUses");
				if (static_cast<int>(mainRule) + static_cast<int>(transferRule) + static_cast<int>(overflowRule) != 1) throw std::runtime_error(entryContext + " needs exactly one Field Mastery rule kind");
				ModuleFieldMasteryRule rule;
				if (mainRule)
				{
					if (sideFeature) throw std::runtime_error(entryContext + ".usesPerLevelUp is main-package only");
					if (!entry.at("usesPerLevelUp").is_array()) throw std::runtime_error(entryContext + ".usesPerLevelUp must be a per-level array");
					(void)ParsePositiveLevelInt(entry, "usesPerLevelUp", entryContext + ".usesPerLevelUp", minimumLevelCount, rule.usesPerLevelUpByLevel);
				}
				else if (overflowRule)
				{
					if (!sideFeature) throw std::runtime_error(entryContext + " overflow is side-feature only");
					rule.overflowUses = ParsePositiveInt(entry.at("overflowUses"), entryContext);
					rule.overflowBonus = ParseFiniteFloat(entry.at("overflowBonus"), entryContext);
					if (rule.overflowBonus <= 0.f) throw std::runtime_error(entryContext + " overflow bonus must be positive");
				}
				else
				{
					if (!sideFeature) throw std::runtime_error(entryContext + ".transferUses is side-feature only");
					rule.transferUses = ParseFiniteFloat(entry.at("transferUses"), entryContext + ".transferUses");
					if (rule.transferUses <= 0.f) throw std::runtime_error(entryContext + " needs a positive transferUses");
				}
				rules.fieldMasteryRules.push_back(rule);
			}

			const Json assault = object.value("assaultVectorRules", Json::array());
			if (!assault.is_array()) throw std::runtime_error(context + ".assaultVectorRules must be an array");
			index = 0;
			for (const Json& entry : assault)
			{
				const std::string entryContext = context + ".assaultVectorRules[" + std::to_string(index++) + "]";
				if (sideFeature) throw std::runtime_error(entryContext + " is main-package only");
				ModuleAssaultVectorRule rule;
				ParseLevelFloats(entry, "damageBonus", entryContext + ".damageBonus", minimumLevelCount, rule.damageBonusByLevel);
				ParseLevelFloats(entry, "armorBonus", entryContext + ".armorBonus", minimumLevelCount, rule.armorBonusByLevel);
				rule.targetRange = ParseFiniteFloat(entry.at("targetRange"), entryContext + ".targetRange");
				rule.minimumCosine = ParseFiniteFloat(entry.at("minimumCosine"), entryContext + ".minimumCosine");
				rule.minimumSpeed = ParseFiniteFloat(entry.at("minimumSpeed"), entryContext + ".minimumSpeed");
				if (rule.targetRange <= 0.f || rule.minimumCosine < 0.f || rule.minimumCosine > 1.f || rule.minimumSpeed < 0.f) throw std::runtime_error(entryContext + " has invalid target range, direction cosine or speed");
				rules.assaultVectorRules.push_back(rule);
			}

			const Json phase = object.value("phaseSlipRules", Json::array());
			if (!phase.is_array()) throw std::runtime_error(context + ".phaseSlipRules must be an array");
			index = 0;
			for (const Json& entry : phase)
			{
				const std::string entryContext = context + ".phaseSlipRules[" + std::to_string(index++) + "]";
				if (sideFeature) throw std::runtime_error(entryContext + " is main-package only");
				ModulePhaseSlipRule rule;
				rule.phaseSeconds = ParseFiniteFloat(entry.at("phaseSeconds"), entryContext + ".phaseSeconds");
				rule.cooldownSeconds = ParseFiniteFloat(entry.at("cooldownSeconds"), entryContext + ".cooldownSeconds");
				rule.explosionRadius = ParseLevelFloat(entry, "explosionRadius", entryContext + ".explosionRadius", minimumLevelCount, rule.explosionRadiusByLevel);
				ValidateTable(rule.explosionRadiusByLevel, entryContext, [](float value) { return value > 0.f; }, "radius must be positive");
				ParseLevelFloats(entry, "primaryDamageMultiplier", entryContext + ".primaryDamageMultiplier", minimumLevelCount, rule.primaryDamageMultiplierByLevel);
				if (rule.phaseSeconds <= 0.f || rule.cooldownSeconds <= 0.f || rule.explosionRadius <= 0.f) throw std::runtime_error(entryContext + " needs positive phase, cooldown and radius");
				rules.phaseSlipRules.push_back(rule);
			}
		}
	}
}
