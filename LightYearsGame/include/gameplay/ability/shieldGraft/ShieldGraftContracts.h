#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::ShieldGraft
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Defense.ShieldGraft.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Defense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.ShieldGraft" };
	inline const ly::GameplayTag FamilyTag{ ly::GameplayTags::Ability::Family::ShieldGraft };

	struct Attribute
	{
		inline static const sas::AttributeId ConversionRatio{
			"Ability.Defense.ShieldGraft.ConversionRatio"
		};
		inline static const sas::AttributeId EnergyPowerReference{
			"Ability.Defense.ShieldGraft.EnergyPowerReference"
		};
		inline static const sas::AttributeId EnergyPowerScale{
			"Ability.Defense.ShieldGraft.EnergyPowerScale"
		};
	};
}
