#pragma once

#include "framework/Core.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::VectorSync
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Movement.VectorSync.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Movement };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.VectorSync" };
	inline const ly::GameplayTag FamilyTag{ ly::GameplayTags::Ability::Family::VectorSync };

	struct State
	{
		inline static const ly::GameplayTag Active{
			ly::GameplayTags::State::Ability::VectorSync::Active
		};
	};
}
