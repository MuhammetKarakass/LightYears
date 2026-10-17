#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::ScorchDrive
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Movement.ScorchDrive.Basic";
	};

	inline const vega::GameplayTag CategoryTag{
		vega::GameplayTagSchema::AbilityMovement
	};
	inline const vega::GameplayTag FamilyTag{
		vega::GameplayTags::Ability::Family::ScorchDrive
	};

	struct Attribute
	{
		inline static const sas::AttributeId Damage = vega::CommonAttributeIds::Damage;
		inline static const sas::AttributeId SegmentWidth = vega::AreaAttributeIds::Width;
		inline static const sas::AttributeId SegmentLength = vega::AreaAttributeIds::Length;

		inline static const sas::AttributeId SegmentSpawnDistance{
			"Ability.Movement.ScorchDrive.SegmentSpawnDistance"
		};
		inline static const sas::AttributeId BaseSegmentLifetime{
			"Ability.Movement.ScorchDrive.BaseSegmentLifetime"
		};
		inline static const sas::AttributeId IgniteStacks{
			"Ability.Movement.ScorchDrive.IgniteStacks"
		};
	};

	struct Actor
	{
		struct FireSegment
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.ScorchDrive.FireSegment.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.ScorchDrive.FireSegment"
			};
		};
	};

	struct State
	{
		inline static const vega::GameplayTag Active =
			vega::GameplayTags::State::Ability::ScorchDrive::Active;
	};
}
