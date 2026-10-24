#pragma once

#include "attributes/AttributeSystem.h"
#include "framework/Core.h"
#include "gameplay/module/ModuleRulePredicates.h"
#include "gameplay/module/ModuleLevelTable.h"

#include <cstddef>

namespace vega
{
	using EpicLevelTable = ModuleLevelTable<float>;

	enum class EpicFamilyStatTarget
	{
		Owner,
		PrimaryWeapon,
		Ability
	};

	// Family-counted stat (Confluence Core, Momentum Drive). Each installed main module of `family` other than the placement
	// that owns the rule adds perModuleByLevel[level]. Add sums those values. Multiply uses 1 + sum, or 1 - sum when reduce is set.
	// Ability targets apply to abilities of `family`. doubleOnAbilityUse (Add, Owner only) also adds the same total as a trigger
	// buff for durationSeconds after an ability of `family` is used; a re-use refreshes that buff.
	struct EpicFamilyStatRule
	{
		GameplayTag family;
		EpicFamilyStatTarget target = EpicFamilyStatTarget::Owner;
		sas::AttributeId attributeId;
		sas::AttributeModifierOperation operation = sas::AttributeModifierOperation::Add;
		EpicLevelTable perModuleByLevel;
		bool reduce = false;
		bool doubleOnAbilityUse = false;
		float durationSeconds = 0.f;
	};
}
