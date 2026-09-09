#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/portal/PortalDestinationRebaser.h"
#include "gameplay/projectile/ProjectileReflectionService.h"
#include "gameplay/projectile/ProjectileInterceptionService.h"
#include "framework/World.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/effects/content/directionalBarrier/DirectionalBarrierEffectBehavior.h"
#include "framework/MathUtility.h"
#include "gameplay/targeting/SweptGeometry.h"

#include <algorithm>
#include <cmath>

namespace
{
	ly::weak_ptr<ly::Actor> MakeWeakActor(ly::Actor* actor)
	{
		if (!actor)
		{
			return {};
		}
		const ly::shared_ptr<ly::Object> object = actor->GetWeakPtr().lock();
		return object
			? std::dynamic_pointer_cast<ly::Actor>(object)
			: ly::weak_ptr<ly::Actor>{};
	}

	float DistanceSquaredToActorBounds(
		const ly::Actor& actor,
		const sf::Vector2f& point
	)
	{
		const sf::FloatRect bounds = actor.GetActorGlobalBounds();
		if (bounds.size.x <= 0.f || bounds.size.y <= 0.f)
		{
			const sf::Vector2f delta = actor.GetActorLocation() - point;
			return delta.x * delta.x + delta.y * delta.y;
		}

		// Actor physics uses the visual bounds as its default box shape. Testing
		// the closest point on that same bounds keeps splash damage consistent
		// with collision geometry instead of testing only the actor's origin.
		const float closestX = std::clamp(
			point.x,
			bounds.position.x,
			bounds.position.x + bounds.size.x
		);
		const float closestY = std::clamp(
			point.y,
			bounds.position.y,
			bounds.position.y + bounds.size.y
		);
		const float deltaX = point.x - closestX;
		const float deltaY = point.y - closestY;
		return deltaX * deltaX + deltaY * deltaY;
	}

	sf::Vector2f NormalizeOrFallback(
		const sf::Vector2f& value,
		const sf::Vector2f& fallback
	)
	{
		const float length = ly::GetVectorLength(value);
		return length > 0.001f ? value / length : fallback;
	}
}

namespace ly
{
	AbilityWorldActor::AbilityWorldActor(World* world, Actor* owner, const std::string& texturePath)
		: Actor(world, texturePath),
		mOwner{ MakeWeakActor(owner) },
		mUnmanagedOwner{ mOwner.expired() ? owner : nullptr },
		mDamage{ 0.f },
		mLifeTime{ 0.f },
		mAge{ 0.f },
		mPreviousLocation{},
		mCollisionRadius{ 0.f },
		mAllowFriendlyFire{ false },
		mEnablePhysicsOnBeginPlay{ true }
	{
		// A projectile/field owned by an enemy automatically follows the same
		// temporal domain, so Time Slip also affects actors spawned after activation.
		if (owner)
		{
			SetSimulationTimeDomain(owner->GetSimulationTimeDomain());
		}
	}

	Actor* AbilityWorldActor::GetOwnerActor() const
	{
		const shared_ptr<Actor> owner = mOwner.lock();
		return owner ? owner.get() : mUnmanagedOwner;
	}

	Actor* AbilityWorldActor::GetOriginalProjectileOwner() const
	{
		const shared_ptr<Actor> owner = mOriginalProjectileOwner.lock();
		return owner ? owner.get() : mUnmanagedOriginalProjectileOwner;
	}

	void AbilityWorldActor::SetSourceAbilityInstance(GameAbility* instance)
	{
		mSourceAbilityHandle = instance
			? instance->GetHandle()
			: sas::AbilityHandle{};
	}

	GameAbility* AbilityWorldActor::GetSourceAbilityInstance() const
	{
		const shared_ptr<Actor> owner = mOwner.lock();
		Actor* ownerActor = owner ? owner.get() : mUnmanagedOwner;
		auto* combatant = dynamic_cast<Combatant*>(ownerActor);
		return combatant && mSourceAbilityHandle.IsValid()
			? combatant->GetAbilitySystemComponent().GetAbility(mSourceAbilityHandle)
			: nullptr;
	}

	void AbilityWorldActor::BeginPlay()
	{
		// Every concrete ability/weapon projectile already opts in through the
		// shared marker. Assigning the domain here keeps newly added projectile
		// families consistent without repeating temporal code in each constructor.
		if (IsProjectileActor())
		{
			SetSimulationTimeDomain(SimulationTimeDomain::ProjectileGameplay);
		}

		Actor::BeginPlay();
		mPreviousLocation = GetActorLocation();

		if (mEnablePhysicsOnBeginPlay)
		{
			SetEnablePhysics(true);
			if (mCollisionRadius > 0.f)
			{
				SetCollisionRadius(mCollisionRadius);
			}
		}
	}

	void AbilityWorldActor::Tick(float deltaTime)
	{
		if (mPortalTransit)
		{
			return;
		}

		if (!GetIsPendingDestroy() &&
			IsProjectileActor() &&
			DirectionalBarrierEffectBehavior::TryInterceptProjectile(
				*this,
				mPreviousLocation
			))
		{
			mPreviousLocation = GetActorLocation();
			// The interception policy normally only reports the hit and lets this
			// common owner destroy the projectile. Keep the guard because a future
			// boundary is allowed to consume it during its response; calling a
			// derived Destroy() twice can repeat family-specific cleanup.
			if (!GetIsPendingDestroy())
			{
				Destroy();
			}
			return;
		}
		if (!GetIsPendingDestroy() &&
			IsProjectileActor() &&
			ProjectileInterceptionService::TryInterceptProjectile(
				*this,
				mPreviousLocation
			))
		{
			mPreviousLocation = GetActorLocation();
			if (!GetIsPendingDestroy())
			{
				Destroy();
			}
			return;
		}

		mAge += deltaTime;

		if (mLifeTime > 0.f && mAge >= mLifeTime)
		{
			Destroy();
		}

		mPreviousLocation = GetActorLocation();
		Actor::Tick(deltaTime);
	}

	void AbilityWorldActor::OnActorBeginOverlap(Actor* otherActor)
	{
		if (mPortalTransit)
		{
			return;
		}
		Actor::OnActorBeginOverlap(otherActor);
		if (IsProjectileActor() && !CanBeReflected() && otherActor &&
			otherActor->GetPhysicsBodyType() == PhysicsBodyType::Static)
		{
			const sf::Vector2f halfExtents = otherActor->GetPhysicsCollisionBoxHalfExtents();
			if (halfExtents.x > 0.f || halfExtents.y > 0.f)
			{
				// Families that support reflection consume the surface before they
				// reach this overlap. Every other travelling ability projectile is
				// still blocked by the same reusable physical geometry.
				Destroy();
			}
		}
	}

	bool AbilityWorldActor::TryReflectProjectile(
		const ProjectileReflectionRequest& request
	)
	{
		(void)request;
		return false;
	}

	bool AbilityWorldActor::TryReflectOnOverlap(Actor* otherActor)
	{
		if (!otherActor || !IsProjectileActor() || GetIsPendingDestroy())
		{
			return false;
		}

		// A Box2D overlap can arrive before the projectile family's manual sweep.
		// Check both common reflection mechanisms here so that timing does not turn
		// a valid wall bounce into an ordinary projectile impact.
		return ProjectileReflectionService::TryReflectProjectile(*this, *otherActor) ||
			ProjectileReflectionService::TryReflectFromSurfaceOverlap(*this, *otherActor);
	}

	bool AbilityWorldActor::ApplyBallisticReflection(
		const ProjectileReflectionRequest& request,
		float speed,
		sf::Vector2f& inOutTrajectory
	)
	{
		if (!CanBeReflected() ||
			(GetOwnerActor() == &request.newOwner && !request.allowSameOwnerReflection) ||
			GetVectorLength(request.returnDirection) <= 0.001f)
		{
			return false;
		}

		const float safeSpeed = std::max(0.f, speed);
		const sf::Vector2f direction = NormalizeOrFallback(
			request.returnDirection,
			GetActorForwardDirection()
		);
		ApplyReflectionOwnership(request.newOwner, request.damageMultiplier);
		inOutTrajectory = direction * safeSpeed;
		SetVelocity(inOutTrajectory);
		SetActorRotation(std::atan2(direction.y, direction.x) * 57.2957795131f + 90.f);
		return true;
	}

	void AbilityWorldActor::ApplyReflectionOwnership(
		Actor& newOwner,
		float damageMultiplier
	)
	{
		if (!mHasOriginalProjectileOwner)
		{
			mOriginalProjectileOwner = mOwner;
			mUnmanagedOriginalProjectileOwner = mOriginalProjectileOwner.expired()
				? GetOwnerActor()
				: nullptr;
			mHasOriginalProjectileOwner = true;
		}
		mOwner = MakeWeakActor(&newOwner);
		mUnmanagedOwner = mOwner.expired() ? &newOwner : nullptr;
		SetSimulationTimeDomain(
			IsProjectileActor()
				? SimulationTimeDomain::ProjectileGameplay
				: newOwner.GetSimulationTimeDomain()
		);
		mAllowFriendlyFire = false;
		SetDamage(std::max(0.f, GetDamage() * std::max(0.f, damageMultiplier)));
		ConfigureCollisionFromOwner();
	}

	void AbilityWorldActor::BeginPortalTransit()
	{
		if (mPortalTransit)
		{
			return;
		}

		mPortalTransit = true;
		mPortalPhysicsWasEnabled = IsPhysicsEnabled();
		mPortalCollisionLayer = GetCollisionLayer();
		mPortalCollisionMask = GetCollisionMask();
		mPortalVelocity = GetVelocity();
		SetRenderEnabled(false);
		SetVelocity({});
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetEnablePhysics(false);
	}

	void AbilityWorldActor::CompletePortalTransit(
		const sf::Vector2f& exitLocation
	)
	{
		if (!mPortalTransit)
		{
			return;
		}

		SetActorLocation(exitLocation);
		SetCollisionLayer(mPortalCollisionLayer);
		SetCollisionMask(mPortalCollisionMask);
		if (mPortalPhysicsWasEnabled)
		{
			SetEnablePhysics(true);
		}
		SetVelocity(mPortalVelocity);
		SetRenderEnabled(true);
		mPortalTransit = false;
		if (auto* destinationRebaser = dynamic_cast<PortalDestinationRebaser*>(this))
		{
			destinationRebaser->RebasePortalDestination(exitLocation);
		}
	}

	void AbilityWorldActor::SetAbilityCollisionRadius(float radius)
	{
		mCollisionRadius = std::max(0.f, radius);
		SetCollisionRadius(mCollisionRadius);
		if (World* world = GetWorld())
		{
			world->RefreshActorSpatialQuery(*this);
		}
	}

	void AbilityWorldActor::SetRelayProjectileDamagePolicy(bool allowFriendlyFire)
	{
		mAllowFriendlyFire = allowFriendlyFire;
		if (mAllowFriendlyFire)
		{
			SetCollisionLayer(CollisionLayer::RelayProjectile);
			SetCollisionMask(CollisionLayer::AllRelayTargets);
		}
		else
		{
			ConfigureCollisionFromOwner();
		}
	}

	void AbilityWorldActor::ConfigureRelayClone(
		const ProjectileRelayCloneRequest& request
	)
	{
		SetActorLocation(request.location);
		if (GetVectorLength(request.direction) > 0.001f)
		{
			const float rotation = std::atan2(
				request.direction.y,
				request.direction.x
			) * 57.2957795131f + 90.f;
			SetActorRotation(rotation);
		}

		SetDamageTags(request.snapshot.damageTags);
		SetSourceAbility(
			request.snapshot.sourceAbilityId,
			request.snapshot.sourceAbilityTags
		);
		SetProjectileRelayLineage(request.snapshot.lineage);
		SetAbilityCollisionRadius(request.snapshot.collisionRadius);
		SetLifeTime(request.snapshot.remainingLifetime);
		SetRelayProjectileDamagePolicy(request.allowFriendlyFire);
		SetDamage(request.damage);
	}

	void AbilityWorldActor::SetDamageTags(const List<GameplayTag>& damageTags)
	{
		mDamageTags = damageTags;
		RebuildDamagePayload();
	}

	void AbilityWorldActor::SetDamageAttributes(const sas::GameplayAttributeList& attributes)
	{
		mDamageAttributes = attributes;
		RebuildDamagePayload();
	}

	bool AbilityWorldActor::BuildRelaySnapshot(
		ProjectileRelaySnapshot& snapshot
	) const
	{
		if (!CanBeCapturedByRelay())
		{
			return false;
		}

		snapshot.damage = GetDamage();
		snapshot.damageAttributes = mDamageAttributes;
		snapshot.damageTags = mDamageTags;
		snapshot.sourceAbilityId = mSourceAbilityId;
		snapshot.sourceAbilityTags = mSourceAbilityTags;
		snapshot.velocity = GetVelocity();
		snapshot.collisionRadius = mCollisionRadius;
		snapshot.remainingLifetime = mLifeTime > 0.f
			? std::max(0.f, mLifeTime - mAge)
			: 0.f;
		snapshot.lineage = mProjectileRelayLineage;
		return true;
	}

	weak_ptr<AbilityWorldActor> AbilityWorldActor::SpawnRelayClone(
		const ProjectileRelayCloneRequest& request
	) const
	{
		(void)request;
		return {};
	}

	void AbilityWorldActor::RebuildDamagePayload()
	{
		mDamagePayload = DamageTypeSystem::BuildPayload(mDamageTags, mDamageAttributes);
	}

	bool AbilityWorldActor::HasAbilityUpgrade(const std::string& upgradeId) const
	{
		return std::any_of(
			mAbilityUpgradeIds.begin(),
			mAbilityUpgradeIds.end(),
			[&](const std::string& unlockedUpgradeId)
			{
				return unlockedUpgradeId == upgradeId;
			}
		);
	}

	void AbilityWorldActor::ConfigureCollisionFromOwner()
	{
		const shared_ptr<Actor> owner = mOwner.lock();
		Actor* ownerActor = owner ? owner.get() : mUnmanagedOwner;
		if (!ownerActor)
		{
			SetCollisionLayer(CollisionLayer::None);
			SetCollisionMask(CollisionLayer::None);
			return;
		}

		switch (ownerActor->GetCollisionLayer())
		{
		case CollisionLayer::Player:
		case CollisionLayer::FriendlySummon:
			SetCollisionLayer(CollisionLayer::PlayerBullet);
			SetCollisionMask(
				CollisionLayer::Enemy |
				CollisionLayer::EnemyBullet |
				CollisionLayer::Environment
			);
			break;
		case CollisionLayer::Enemy:
			SetCollisionLayer(CollisionLayer::EnemyBullet);
			SetCollisionMask(
				CollisionLayer::Player |
				CollisionLayer::FriendlySummon |
				CollisionLayer::PlayerBullet |
				CollisionLayer::Environment
			);
			break;
		default:
			SetCollisionLayer(CollisionLayer::None);
			SetCollisionMask(CollisionLayer::None);
			break;
		}
	}

	bool AbilityWorldActor::IsValidAbilityTarget(const Actor* actor) const
	{
		const shared_ptr<Actor> owner = mOwner.lock();
		Actor* ownerActor = owner ? owner.get() : mUnmanagedOwner;
		return actor && actor != this && (mAllowFriendlyFire || actor != ownerActor) &&
			!actor->GetIsPendingDestroy()
			&& CanCollideWith(actor) && actor->CanCollideWith(this);
	}

	void AbilityWorldActor::ApplyCombatDamageInRadius(
		const sf::Vector2f& center,
		float radius,
		float damageMultiplier
	)
	{
		World* world = GetWorld();
		const float effectiveRadius = std::max(0.f, radius);
		const float damage = std::max(0.f, mDamage * std::max(0.f, damageMultiplier));
		if (world == nullptr || effectiveRadius <= 0.f || damage <= 0.f)
		{
			return;
		}

		const float radiusSquared = effectiveRadius * effectiveRadius;
		const shared_ptr<Actor> owner = mOwner.lock();
		Actor* ownerActor = owner ? owner.get() : mUnmanagedOwner;
		world->ForEachActorInBounds(
			targeting::swept::RadiusBounds(center, effectiveRadius),
			[this, center, radiusSquared, damage, ownerActor](Actor& target)
			{
			if (!IsValidAbilityTarget(&target))
			{
				return;
			}

			const float distanceSquared = DistanceSquaredToActorBounds(target, center);
			if (distanceSquared <= radiusSquared)
			{
				ApplyCombatDamage(
					target,
					damage,
					ownerActor,
					mDamageTags,
					mDamagePayload,
					mSourceAbilityId,
					mSourceAbilityTags,
					DamageDeliveryType::Area,
					this
				);
			}
			}
		);
	}

	void AbilityWorldActor::ConfigureFromAttributes(const sas::GameplayAttributeList& attributes)
	{
		mDamage = std::max(
			0.f,
			sas::FindAttributeValue(attributes, CommonAttributeIds::Damage, mDamage)
		);
		SetDamageAttributes(attributes);
	}
}
