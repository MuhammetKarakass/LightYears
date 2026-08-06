#pragma once

#include "framework/Core.h"

namespace AbilityData::InfernoSpray
{
	inline const ly::GameplayTag BehaviorId{ "GameAbilityBehavior.InfernoSpray" };
	inline const ly::GameplayTag FamilyTag{ "Ability.Offense.InfernoSpray" };
	inline const ly::GameplayTag StateTag{ "State.Ability.InfernoSpray.Active" };

	inline const ly::GameplayTag StartEvent{ "Event.Ability.InfernoSpray.Start" };
	inline const ly::GameplayTag CancelAvailableEvent{ "Event.Ability.InfernoSpray.CancelAvailable" };
	inline const ly::GameplayTag CancelledEvent{ "Event.Ability.InfernoSpray.Cancelled" };
	inline const ly::GameplayTag CompletedEvent{ "Event.Ability.InfernoSpray.Completed" };
	inline const ly::GameplayTag EndEvent{ "Event.Ability.InfernoSpray.End" };

	inline const std::string MovementPenaltyEffectId = "Effect.Movement.Slow";

	struct ActorSchema
	{
		inline static const ly::GameplayTag TypeId{ "AbilityActor.InfernoSpray.FlameCone" };
		inline static const ly::GameplayTag AttributeRoot{ "Attribute.AbilityActor.InfernoSpray" };
		inline static const ly::GameplayTag Range{ "Attribute.AbilityActor.InfernoSpray.Range" };
		inline static const ly::GameplayTag ConeAngle{ "Attribute.AbilityActor.InfernoSpray.ConeAngle" };
		inline static const ly::GameplayTag CombatTickInterval{
			"Attribute.AbilityActor.InfernoSpray.CombatTickInterval"
		};
		inline static const ly::GameplayTag BaseDPS{ "Attribute.AbilityActor.InfernoSpray.BaseDPS" };
	};
}
