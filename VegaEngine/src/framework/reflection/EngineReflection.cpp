#include "framework/reflection/Reflection.h"
#include "framework/Actor.h"
#include "framework/ActorComponent.h"
#include "framework/SpriteComponent.h"
#include "framework/scene/SceneEntityComponent.h"
#include "framework/LightComponent.h"
#include "framework/CollisionComponent.h"
#include "framework/SoundComponent.h"

namespace vega::reflect
{
	void RegisterEngineTypes() { RegisterEngineTypes(CurrentTypeRegistry()); }

	void RegisterEngineTypes(TypeRegistry& registry)
	{
		if (registry.mEngineTypesRegistered) return;
		VEGA_REGISTER_IN(Actor, registry);
		VEGA_REGISTER_IN(ActorComponent, registry);
		VEGA_REGISTER_IN(SpriteComponent, registry);
		VEGA_REGISTER_IN(SceneEntityComponent, registry);
		VEGA_REGISTER_IN(LightComponent, registry);
		VEGA_REGISTER_IN(CollisionComponent, registry);
		VEGA_REGISTER_IN(SoundComponent, registry);
		registry.mEngineTypesRegistered = true;
	}
}
