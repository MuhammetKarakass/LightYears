#pragma once

#include "framework/Core.h"

namespace AbilityData::Dash
{
	inline const ly::GameplayTag BehaviorId{ "GameAbilityBehavior.Dash" };
	inline const ly::GameplayTag FamilyTag{ "Ability.Movement.Dash" };
	inline const ly::GameplayTag StartEvent{ "Event.Ability.Dash.Start" };
	inline const ly::GameplayTag EndEvent{ "Event.Ability.Dash.End" };
	inline const ly::GameplayTag StateTag{ "State.Ability.Dashing" };

	enum class DirectionPolicy
	{
		MovementInputOrMouseWorld
	};
}
