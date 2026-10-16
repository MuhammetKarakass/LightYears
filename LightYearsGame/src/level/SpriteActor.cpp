#include "level/SpriteActor.h"
#include <framework/reflection/Reflection.h>

namespace ly
{
	VEGA_REFLECT(SpriteActor, Actor)
	{
		VEGA_SPAWNABLE();
	}
}
