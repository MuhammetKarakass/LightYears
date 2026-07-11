#include "weapon/Bullet.h"
#include "gameplay/combat/Combatant.h"
#include "framework/Core.h"
#include "framework/PerfMonitor.h"
#include "framework/MathUtility.h"
#include <algorithm>
#include <cmath>

namespace ly
{
	Bullet::Bullet(World* world, Actor* owner, const WeaponPresentationDefinition& presentation, const GameplayAttributeList& values)
		: AbilityWorldActor(world, owner, presentation.texturePath),
		mSpeed(FindGameplayAttributeValue(values, PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f)),
		mMaxTravelDistance(FindGameplayAttributeValue(values, CommonAttributeIds::Range, 1600.f)),
		mTravelDistance(0.f),
		mAreaDamageRadius(std::max(0.f, FindGameplayAttributeValue(values, CommonAttributeIds::AreaRadius, 0.f))),
		mVisualScale(std::max(0.01f, presentation.visualScale)),
		mRemainingPierces(std::max(0, static_cast<int>(std::round(FindGameplayAttributeValue(values, PrimaryWeaponSchema::Projectile::Delivery::PierceCount, 0.f)))))
	{
		SetDamage(FindGameplayAttributeValue(values, CommonAttributeIds::Damage, 0.f));
		SetLifeTime(FindGameplayAttributeValue(values, PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.f));
		SetAbilityCollisionRadius(std::max(0.1f, FindGameplayAttributeValue(values, CommonAttributeIds::CollisionRadius, 8.f)));
		ConfigureCollisionFromOwner();
		SetVisualScale(mVisualScale);
		ly::perf::IncBullets();

	}

	void Bullet::SetVisualScale(float scale)
	{
		if (GetSprite())
		{
			GetSprite().value().setScale({ scale, scale });
		}
	}

	void Bullet::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
	}

	void Bullet::Tick(float deltaTime)
	{
		Move(deltaTime);
		mTravelDistance += std::abs(mSpeed) * deltaTime;

		const bool exceededTravelDistance = mMaxTravelDistance > 0.f && mTravelDistance >= mMaxTravelDistance;

		if (exceededTravelDistance)
		{
			Destroy();
		}
		AbilityWorldActor::Tick(deltaTime);
	}

	void Bullet::OnActorBeginOverlap(Actor* otherActor)
	{
		AbilityWorldActor::OnActorBeginOverlap(otherActor);
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
		SetVelocity(GetActorForwardDirection() * mSpeed);
		AddActorLocationOffset(GetActorForwardDirection() * mSpeed * deltaTime);
	}

	void Bullet::ApplyImpactDamage(Actor* directHitActor)
	{
		if (mAreaDamageRadius > 0.f)
		{
			ApplyAreaDamage();
			return;
		}

		if (IsValidAbilityTarget(directHitActor))
		{
			ApplyCombatDamage(*directHitActor, GetDamage(), GetOwner(), GetDamageTags());
			LOG("damage: %f", GetDamage());
		}
	}

	void Bullet::ApplyAreaDamage()
	{
		ApplyCombatDamageInRadius(GetActorLocation(), mAreaDamageRadius);
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
