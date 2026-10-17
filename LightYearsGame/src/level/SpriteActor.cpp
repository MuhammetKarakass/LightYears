#include "level/SpriteActor.h"
#include <framework/reflection/Reflection.h>

namespace vega
{
	VEGA_REFLECT(SpriteActor, Actor)
	{
		VEGA_SPAWNABLE();
	}
}
