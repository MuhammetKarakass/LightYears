#pragma once

#include "attributes/AttributeSystem.h"

#include <string>

namespace sas
{
	struct GameplayEffectRuntimeSnapshot
	{
		std::string effectId;
		float remainingDuration = 0.f;
		float totalDuration = 0.f;
		int stackCount = 1;
		GameplayAttributeList runtimeAttributes;
	};
}
