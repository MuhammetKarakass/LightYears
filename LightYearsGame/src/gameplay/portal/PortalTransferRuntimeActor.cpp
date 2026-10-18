#include "gameplay/portal/PortalTransferRuntimeActor.h"

#include "gameplay/portal/PortalTransferService.h"
#include "PortalTransferRuntimeState.h"

namespace vega
{
	PortalTransferRuntimeActor::PortalTransferRuntimeActor(World* world)
		: Actor(world),
		mRuntimeState(std::make_unique<PortalTransferRuntimeState>())
	{
		SetCollisionLayer(vega::CollisionLayer::None);
		SetCollisionMask(vega::CollisionLayer::None);
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
