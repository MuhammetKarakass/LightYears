#pragma once

#include "framework/Core.h"
#include "gameplay/tags/GameplayTagSchema.h"

namespace AbilityData::Shield
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Defense.Shield.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTagSchema::AbilityDefense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.Shield" };
	inline const ly::GameplayTag FamilyTag{ "Ability.Defense.Shield" };
}
