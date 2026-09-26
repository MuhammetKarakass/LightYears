#include "gameplay/portal/PortalTransferRuntimeActor.h"

#include "gameplay/portal/PortalTransferService.h"
#include "PortalTransferRuntimeState.h"

namespace ly
{
	PortalTransferRuntimeActor::PortalTransferRuntimeActor(World* world)
		: Actor(world),
		mRuntimeState(std::make_unique<PortalTransferRuntimeState>())
	{
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetTickWhenPaused(true);
	}

	PortalTransferRuntimeActor::~PortalTransferRuntimeActor() = default;

	void PortalTransferRuntimeActor::Tick(float deltaTime)
	{
		if (World* world = GetWorld())
		{
			PortalTransferService::Tick(*world, deltaTime);
		}
		Actor::Tick(deltaTime);
	}

	void PortalTransferRuntimeActor::Destroy()
	{
		Actor::Destroy();
		mRuntimeState.reset();
	}
}
