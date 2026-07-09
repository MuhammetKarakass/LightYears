#include "weapon/Bullet.h"
#include "framework/Core.h"
#include "framework/PerfMonitor.h"
#include "framework/World.h"
#include "framework/MathUtility.h"
#include <algorithm>
#include <cmath>

namespace ly
{
	Bullet::Bullet(World* world, Actor* owner, const WeaponPresentationDefinition& presentation, const PrimaryWeaponAttributes& attributes)
		: Actor(world, presentation.texturePath),
		mOwner(owner),
		mSpeed(attributes.projectileSpeed.currentValue),
		mDamage(attributes.damage.currentValue),
		mLifeTime(attributes.projectileLifeTime.currentValue),
		mAge(0.f),
		mMaxTravelDistance(attributes.projectileMaxTravelDistance.currentValue),
		mTravelDistance(0.f),
		mAreaDamageRadius(std::max(0.f, attributes.projectileAreaRadius.currentValue)),
		mCollisionRadius(std::max(0.1f, attributes.projectileCollisionRadius.currentValue)),
		mVisualScale(std::max(0.01f, presentation.visualScale)),
		mRemainingPierces(std::max(0, static_cast<int>(std::round(attributes.pierceCount.currentValue))))
	{
		SetupCollisionFromOwner();
		SetVisualScale(mVisualScale);
		ly::perf::IncBullets();

	}

	void Bullet::SetSpeed(float speed)
	{
		mSpeed = speed;
		SetVelocity(GetActorForwardDirection() * mSpeed);
	}

	void Bullet::SetDamage(float damage)
	{
		mDamage = damage;
	}

	void Bullet::SetVisualScale(float scale)
	{
		if (GetSprite())
		{
			GetSprite().value().setScale({ scale, scale });
		}
	}

	void Bullet::SetProjectileCollisionRadius(float radius)
	{
		SetCollisionRadius(radius);
	}

	void Bullet::SetProjectileAreaRadius(float radius)
	{
		mAreaDamageRadius = std::max(0.f, radius);
	}

	void Bullet::BeginPlay()
	{
		Actor::BeginPlay();
		SetEnablePhysics(true);
		SetProjectileCollisionRadius(mCollisionRadius);
		
	}

	void Bullet::Tick(float deltaTime)
	{
		Move(deltaTime);
		mAge += deltaTime;
		mTravelDistance += std::abs(mSpeed) * deltaTime;

		const bool exceededLifeTime = mLifeTime > 0.f && mAge >= mLifeTime;
		const bool exceededTravelDistance = mMaxTravelDistance > 0.f && mTravelDistance >= mMaxTravelDistance;

		if (exceededLifeTime || exceededTravelDistance)
		{
			Destroy();
		}
		Actor::Tick(deltaTime);
	}

	void Bullet::OnActorBeginOverlap(Actor* otherActor)
	{
		Actor::OnActorBeginOverlap(otherActor);
		if(GetCanCollide())
		{
			ApplyImpactDamage(otherActor);

			if (mAreaDamageRadius > 0.f)
			{
				Destroy();
				return;
			}

			if (mRemainingPierces > 0)
			{
				--mRemainingPierces;
				return;
			}

			Destroy();
		}
	}

	void Bullet::Move(float deltaTime)
	{
		if (mOwner)
		{
			SetVelocity(GetActorForwardDirection() * mSpeed);
			AddActorLocationOffset(GetActorForwardDirection() * mSpeed * deltaTime);
		}
	}

	void Bullet::SetupCollisionFromOwner()
	{
		/*if (!mOwner)
		{
			SetCollisionLayer(CollisionLayer::None);
			SetCollisionMask(CollisionLayer::None);
			return;
		}*/

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
			SetCollisionLayer(CollisionLayer::None);
			SetCollisionMask(CollisionLayer::None);
			break;
		}
	}

	void Bullet::ApplyImpactDamage(Actor* directHitActor)
	{
		if (mAreaDamageRadius > 0.f)
		{
			ApplyAreaDamage();
			return;
		}

		if (directHitActor)
		{
			directHitActor->ApplyDamage(GetDamage());
			LOG("damage: %f", GetDamage());
		}
	}

	void Bullet::ApplyAreaDamage()
	{
		if (!GetWorld())
		{
			return;
		}

		const sf::Vector2f impactLocation = GetActorLocation();
		const float areaRadiusSquared = mAreaDamageRadius * mAreaDamageRadius;

		for (const weak_ptr<Actor>& weakActor : GetWorld()->GetActorsByType<Actor>())
		{
			auto actor = weakActor.lock();
			if (!actor || actor.get() == this || actor.get() == mOwner || actor->GetIsPendingDestroy())
			{
				continue;
			}

			if (!IsValidAreaDamageTarget(actor.get()))
			{
				continue;
			}

			const sf::Vector2f delta = actor->GetActorLocation() - impactLocation;
			const float distanceSquared = delta.x * delta.x + delta.y * delta.y;
			if (distanceSquared <= areaRadiusSquared)
			{
				actor->ApplyDamage(GetDamage());
				LOG("area damage: %f", GetDamage());
			}
		}
	}

	bool Bullet::IsValidAreaDamageTarget(const Actor* actor) const
	{
		return actor && CanCollideWith(actor) && actor->CanCollideWith(this);
	}

	void Bullet::Destroy()
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		ly::perf::DecBullets();
		Actor::Destroy();
	}

}
