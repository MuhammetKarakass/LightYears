#include "presentation/effects/GameplayEffectVisual.h"

namespace vega
{
	GameplayEffectVisual::GameplayEffectVisual(
		World* world,
		Actor* owner,
		const std::string& texturePath
	)
		: Actor(world, texturePath),
		mOwner(owner
			? std::static_pointer_cast<Actor>(owner->GetWeakPtr().lock())
			: shared_ptr<Actor>{})
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		SetCollisionLayer(vega::CollisionLayer::None);
		SetCollisionMask(vega::CollisionLayer::None);
	}

	void GameplayEffectVisual::Tick(float deltaTime)
	{
		const shared_ptr<Actor> owner = GetVisualOwner();
		if (!owner || owner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}

		TickVisual(deltaTime);
		Actor::Tick(deltaTime);
	}
}
