#pragma once

#include "framework/Core.h"
#include "gameplay/tags/GameplayTagSchema.h"

namespace AbilityData::SunBeam
{
	struct AbilityId
	{
		struct Strike
		{
			inline static constexpr char Basic[] = "Ability.Offense.SunBeam.Strike.Basic";
		};
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTagSchema::AbilityOffense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.SunBeam" };
	inline const ly::GameplayTag FamilyTag{ "Ability.Offense.SunBeam" };

	struct Actor
	{
		struct Shared
		{
			inline static const ly::GameplayTag AttributeRoot{
				"Attribute.AbilityActor.SunBeam.Shared"
			};
			inline static const ly::GameplayTag Width{
				"Attribute.AbilityActor.SunBeam.Shared.Width"
			};
			inline static const ly::GameplayTag Length{
				"Attribute.AbilityActor.SunBeam.Shared.Length"
			};
		};

		struct Strike
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.SunBeam.Strike.Basic";
			inline static const ly::GameplayTag TypeTag{ "AbilityActor.SunBeam.Strike" };
			inline static const ly::GameplayTag AttributeRoot{
				"Attribute.AbilityActor.SunBeam.Strike"
			};
			inline static const ly::GameplayTag TelegraphDuration{
				"Attribute.AbilityActor.SunBeam.Strike.TelegraphDuration"
			};
			inline static const ly::GameplayTag ArrivalDuration{
				"Attribute.AbilityActor.SunBeam.Strike.ArrivalDuration"
			};
			inline static const ly::GameplayTag ImpactDelay{
				"Attribute.AbilityActor.SunBeam.Strike.ImpactDelay"
			};
			inline static const ly::GameplayTag ImpactVisualDuration{
				"Attribute.AbilityActor.SunBeam.Strike.ImpactVisualDuration"
			};
		};
	};
}
