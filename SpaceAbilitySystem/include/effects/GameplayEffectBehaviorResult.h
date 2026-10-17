#pragma once

#include "framework/Core.h"

namespace sas
{
	struct GameplayEffectBehaviorEvent
	{
		vega::GameplayTag eventTag;
		float magnitude = 0.f;
	};

	struct GameplayEffectBehaviorResult
	{
		bool changed = false;
		bool removeEffect = false;
		vega::List<GameplayEffectBehaviorEvent> events;
	};
}
