#pragma once

#include "attributes/AttributeSystem.h"
#include "framework/Core.h"
#include "gameplay/module/ModuleRulePredicates.h"

#include <cstddef>

namespace vega
{
	// Level table of the Epic stat rules. Same contract as ModuleLevelTable: levels past the authored entries extrapolate
	// linearly from the last step. Kept separate because ModuleDefinition.h includes this header before its own tables.
	struct EpicLevelTable
	{
		List<float> values;

		float ValueAtLevel(float fallback, int level) const
		{
			if (values.empty() || level <= 0) return fallback;
			const int count = static_cast<int>(values.size());
			if (level <= count) return values[static_cast<std::size_t>(level - 1)];
			if (count == 1) return values.front();
			const float step = values.back() - values[static_cast<std::size_t>(count - 2)];
			return values.back() + step * static_cast<float>(level - count);
		}
	};

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
