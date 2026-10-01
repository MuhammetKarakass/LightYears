#pragma once

#include "framework/Core.h"
#include "gameplay/ability/ScopedAbilityRule.h"
#include "gameplay/module/ModuleLoadout.h"

namespace ly
{
	struct ResolvedModuleEffects
	{
		List<sas::AttributeModifier> ownerModifiers;
		List<sas::AttributeModifier> shipModifiers;
		List<sas::AttributeModifier> primaryWeaponModifiers;
		// requiredAbilityTags = { module family }, one rule per placement.
		List<ScopedAbilityRule> abilityRules;
		List<GameplayTag> enabledSystems;
	};

	namespace ModuleResolver
	{
		// Bonus for one placement: the zone bonus when every covered cell lies in
		// one zone that rewards the module's family (or any family), else 0.
		float GetZoneBonus(const ModuleRulesContext& rules, const ModuleDefinition& module, const ModulePlacement& placement);

		// Output follows the order of activePlacements.
		ResolvedModuleEffects Resolve(const ModuleRulesContext& rules, const List<ModulePlacement>& activePlacements);
	}
}
