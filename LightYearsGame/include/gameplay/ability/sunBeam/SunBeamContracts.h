#pragma once

#include "framework/Core.h"

namespace AbilityData::SunBeam
{
	inline const ly::GameplayTag BehaviorId{ "GameAbilityBehavior.SunBeam" };
	inline const ly::GameplayTag FamilyTag{ "Ability.Offense.SunBeam" };

	struct ActorSchema
	{
		inline static const ly::GameplayTag SharedAttributeRoot{
			"Attribute.AbilityActor.SunBeam.Shared"
		};
		inline static const ly::GameplayTag Width{
			"Attribute.AbilityActor.SunBeam.Shared.Width"
		};
		inline static const ly::GameplayTag Length{
			"Attribute.AbilityActor.SunBeam.Shared.Length"
		};

		struct Strike
		{
			inline static const ly::GameplayTag TypeId{ "AbilityActor.SunBeam.Strike" };
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
