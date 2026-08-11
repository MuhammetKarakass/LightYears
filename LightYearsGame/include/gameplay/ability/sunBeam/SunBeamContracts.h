#pragma once

#include "attributes/AttributeId.h"
#include "framework/Core.h"

namespace AbilityData::SunBeam
{
	struct AbilityId
	{
		struct Strike
		{
			inline static constexpr char Basic[] = "Ability.Offense.SunBeam.Strike.Basic";
		};
	};

	struct Actor
	{
		struct Shared
		{
			inline static const sas::AttributeId Root{
				"AbilityActor.SunBeam.Shared"
			};
			inline static const sas::AttributeId Width{
				"AbilityActor.SunBeam.Shared.Width"
			};
			inline static const sas::AttributeId Length{
				"AbilityActor.SunBeam.Shared.Length"
			};
		};

		struct Strike
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.SunBeam.Strike.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.SunBeam.Strike"
			};
			inline static const sas::AttributeId TelegraphDuration{
				"AbilityActor.SunBeam.Strike.TelegraphDuration"
			};
			inline static const sas::AttributeId ArrivalDuration{
				"AbilityActor.SunBeam.Strike.ArrivalDuration"
			};
			inline static const sas::AttributeId ImpactDelay{
				"AbilityActor.SunBeam.Strike.ImpactDelay"
			};
			inline static const sas::AttributeId ImpactVisualDuration{
				"AbilityActor.SunBeam.Strike.ImpactVisualDuration"
			};
		};
	};
}
