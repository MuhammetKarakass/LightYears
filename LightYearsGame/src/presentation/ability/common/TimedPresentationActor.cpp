#include "presentation/ability/common/TimedPresentationActor.h"

#include <algorithm>

namespace vega
{
	TimedPresentationActor::TimedPresentationActor(World* world, const sf::Vector2f& worldLocation, float duration, RenderLayer renderLayer)
		: Actor(world),
		mDuration(std::max(0.f, duration))
	{
		SetActorLocation(worldLocation);
		SetRenderLayer(renderLayer);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetEnablePhysics(false);
	}

	void TimedPresentationActor::Tick(float deltaTime)
	{
		mAge += std::max(0.f, deltaTime);
		if (mAge >= mDuration)
		{
			Destroy();
			return;
		}

		UpdateVisuals();
		Actor::Tick(deltaTime);
	}
}
