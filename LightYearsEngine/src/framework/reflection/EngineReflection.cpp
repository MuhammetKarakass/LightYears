#include "framework/reflection/Reflection.h"
#include "framework/Actor.h"
#include "framework/ActorComponent.h"
#include "framework/SpriteComponent.h"
#include "framework/scene/SceneEntityComponent.h"
#include "framework/LightComponent.h"
#include "framework/CollisionComponent.h"
#include "framework/SoundComponent.h"

namespace ly::reflect
{
	void RegisterEngineTypes()
	{
		static bool registered = false;
		if (registered) return;
		registered = true;
		VEGA_REGISTER(Actor);
		VEGA_REGISTER(ActorComponent);
		VEGA_REGISTER(SpriteComponent);
		VEGA_REGISTER(SceneEntityComponent);
		VEGA_REGISTER(LightComponent);
		VEGA_REGISTER(CollisionComponent);
		VEGA_REGISTER(SoundComponent);
	}
}
