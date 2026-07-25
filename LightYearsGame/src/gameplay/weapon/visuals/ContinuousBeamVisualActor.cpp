#include "gameplay/weapon/visuals/ContinuousBeamVisualActor.h"

#include "framework/MathUtility.h"

#include <algorithm>

namespace ly
{
	ContinuousBeamVisualActor::ContinuousBeamVisualActor(World* world, Actor* owner)
		: Actor(world)
		, mOwner(owner)
	{
		SetRenderLayer(RenderLayer::Projectile);
		if (!mOwner)
		{
			return;
		}

		switch (mOwner->GetCollisionLayer())
		{
		case CollisionLayer::Player:
			SetCollisionLayer(CollisionLayer::PlayerBullet);
			SetCollisionMask(CollisionLayer::Enemy | CollisionLayer::EnemyBullet);
			break;
		case CollisionLayer::Enemy:
			SetCollisionLayer(CollisionLayer::EnemyBullet);
			SetCollisionMask(CollisionLayer::Player | CollisionLayer::PlayerBullet);
			break;
		default:
			break;
		}
	}

	void ContinuousBeamVisualActor::Tick(float)
	{
		if (!mOwner || mOwner->GetIsPendingDestroy())
		{
			Destroy();
		}
	}

	void ContinuousBeamVisualActor::Render(sf::RenderWindow& window)
	{
		Actor::Render(window);
		window.draw(mOuterBeam);
		window.draw(mCoreBeam);
	}

	void ContinuousBeamVisualActor::UpdateBeam(
		const sf::Vector2f& start,
		float directionRotation,
		float range,
		float width,
		float heatRatio,
		const sf::Color& baseColor
	)
	{
		const float beamRange = std::max(0.f, range);
		const float beamWidth = std::max(1.f, width);
		const float coreWidth = std::max(1.f, beamWidth * 0.35f);
		const float normalizedHeat = std::clamp(heatRatio, 0.f, 1.f);

		SetActorLocation(start);
		SetActorRotation(directionRotation + 90.f);

		mOuterBeam.setSize({ beamRange, beamWidth });
		mOuterBeam.setOrigin({ 0.f, beamWidth * 0.5f });
		mOuterBeam.setPosition(start);
		mOuterBeam.setRotation(sf::degrees(directionRotation));
		mOuterBeam.setFillColor(LerpColor(baseColor, sf::Color{ 255, 72, 32, 220 }, normalizedHeat));

		mCoreBeam.setSize({ beamRange, coreWidth });
		mCoreBeam.setOrigin({ 0.f, coreWidth * 0.5f });
		mCoreBeam.setPosition(start);
		mCoreBeam.setRotation(sf::degrees(directionRotation));
		mCoreBeam.setFillColor(LerpColor(sf::Color{ 225, 250, 255, 255 }, sf::Color{ 255, 235, 120, 255 }, normalizedHeat));
	}

	bool ContinuousBeamVisualActor::IsValidDamageTarget(const Actor* actor) const
	{
		return actor && actor != this && actor != mOwner && !actor->GetIsPendingDestroy() &&
			CanCollideWith(actor) && actor->CanCollideWith(this);
	}
}
