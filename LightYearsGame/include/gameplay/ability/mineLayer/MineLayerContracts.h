#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::MineLayer
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.MineLayer.Basic";
	};

	inline const ly::GameplayTag CategoryTag = ly::GameplayTags::Ability::Offense;
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.MineLayer" };
	inline const ly::GameplayTag FamilyTag = ly::GameplayTags::Ability::Family::MineLayer;

	struct Actor
	{
		struct Mine
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.MineLayer.Mine.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.MineLayer.Mine"
			};
			inline static const sas::AttributeId TriggerRadius{
				"AbilityActor.MineLayer.Mine.TriggerRadius"
			};
			// Deployment is an explicit projectile-like flight. This speed governs
			// only the short trip from the ship to the armed mine position.
			inline static const sas::AttributeId LaunchSpeed{
				"AbilityActor.MineLayer.Mine.LaunchSpeed"
			};
			inline static const sas::AttributeId StunDuration{
				"AbilityActor.MineLayer.Mine.StunDuration"
			};
			inline static const sas::AttributeId KnockbackStrength{
				"AbilityActor.MineLayer.Mine.KnockbackStrength"
			};
		};
	};

	struct Attribute
	{
		// The guaranteed count is ability-owned because it controls how many
		// actors one activation creates, rather than an individual mine value.
		inline static const sas::AttributeId BaseMineCount{
			"Ability.Offense.MineLayer.BaseMineCount"
		};
		inline static const sas::AttributeId MineSpacing{
			"Ability.Offense.MineLayer.MineSpacing"
		};
		// Luck is a rating. This coefficient converts one point of the owner's
		// Luck rating into the bonus-mine roll value; the fractional remainder is
		// resolved as one additional proc, so there is no gameplay hard cap.
		inline static const sas::AttributeId LuckToBonusMineScale{
			"Ability.Offense.MineLayer.LuckToBonusMineScale"
		};
	};

	struct Effect
	{
		// Mine Layer reuses the project-wide control effect instead of creating a
		// second stun definition with identical semantics.
		inline static constexpr char StunId[] = "Effect.Control.Stun.Basic";
	};
}
