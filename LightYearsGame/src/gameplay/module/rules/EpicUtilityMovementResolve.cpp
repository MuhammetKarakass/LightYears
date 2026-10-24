#include "gameplay/module/ModuleResolver.h"

namespace vega
{
	namespace ModuleResolver
	{
		// Resolves the Epic Utility/Movement rules of one package (main or side) at the placement's effective level.
		// Side packages only carry Knowledge Transfer (Field Mastery); the other rules are main-package only.
		void ResolveEpicUtilityMovementRules(ResolvedModuleEffects& effects, const ModuleRulePackage& package, std::uint32_t instanceId, int level, bool sideFeature)
		{
			if (!sideFeature)
			{
				for (const ModuleCascadeRule& rule : package.cascadeRules)
					effects.epicRules.cascades.push_back({ instanceId, rule.stepBonusByLevel.ValueAtLevel(0.f, level), rule.maxSteps, rule.windowSeconds });
				for (const ModuleAssaultVectorRule& rule : package.assaultVectorRules)
					effects.epicRules.assaultVectors.push_back({ instanceId, rule.damageBonusByLevel.ValueAtLevel(0.f, level), rule.armorBonusByLevel.ValueAtLevel(0.f, level), rule.targetRange, rule.minimumCosine, rule.minimumSpeed });
				for (const ModulePhaseSlipRule& rule : package.phaseSlipRules)
					effects.epicRules.phaseSlips.push_back({ instanceId, rule.phaseSeconds, rule.cooldownSeconds, rule.explosionRadiusByLevel.ValueAtLevel(rule.explosionRadius, level), rule.primaryDamageMultiplierByLevel.ValueAtLevel(1.f, level) });
			}
			for (const ModuleFieldMasteryRule& rule : package.fieldMasteryRules)
				effects.epicRules.fieldMasteries.push_back({ instanceId, rule.usesPerLevelUpByLevel.ValueAtLevel(0, level), rule.transferUses });
		}
	}
}
