#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/projectile/ProjectileCaptureVolume.h"
#include "gameplay/projectile/ProjectileReflectionService.h"
#include "gameplay/weapon/impact/ProjectileImpactBehavior.h"
#include "gameplay/projectile/ProjectileSweep.h"
#include "framework/Core.h"
#include "framework/PerfMonitor.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		sf::Vector2f NormalizeOrDefault(
			const sf::Vector2f& direction,
			const sf::Vector2f& fallback
		)
		{
			const float length = GetVectorLength(direction);
			return length > 0.001f ? direction / length : fallback;
		}
	}

	PrimaryWeaponProjectileActor::PrimaryWeaponProjectileActor(
		World* world,
		Actor* owner,
		const WeaponPresentationDefinition& presentation,
		const sas::GameplayAttributeList& values
	)
		: AbilityWorldActor(world, owner, presentation.texturePath),
		mSpeed(sas::FindAttributeValue(values, PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f)),
		mLaunchVelocity{},
		mMaxTravelDistance(sas::FindAttributeValue(values, CommonAttributeIds::Range, 1600.f)),
		mTravelDistance(0.f),
		mAreaDamageRadius(std::max(0.f, sas::FindAttributeValue(values, AreaAttributeIds::Radius, 0.f))),
		mVisualScale(std::max(0.01f, presentation.visualScale)),
		mRemainingPierces(std::max(0, static_cast<int>(std::round(sas::FindAttributeValue(values, PrimaryWeaponSchema::Projectile::Delivery::PierceCount, 0.f))))),
		mPresentationDefinition(presentation)
	{
		SetRenderLayer(RenderLayer::Projectile);
		SetDamage(sas::FindAttributeValue(values, CommonAttributeIds::Damage, 0.f));
		SetDamageAttributes(values);
		SetLifeTime(sas::FindAttributeValue(values, PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.f));
		SetAbilityCollisionRadius(std::max(0.1f, sas::FindAttributeValue(values, CollisionAttributeIds::Radius, 8.f)));
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
		if (IsInPortalTransit())
		{
			AbilityWorldActor::Tick(deltaTime);
			return;
		}
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
		if (GetIsPendingDestroy())
		{
			return;
		}
		if (TryReflectOnOverlap(otherActor))
		{
			return;
		}
		if (auto* captureVolume = dynamic_cast<ProjectileCaptureVolume*>(otherActor);
			captureVolume && captureVolume->TryCaptureProjectile(*this))
		{
			return;
		}
		if (!otherActor ||
			!mProcessedImpactTargets.insert(otherActor->GetUniqueID()).second)
		{
			return;
		}
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

	weak_ptr<AbilityWorldActor> PrimaryWeaponProjectileActor::SpawnRelayClone(
		const ProjectileRelayCloneRequest& request
	) const
	{
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner)
		{
			return {};
		}

		weak_ptr<PrimaryWeaponProjectileActor> clone =
			world->SpawnActor<PrimaryWeaponProjectileActor>(
				owner,
				mPresentationDefinition,
				request.snapshot.damageAttributes
			);
		if (const shared_ptr<PrimaryWeaponProjectileActor> spawned = clone.lock())
		{
			spawned->ConfigureFromAttributes(request.snapshot.damageAttributes);
			spawned->ConfigureRelayClone(request);
			const float speed = sas::FindAttributeValue(
				request.snapshot.damageAttributes,
				PrimaryWeaponSchema::Projectile::Delivery::Speed,
				500.f
			);
			spawned->SetLaunchVelocity(request.direction * speed);
		}
		return clone;
	}

	bool PrimaryWeaponProjectileActor::TryReflectProjectile(
		const ProjectileReflectionRequest& request
	)
	{
		if (!CanBeReflected() ||
			(GetOwnerActor() == &request.newOwner && !request.allowSameOwnerReflection) ||
			GetVectorLength(request.returnDirection) <= 0.001f)
		{
			return false;
		}

		const float speed = std::max(
			0.f,
			GetVectorLength(mLaunchVelocity) > 0.001f
				? GetVectorLength(mLaunchVelocity)
				: mSpeed
		);
		const sf::Vector2f direction = NormalizeOrDefault(
			request.returnDirection,
			GetActorForwardDirection()
		);
		ApplyReflectionOwnership(request.newOwner, request.damageMultiplier);
		mLaunchVelocity = direction * speed;
		SetVelocity(mLaunchVelocity);
		SetActorRotation(std::atan2(direction.y, direction.x) * 57.2957795131f + 90.f);
		return true;
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

		const sf::Vector2f startLocation = GetActorLocation();
		const sf::Vector2f endLocation = startLocation + mLaunchVelocity * deltaTime;
		World* world = GetWorld();
		if (!world || deltaTime <= 0.f)
		{
			SetActorLocation(endLocation);
			return;
		}

		const float collisionRadius = std::max(0.f, GetPhysicsCollisionRadius());
		for (const projectile::SweptContact& contact :
			projectile::FindSweptContacts(
				*this,
				startLocation,
				endLocation,
				collisionRadius
			))
		{
			if (GetIsPendingDestroy() || IsInPortalTransit())
			{
				return;
			}
			SetActorLocation(
				startLocation + (endLocation - startLocation) * contact.fraction
			);
			if (contact.hasSurfaceNormal &&
				ProjectileReflectionService::TryReflectFromSurface(
					*this,
					*contact.actor,
					{ contact.impactLocation, contact.surfaceNormal }
				))
			{
				// The reflected velocity starts a fresh segment next frame. Continuing
				// along the pre-impact segment would overwrite the new direction.
				return;
			}
			OnActorBeginOverlap(contact.actor.get());
		}
		if (!GetIsPendingDestroy() && !IsInPortalTransit())
		{
			SetActorLocation(endLocation);
		}
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
			ApplyCombatDamage(
				*directHitActor,
				GetDamage(),
				GetOwner(),
				GetDamageTags(),
				GetDamagePayload(),
				sas::ContentId{},
				{},
				DamageDeliveryType::Projectile,
				this
			);
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
