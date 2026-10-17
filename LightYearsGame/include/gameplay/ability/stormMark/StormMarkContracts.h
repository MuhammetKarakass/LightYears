#pragma once

#include "framework/Core.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::StormMark
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Offense.StormMark.Basic";
	};

	inline const vega::GameplayTag CategoryTag{
		vega::GameplayTags::Ability::Offense
	};
	inline const vega::GameplayTag FamilyTag{
		vega::GameplayTags::Ability::Family::StormMark
	};

	struct Attribute
	{
		// Damage and search radius are shared ability concepts. Storm Mark's
		// family-specific values describe only its target count/timing rules.
		inline static const sas::AttributeId Damage =
			vega::CommonAttributeIds::Damage;
		inline static const sas::AttributeId SearchRadius =
			vega::CommonAttributeIds::Range;
		inline static const sas::AttributeId BaseTargetCount{
			"Ability.Offense.StormMark.BaseTargetCount"
		};
		inline static const sas::AttributeId FocusDuration{
			"Ability.Offense.StormMark.FocusDuration"
		};
		inline static const sas::AttributeId StrikeDuration{
			"Ability.Offense.StormMark.StrikeDuration"
		};
		inline static const sas::AttributeId LuckPerExtraTarget{
			"Ability.Offense.StormMark.LuckPerExtraTarget"
		};

		// These values are Storm Mark's ability-owned tuning inputs. The behavior
		// maps them to the shared DamagePayload at the combat boundary, because
		// ability-scoped attributes must stay in Common.* or this family namespace.
		inline static const sas::AttributeId ElectricStacks{
			"Ability.Offense.StormMark.ElectricStacks"
		};
	};
}
