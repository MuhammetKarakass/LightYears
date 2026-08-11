#pragma once

#include "framework/Core.h"

namespace ly
{
	// Control classification belongs to the target, not to the ability that
	// applies a control effect. This keeps future abilities from branching on
	// concrete enemy classes such as LevelOneBoss.
	enum class ControlTargetClass
	{
		Normal,
		Elite,
		MiniBoss,
		Boss
	};

	enum class ControlResponseMode
	{
		Full,
		Reduced,
		InterruptOnly,
		Immune
	};

	struct ControlResponse
	{
		ControlResponseMode mode = ControlResponseMode::Full;
		float durationMultiplier = 1.f;
		float maximumInterruptDuration = 0.f;
		bool interruptionAllowed = true;
	};
}
