#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::TemporalConvergence
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Defense.TemporalConvergence.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Defense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.TemporalConvergence" };
	inline const ly::GameplayTag FamilyTag{
		ly::GameplayTags::Ability::Family::TemporalConvergence
	};

	struct Actor
	{
		struct Field
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.TemporalConvergence.Field.Basic";
		};
	};

	struct Effect
	{
		// Both effects are reusable project-wide control mechanics. This ability
		// supplies only its own magnitude and duration at application time.
		inline static constexpr char SlowId[] = "Effect.Movement.Slow.Basic";
		inline static constexpr char StunId[] = "Effect.Control.Stun.Basic";
	};

	struct Attribute
	{
		// Spatial vocabulary is shared: Range is the cursor clamp and Radius is
		// the field's influence circle. The rest describe this family's timeline
		// and shield policy, so they remain ability-local instead of bloating
		// CommonAttributeIds.
		inline static const sas::AttributeId CastRange = ly::CommonAttributeIds::Range;
		inline static const sas::AttributeId Radius = ly::CommonAttributeIds::Radius;
		inline static const sas::AttributeId BaseShield{
			"Ability.Defense.TemporalConvergence.BaseShield"
		};
		inline static const sas::AttributeId InitialDelay{
			"Ability.Defense.TemporalConvergence.InitialDelay"
		};
		inline static const sas::AttributeId TravelSpeed{
			"Ability.Defense.TemporalConvergence.TravelSpeed"
		};
		inline static const sas::AttributeId FieldDuration{
			"Ability.Defense.TemporalConvergence.FieldDuration"
		};
		inline static const sas::AttributeId SlowFraction{
			"Ability.Defense.TemporalConvergence.SlowFraction"
		};
		inline static const sas::AttributeId StunDuration{
			"Ability.Defense.TemporalConvergence.StunDuration"
		};
		inline static const sas::AttributeId OvershieldHoldDuration{
			"Ability.Defense.TemporalConvergence.OvershieldHoldDuration"
		};
		inline static const sas::AttributeId OvershieldDecayPerSecond{
			"Ability.Defense.TemporalConvergence.OvershieldDecayPerSecond"
		};
	};
}
