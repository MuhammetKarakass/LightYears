#pragma once

#include "framework/Actor.h"

namespace ly
{
	// One coordinator per World keeps portal transit ticking even after the
	// ability that created a portal pair has entered cooldown and destroyed its
	// visual portal actors.
	class PortalTransferRuntimeActor final : public Actor
	{
	public:
		explicit PortalTransferRuntimeActor(World* world)
			: Actor(world)
		{
			SetCollisionLayer(CollisionLayer::None);
			SetCollisionMask(CollisionLayer::None);
			SetTickWhenPaused(true);
		}
		~PortalTransferRuntimeActor() override;

		void Tick(float deltaTime) override;
	};
}
