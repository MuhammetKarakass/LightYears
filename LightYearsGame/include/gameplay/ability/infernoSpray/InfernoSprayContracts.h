#pragma once

#include "framework/Core.h"
#include "gameplay/ability/content/NumericSettingContract.h"
#include "gameplay/tags/GameplayTagSchema.h"

namespace AbilityData::InfernoSpray
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.InfernoSpray.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTagSchema::AbilityOffense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.InfernoSpray" };
	inline const ly::GameplayTag FamilyTag{ "Ability.Offense.InfernoSpray" };

	struct State
	{
		inline static const ly::GameplayTag Active{ "State.Ability.InfernoSpray.Active" };
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{ "Event.Ability.InfernoSpray.Start" };
		inline static const ly::GameplayTag CancelAvailable{
			"Event.Ability.InfernoSpray.CancelAvailable"
		};
		inline static const ly::GameplayTag Cancelled{ "Event.Ability.InfernoSpray.Cancelled" };
		inline static const ly::GameplayTag Completed{ "Event.Ability.InfernoSpray.Completed" };
		inline static const ly::GameplayTag Ended{ "Event.Ability.InfernoSpray.End" };
	};

	struct Setting
	{
		inline static constexpr char MinCancelDuration[] = "minCancelDuration";

		inline static const ly::content::NumericSettingContract Contract{
			{ MinCancelDuration },
			{ MinCancelDuration }
		};
	};

	struct Actor
	{
		struct FlameCone
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.InfernoSpray.FlameCone.Basic";
			inline static const ly::GameplayTag TypeTag{ "AbilityActor.InfernoSpray.FlameCone" };
			inline static const ly::GameplayTag AttributeRoot{
				"Attribute.AbilityActor.InfernoSpray.FlameCone"
			};
			inline static const ly::GameplayTag Range{
				"Attribute.AbilityActor.InfernoSpray.FlameCone.Range"
			};
			inline static const ly::GameplayTag ConeAngle{
				"Attribute.AbilityActor.InfernoSpray.FlameCone.ConeAngle"
			};
			inline static const ly::GameplayTag CombatTickInterval{
				"Attribute.AbilityActor.InfernoSpray.FlameCone.CombatTickInterval"
			};
			inline static const ly::GameplayTag BaseDPS{
				"Attribute.AbilityActor.InfernoSpray.FlameCone.BaseDPS"
			};
		};
	};
}
