#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "framework/World.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"

#include <algorithm>

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

	void AbilityWorldActor::SetDamageAttributes(const GameplayAttributeList& attributes)
	{
		mDamageAttributes = attributes;
		RebuildDamagePayload();
	}

	void AbilityWorldActor::RebuildDamagePayload()
	{
		mDamagePayload = DamageTypeSystem::BuildPayload(mDamageTags, mDamageAttributes);
	}

	bool AbilityWorldActor::HasAbilityUpgrade(const GameplayTag& upgradeId) const
	{
		return std::any_of(
			mAbilityUpgradeIds.begin(),
			mAbilityUpgradeIds.end(),
			[&](const GameplayTag& unlockedUpgradeId)
			{
				return unlockedUpgradeId.MatchesTag(upgradeId);
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

			const sf::Vector2f delta = target->GetActorLocation() - center;
			const float distanceSquared = delta.x * delta.x + delta.y * delta.y;
			if (distanceSquared <= radiusSquared)
			{
				ApplyCombatDamage(*target, damage, mOwner, mDamageTags, mDamagePayload);
			}
		}
	}

	void AbilityWorldActor::ConfigureFromAttributes(const GameplayAttributeList& attributes)
	{
		mDamage = std::max(
			0.f,
			FindGameplayAttributeValue(attributes, CommonAttributeIds::Damage, mDamage)
		);
		SetDamageAttributes(attributes);
	}
}


