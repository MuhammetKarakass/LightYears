#include "enemy/EnemySpaceShip.h"
#include "gameplay/combat/Combatant.h"

namespace ly
{
	EnemySpaceShip::EnemySpaceShip(World* owningWorld, const ShipDefinition& shipDef) :
		SpaceShip{ owningWorld, shipDef },
		mCollisionDamage{ shipDef.collisionDamage },
		mWeightedRewards{ shipDef.rewards},
		mScoreAmt{ shipDef.scoreAmt },
		mShipXPReward{ shipDef.shipXPReward }
	{
		// Enemy ships are the hostile gameplay domain. Their child projectiles
		// inherit this domain through AbilityWorldActor's owner propagation.
		SetSimulationTimeDomain(SimulationTimeDomain::HostileGameplay);
	}
	
	void EnemySpaceShip::Tick(float deltaTime)
	{
		SpaceShip::Tick(deltaTime);

		if (IsActorOutOfWindow(GetActorGlobalBounds().size.x*2.f))
		{
			Destroy();
		}
	}

	void EnemySpaceShip::SetupCollisionLayers()
	{
		SetCollisionLayer(CollisionLayer::Enemy);  
		SetCollisionMask(
			CollisionLayer::Player |
			CollisionLayer::FriendlySummon |
			CollisionLayer::PlayerBullet |
			CollisionLayer::RelayProjectile |
			// Environment includes static ability walls such as Lance Drive's
			// edge segments and the reusable Crystal Barricade geometry.
			CollisionLayer::Environment
		);
	}

	void EnemySpaceShip::OnActorBeginOverlap(Actor* otherActor)
	{
		SpaceShip::OnActorBeginOverlap(otherActor);
		if (otherActor == nullptr) return;
		if (otherActor->GetCollisionLayer() == CollisionLayer::Player &&
			CanApplyContactDamage(*this, *otherActor))
		{
			ApplyCombatDamage(*otherActor, mCollisionDamage, this);
		}
	}
	
	void EnemySpaceShip::SpawnReward()
	{
		if (mWeightedRewards.size() == 0) return;

		float totalWeight = 0.0f;
		for (const auto& reward : mWeightedRewards)
		{
			totalWeight += reward.weight;
		}

		float randValue = RandRange(0.0f, 1.0f);

		if (randValue > totalWeight)
		{
		}

		float currentWeight = 0.0f;
		for (const auto& reward : mWeightedRewards)
		{
			currentWeight += reward.weight;
			if (randValue <= currentWeight)
			{
				weak_ptr<Reward> spawnedReward = reward.factory(GetWorld());
				if (auto rewardPtr = spawnedReward.lock())
				{
					rewardPtr->SetActorLocation(GetActorLocation());
				}
				return;
			}
		}
	}
	
	void EnemySpaceShip::Blew()
	{
		SpawnReward();
		onScoreAwarded.Broadcast(mScoreAmt);
		onShipXPAwarded.Broadcast(mShipXPReward);
	}
}

