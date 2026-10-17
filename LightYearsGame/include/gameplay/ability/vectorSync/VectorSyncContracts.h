#pragma once

#include "framework/Core.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::VectorSync
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Movement.VectorSync.Basic";
	};

	inline const vega::GameplayTag CategoryTag{ vega::GameplayTags::Ability::Movement };
	inline const vega::GameplayTag FamilyTag{ vega::GameplayTags::Ability::Family::VectorSync };

	struct State
	{
		inline static const vega::GameplayTag Active{
			vega::GameplayTags::State::Ability::VectorSync::Active
		};
	};
}
