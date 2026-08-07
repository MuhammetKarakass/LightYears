#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"

namespace ly
{
	LightYearsEffectBehaviorRuntime& GetEffectBehaviorRuntime()
	{
		static LightYearsEffectBehaviorRuntime runtime;
		return runtime;
	}
}
