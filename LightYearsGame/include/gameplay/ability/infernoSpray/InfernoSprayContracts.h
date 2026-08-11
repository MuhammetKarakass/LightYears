#pragma once

#include "attributes/AttributeId.h"
#include "framework/Core.h"
#include "gameplay/ability/content/NumericSettingContract.h"

namespace AbilityData::InfernoSpray
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.InfernoSpray.Basic";
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
			inline static const sas::AttributeId Root{
				"AbilityActor.InfernoSpray.FlameCone"
			};
			inline static const sas::AttributeId Range{
				"AbilityActor.InfernoSpray.FlameCone.Range"
			};
			inline static const sas::AttributeId ConeAngle{
				"AbilityActor.InfernoSpray.FlameCone.ConeAngle"
			};
			inline static const sas::AttributeId CombatTickInterval{
				"AbilityActor.InfernoSpray.FlameCone.CombatTickInterval"
			};
			inline static const sas::AttributeId BaseDPS{
				"AbilityActor.InfernoSpray.FlameCone.BaseDPS"
			};
		};
	};
}
