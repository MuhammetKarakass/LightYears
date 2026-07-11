#pragma once

#include "framework/Core.h"

namespace ly
{
	class Actor;
	struct DamageContext;

	struct AbilityEvent
	{
		GameplayTag eventTag;
		Actor* source = nullptr;
		Actor* target = nullptr;
		float magnitude = 0.f;
		const DamageContext* damageContext = nullptr;
	};
}


