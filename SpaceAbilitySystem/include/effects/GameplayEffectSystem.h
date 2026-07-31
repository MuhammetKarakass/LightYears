#pragma once

#include "effects/ActiveGameplayEffect.h"
#include "effects/GameplayEffectRuntimeSystem.h"
#include "effects/GameplayEffectSpec.h"

namespace sas
{
	using GameplayEffectSystem =
		GameplayEffectRuntimeSystem<GameplayEffectSpec, ActiveGameplayEffect>;
}
