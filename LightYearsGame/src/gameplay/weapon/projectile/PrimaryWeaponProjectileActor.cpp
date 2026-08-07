#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/weapon/impact/ProjectileImpactBehavior.h"
#include "framework/Core.h"
#include "framework/PerfMonitor.h"
#include "framework/MathUtility.h"
#include <algorithm>
#include <cmath>

namespace ly
{
	PrimaryWeaponProjectileActor::PrimaryWeaponProjectileActor(
		World* world,
		Actor* owner,
		const WeaponPresentationDefinition& presentation,
		const sas::GameplayAttributeList& values
	)
		: AbilityWorldActor(world, owner, presentation.texturePath),
		mSpeed(sas::FindGameplayAttributeValue(values, PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f)),
		mLaunchVelocity{},
		mMaxTravelDistance(sas::FindGameplayAttributeValue(values, CommonAttributeIds::Range, 1600.f)),
		mTravelDistance(0.f),
		mAreaDamageRadius(std::max(0.f, sas::FindGameplayAttributeValue(values, AreaAttributeIds::Radius, 0.f))),
		mVisualScale(std::max(0.01f, presentation.visualScale)),
		mRemainingPierces(std::max(0, static_cast<int>(std::round(sas::FindGameplayAttributeValue(values, PrimaryWeaponSchema::Projectile::Delivery::PierceCount, 0.f)))))
	{
		SetRenderLayer(RenderLayer::Projectile);
		SetDamage(sas::FindGameplayAttributeValue(values, CommonAttributeIds::Damage, 0.f));
		SetDamageAttributes(values);
		SetLifeTime(sas::FindGameplayAttributeValue(values, PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.f));
		SetAbilityCollisionRadius(std::max(0.1f, sas::FindGameplayAttributeValue(values, CollisionAttributeIds::Radius, 8.f)));
		ConfigureCollisionFromOwner();
		SetVisualScale(mVisualScale);
		ly::perf::IncBullets();

	}

	void PrimaryWeaponProjectileActor::SetVisualScale(float scale)
	{
		if (GetSprite())
		{
			GetSprite().value().setScale({ scale, scale });
		}
	}

	void PrimaryWeaponProjectileActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
	}

	void PrimaryWeaponProjectileActor::Tick(float deltaTime)
	{
		Move(deltaTime);
		mTravelDistance += GetVectorLength(mLaunchVelocity) * deltaTime;

		const bool exceededTravelDistance = mMaxTravelDistance > 0.f && mTravelDistance >= mMaxTravelDistance;

		if (exceededTravelDistance)
		{
			Destroy();
		}
		AbilityWorldActor::Tick(deltaTime);
	}

	void PrimaryWeaponProjectileActor::OnActorBeginOverlap(Actor* otherActor)
	{
		AbilityWorldActor::OnActorBeginOverlap(otherActor);
		if(GetCanCollide())
		{
			if (mImpactBehavior && IsValidAbilityTarget(otherActor) &&
				mImpactBehavior->HandleImpact(*otherActor))
			{
				Destroy();
				return;
			}
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

	void PrimaryWeaponProjectileActor::SetImpactBehavior(
		const shared_ptr<ProjectileImpactBehavior>& impactBehavior
	)
	{
		mImpactBehavior = impactBehavior;
	}

	void PrimaryWeaponProjectileActor::SetLaunchVelocity(const sf::Vector2f& launchVelocity)
	{
		mLaunchVelocity = launchVelocity;
		mHasLaunchVelocity = true;
		SetVelocity(mLaunchVelocity);
	}

	void PrimaryWeaponProjectileActor::Move(float deltaTime)
	{
		if (!mHasLaunchVelocity)
		{
			mLaunchVelocity = GetActorForwardDirection() * mSpeed;
			mHasLaunchVelocity = true;
		}
		SetVelocity(mLaunchVelocity);
		AddActorLocationOffset(mLaunchVelocity * deltaTime);
	}

	void PrimaryWeaponProjectileActor::ApplyImpactDamage(Actor* directHitActor)
	{
		if (mAreaDamageRadius > 0.f)
		{
			ApplyAreaDamage();
			return;
		}

		if (IsValidAbilityTarget(directHitActor))
		{
			ApplyCombatDamage(*directHitActor, GetDamage(), GetOwner(), GetDamageTags(), GetDamagePayload());
			LY_GAME_TRACE("Primary weapon projectile damage: %f", GetDamage());
		}
	}

	void PrimaryWeaponProjectileActor::ApplyAreaDamage()
	{
		ApplyCombatDamageInRadius(GetActorLocation(), mAreaDamageRadius);
	}

	void PrimaryWeaponProjectileActor::Destroy()
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		if (mImpactBehavior && !mImpactBehaviorCompleted)
		{
			mImpactBehavior->OnProjectileFinished();
			mImpactBehaviorCompleted = true;
		}
		ly::perf::DecBullets();
		Actor::Destroy();
	}

}
