#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::StrikeRun
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Offense.StrikeRun.Basic";
	};

	inline const vega::GameplayTag CategoryTag{
		vega::GameplayTagSchema::AbilityOffense
	};
	inline const vega::GameplayTag FamilyTag{
		vega::GameplayTags::Ability::Family::StrikeRun
	};

	struct State
	{
		inline static const vega::GameplayTag Targeting{
			"State.Ability.StrikeRun.Targeting"
		};
		inline static const vega::GameplayTag Confirmed{
			"State.Ability.StrikeRun.Confirmed"
		};
	};

	struct Event
	{
		inline static const vega::GameplayTag Started{
			"Event.Ability.StrikeRun.Start"
		};
		inline static const vega::GameplayTag DirectionLocked{
			"Event.Ability.StrikeRun.DirectionLocked"
		};
		inline static const vega::GameplayTag Ended{
			"Event.Ability.StrikeRun.End"
		};
	};

	struct Attribute
	{
		// Damage, radius and range remain shared channels. Strike Run's range is
		// the maximum distance of the selected starting endpoint. The impact line
		// begins at that endpoint and extends in the confirmed direction.
		inline static const sas::AttributeId Damage = vega::CommonAttributeIds::Damage;
		inline static const sas::AttributeId ExplosionRadius = vega::CommonAttributeIds::Radius;
		inline static const sas::AttributeId CenterRange = vega::CommonAttributeIds::Range;

		// These values describe this family's fixed multi-impact delivery and are
		// intentionally not added to the global projectile attribute namespace.
		inline static const sas::AttributeId ImpactCount{
			"AbilityActor.StrikeRun.Bombardment.ImpactCount"
		};
		inline static const sas::AttributeId ImpactSpan{
			"AbilityActor.StrikeRun.Bombardment.ImpactSpan"
		};
		inline static const sas::AttributeId TargetingWindow{
			"AbilityActor.StrikeRun.Bombardment.TargetingWindow"
		};
		inline static const sas::AttributeId FinalTelegraphDuration{
			"AbilityActor.StrikeRun.Bombardment.FinalTelegraphDuration"
		};
		inline static const sas::AttributeId ImpactDelay{
			"AbilityActor.StrikeRun.Bombardment.ImpactDelay"
		};
	};

	struct Actor
	{
		struct Bombardment
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.StrikeRun.Bombardment.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.StrikeRun.Bombardment"
			};
		};
	};
}
