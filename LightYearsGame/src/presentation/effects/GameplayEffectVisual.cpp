#include "presentation/effects/GameplayEffectVisual.h"

namespace ly
{
	GameplayEffectVisual::GameplayEffectVisual(
		World* world,
		Actor* owner,
		const std::string& texturePath
	)
		: Actor(world, texturePath),
		mOwner(owner)
	{
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void GameplayEffectVisual::Tick(float deltaTime)
	{
		if (!mOwner || mOwner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}

		TickVisual(deltaTime);
		Actor::Tick(deltaTime);
	}
}
