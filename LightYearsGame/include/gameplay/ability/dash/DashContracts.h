#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/content/NumericSettingContract.h"

namespace AbilityData::Dash
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Movement.Dash.Basic";
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

	struct Attribute
	{
		inline static const sas::AttributeId MoveSpeedScale{
			"Ability.Movement.Dash.MoveSpeedScale"
		};
	};

	inline constexpr float DefaultMoveSpeedScale = 0.80f;
	inline constexpr float MoveSpeedScaleUpgrade = 0.10f;

	enum class DirectionPolicy
	{
		MovementInputOrMouseWorld
	};
}
