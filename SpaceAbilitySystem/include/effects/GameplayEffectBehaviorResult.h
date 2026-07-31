#pragma once

#include "framework/Core.h"

namespace sas
{
	struct GameplayEffectBehaviorEvent
	{
		ly::GameplayTag eventTag;
		float magnitude = 0.f;
	};

	struct GameplayEffectBehaviorResult
	{
		bool changed = false;
		bool removeEffect = false;
		ly::List<GameplayEffectBehaviorEvent> events;
	};
}
