#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "framework/World.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"

#include <algorithm>

namespace
{
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
}

namespace ly
{
	AbilityWorldActor::AbilityWorldActor(World* world, Actor* owner, const std::string& texturePath)
		: Actor(world, texturePath),
		mOwner{ owner },
		mDamage{ 0.f },
		mLifeTime{ 0.f },
		mAge{ 0.f },
		mCollisionRadius{ 0.f },
		mEnablePhysicsOnBeginPlay{ true }
	{
	}

	void AbilityWorldActor::BeginPlay()
	{
		Actor::BeginPlay();

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
		mAge += deltaTime;

		if (mLifeTime > 0.f && mAge >= mLifeTime)
		{
			Destroy();
		}

		Actor::Tick(deltaTime);
	}

	void AbilityWorldActor::OnActorBeginOverlap(Actor* otherActor)
	{
		Actor::OnActorBeginOverlap(otherActor);
	}

	void AbilityWorldActor::SetAbilityCollisionRadius(float radius)
	{
		mCollisionRadius = std::max(0.f, radius);
		SetCollisionRadius(mCollisionRadius);
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
		if (!mOwner)
		{
			SetCollisionLayer(CollisionLayer::None);
			SetCollisionMask(CollisionLayer::None);
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
			SetCollisionLayer(CollisionLayer::None);
			SetCollisionMask(CollisionLayer::None);
			break;
		}
	}

	bool AbilityWorldActor::IsValidAbilityTarget(const Actor* actor) const
	{
		return actor && actor != this && actor != mOwner && !actor->GetIsPendingDestroy()
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
		for (const weak_ptr<Actor>& actorWeak : world->GetActorsByType<Actor>())
		{
			const shared_ptr<Actor> target = actorWeak.lock();
			if (!target || !IsValidAbilityTarget(target.get()))
			{
				continue;
			}

			const float distanceSquared = DistanceSquaredToActorBounds(*target, center);
			if (distanceSquared <= radiusSquared)
			{
				ApplyCombatDamage(
					*target,
					damage,
					mOwner,
					mDamageTags,
					mDamagePayload,
					mSourceAbilityId,
					mSourceAbilityTags
				);
			}
		}
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
