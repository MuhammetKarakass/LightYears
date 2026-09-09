#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::SeismicCharge
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.SeismicCharge.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Offense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.SeismicCharge" };
	inline const ly::GameplayTag FamilyTag{ ly::GameplayTags::Ability::Family::SeismicCharge };

	struct Actor
	{
		struct Bomb
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.SeismicCharge.Bomb.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.SeismicCharge.Bomb"
			};
		};
	};

	struct Attribute
	{
		// Damage and maximum shockwave reach use common, cross-family channels.
		inline static const sas::AttributeId Damage = ly::CommonAttributeIds::Damage;
		inline static const sas::AttributeId MaximumRadius = ly::CommonAttributeIds::Range;

		// These are delivery-specific timeline values, so they stay beneath the
		// Seismic Charge bomb actor rather than expanding CommonAttributeIds.
		inline static const sas::AttributeId DropOffset{
			"AbilityActor.SeismicCharge.Bomb.DropOffset"
		};
		inline static const sas::AttributeId DeploymentDuration{
			"AbilityActor.SeismicCharge.Bomb.DeploymentDuration"
		};
		inline static const sas::AttributeId FuseDuration{
			"AbilityActor.SeismicCharge.Bomb.FuseDuration"
		};
		inline static const sas::AttributeId ShockwaveDuration{
			"AbilityActor.SeismicCharge.Bomb.ShockwaveDuration"
		};
		inline static const sas::AttributeId ShockwaveThickness{
			"AbilityActor.SeismicCharge.Bomb.ShockwaveThickness"
		};
	};
}
