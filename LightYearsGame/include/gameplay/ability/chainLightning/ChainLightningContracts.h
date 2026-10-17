#pragma once

#include "framework/Core.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::ChainLightning
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Offense.ChainLightning.Basic";
	};

	inline const vega::GameplayTag CategoryTag{
		vega::GameplayTags::Ability::Offense
	};
	inline const vega::GameplayTag FamilyTag{
		vega::GameplayTags::Ability::Family::ChainLightning
	};

	struct State
	{
		inline static const vega::GameplayTag Active{
			vega::GameplayTags::State::Ability::ChainLightning::Active
		};
	};

	struct Event
	{
		inline static const vega::GameplayTag Started{
			vega::GameplayTags::Event::Ability::ChainLightning::Started
		};
		inline static const vega::GameplayTag LinkHit{
			vega::GameplayTags::Event::Ability::ChainLightning::LinkHit
		};
		inline static const vega::GameplayTag Ended{
			vega::GameplayTags::Event::Ability::ChainLightning::Ended
		};
	};

	struct Attribute
	{
		// Damage and the first target's range are shared ability concepts. They
		// reuse Common.* so future abilities can consume the same resolver path.
		inline static const sas::AttributeId Damage = vega::CommonAttributeIds::Damage;
		inline static const sas::AttributeId InitialTargetRange =
			vega::CommonAttributeIds::Range;

		// These values describe Chain Lightning's unique traversal rules, so they
		// stay under this ability family instead of expanding Common.* globally.
		inline static const sas::AttributeId BounceRange{
			"Ability.Offense.ChainLightning.BounceRange"
		};
		inline static const sas::AttributeId BaseBounceCount{
			"Ability.Offense.ChainLightning.BaseBounceCount"
		};
		inline static const sas::AttributeId LinkTravelTime{
			"Ability.Offense.ChainLightning.LinkTravelTime"
		};
		// These are ability-owned tuning values. They stay under the ability
		// family because the ability validator permits Common.* and Ability.*
		// family attributes only. Chain Lightning maps them into the shared
		// DamagePayload at runtime, where the common electric status pipeline
		// consumes the result.
		inline static const sas::AttributeId ElectricStacks{
			"Ability.Offense.ChainLightning.ElectricStacks"
		};
	};
}
