#include "enemy/EnemySpaceShip.h"

namespace ly
{
	EnemySpaceShip::EnemySpaceShip(World* owningWorld, const ShipDefinition& shipDef) :
		SpaceShip{ owningWorld, shipDef },
		mCollisionDamage{ shipDef.collisionDamage },
		mWeightedRewards{ shipDef.rewards},
		mScoreAmt{ shipDef.scoreAmt }
	{
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
		SetCollisionMask(CollisionLayer::Player | CollisionLayer::PlayerBullet);
	}

	void EnemySpaceShip::OnActorBeginOverlap(Actor* otherActor)
	{
		SpaceShip::OnActorBeginOverlap(otherActor);
		if (otherActor == nullptr) return;
		if (otherActor->GetCollisionLayer() == CollisionLayer::Player)
		{
			otherActor->ApplyDamage(mCollisionDamage);
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
	}
}