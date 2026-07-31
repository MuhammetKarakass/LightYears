#pragma once

#include "effects/GameplayEffectRuntimeEntry.h"
#include "effects/GameplayEffectSpec.h"

namespace sas
{
	using ActiveGameplayEffect =
		GameplayEffectRuntimeEntry<GameplayEffectSpec>;
}
