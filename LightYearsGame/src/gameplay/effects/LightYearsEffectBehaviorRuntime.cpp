#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"

namespace vega
{
	LightYearsEffectBehaviorRuntime& GetEffectBehaviorRuntime()
	{
		static LightYearsEffectBehaviorRuntime runtime;
		return runtime;
	}
}
