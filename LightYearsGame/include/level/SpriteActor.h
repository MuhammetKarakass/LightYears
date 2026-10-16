#pragma once

#include <framework/Actor.h>
#include <framework/SpriteComponent.h>

namespace ly
{
	class SpriteActor : public Actor
	{
		VEGA_CLASS(SpriteActor)
	public:
		explicit SpriteActor(World* world) : Actor{ world }
		{
			AddComponent<SpriteComponent>();
		}
	};
}
