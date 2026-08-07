#pragma once

#include "framework/Core.h"
#include "gameplay/ability/content/NumericSettingContract.h"
#include "gameplay/tags/GameplayTagSchema.h"

namespace AbilityData::Dash
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Movement.Dash.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTagSchema::AbilityMovement };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.Dash" };
	inline const ly::GameplayTag FamilyTag{ "Ability.Movement.Dash" };

	struct State
	{
		inline static const ly::GameplayTag Active{ "State.Ability.Dash.Active" };
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{ "Event.Ability.Dash.Start" };
		inline static const ly::GameplayTag Ended{ "Event.Ability.Dash.End" };
	};

	struct Setting
	{
		inline static constexpr char BaseDistance[] = "baseDistance";
		inline static constexpr char CameraZoomOutRatio[] = "cameraZoomOutRatio";

		inline static const ly::content::NumericSettingContract Contract{
			{ BaseDistance, CameraZoomOutRatio },
			{ BaseDistance, CameraZoomOutRatio }
		};
	};

	enum class DirectionPolicy
	{
		MovementInputOrMouseWorld
	};
}
