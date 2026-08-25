#include "gameplay/portal/PortalTransferRuntimeActor.h"

#include "gameplay/portal/PortalTransferService.h"

namespace ly
{
	PortalTransferRuntimeActor::~PortalTransferRuntimeActor()
	{
		if (World* world = GetWorld())
		{
			PortalTransferService::ResetWorld(*world);
		}
	}

	void PortalTransferRuntimeActor::Tick(float deltaTime)
	{
		if (World* world = GetWorld())
		{
			PortalTransferService::Tick(*world, deltaTime);
		}
		Actor::Tick(deltaTime);
	}
}
