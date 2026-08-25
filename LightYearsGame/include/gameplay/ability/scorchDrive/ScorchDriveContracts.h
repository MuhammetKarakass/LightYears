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
			"Ability.Offense.ScorchDrive.Basic";
	};

	inline const ly::GameplayTag CategoryTag{
		ly::GameplayTagSchema::AbilityOffense
	};
	inline const ly::GameplayTag BehaviorTag{
		"GameAbilityBehavior.ScorchDrive"
	};
	inline const ly::GameplayTag FamilyTag{
		ly::GameplayTags::Ability::Family::ScorchDrive
	};

	struct Attribute
	{
		inline static const sas::AttributeId Damage = ly::CommonAttributeIds::Damage;
		inline static const sas::AttributeId SegmentWidth = ly::AreaAttributeIds::Width;
		inline static const sas::AttributeId SegmentLength = ly::AreaAttributeIds::Length;

		inline static const sas::AttributeId SegmentSpawnDistance{
			"Ability.Offense.ScorchDrive.SegmentSpawnDistance"
		};
		inline static const sas::AttributeId BaseSegmentLifetime{
			"Ability.Offense.ScorchDrive.BaseSegmentLifetime"
		};
		inline static const sas::AttributeId FireTickInterval{
			"Ability.Offense.ScorchDrive.FireTickInterval"
		};
		inline static const sas::AttributeId BurnThresholdTicks{
			"Ability.Offense.ScorchDrive.BurnThresholdTicks"
		};
		inline static const sas::AttributeId BurnDuration{
			"Ability.Offense.ScorchDrive.BurnDuration"
		};
		inline static const sas::AttributeId BurnTickInterval{
			"Ability.Offense.ScorchDrive.BurnTickInterval"
		};
		inline static const sas::AttributeId BurnDamageRatio{
			"Ability.Offense.ScorchDrive.BurnDamageRatio"
		};
		inline static const sas::AttributeId ReferenceMaxHealth{
			"Ability.Offense.ScorchDrive.ReferenceMaxHealth"
		};
		inline static const sas::AttributeId MaxHealthLifetimeScale{
			"Ability.Offense.ScorchDrive.MaxHealthLifetimeScale"
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
		inline static const ly::GameplayTag Active =
			ly::GameplayTags::State::Ability::ScorchDrive::Active;
	};
}
