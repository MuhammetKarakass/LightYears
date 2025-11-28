#include "enemy/EnemySpaceShip.h"

namespace ly
{
	EnemySpaceShip::EnemySpaceShip(World* owningWorld, const std::string& texturePath, float collisionDamage, const List<WeightedReward>& weightedRewards) :
		SpaceShip{ owningWorld, texturePath },
		mCollisionDamage{ collisionDamage },
		mWeightedRewards{ weightedRewards.empty() ? GetDefaultRewards() : weightedRewards },
		mScoreAmt{ 10 }
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

	void EnemySpaceShip::OnActorBeginOverlap(Actor* other)
	{
		SpaceShip::OnActorBeginOverlap(other);
		if (other == nullptr) return;
		if (GetCanCollide())
		{
			other->ApplyDamage(mCollisionDamage);
		}
	}
	
	void EnemySpaceShip::SpawnReward()
	{
		if (mWeightedRewards.size() == 0) return;

		// Calculate total weight
		float totalWeight = 0.0f;
		for (const auto& reward : mWeightedRewards)
		{
			totalWeight += reward.weight;
		}

		// ? FIX: Random 0.0 to 1.0, NOT 0.0 to totalWeight!
		float randValue = RandRange(0.0f, 1.0f);

		// If random exceeds total weight, no reward spawns
		if (randValue > totalWeight)
		{
			return;  // Nothing spawns (e.g., if totalWeight=0.8, 20% chance to spawn nothing)
		}

		// Select reward based on weight
		float currentWeight = 0.0f;
		for (const auto& reward : mWeightedRewards)
		{
			currentWeight += reward.weight;
			if (randValue <= currentWeight)
			{
				// Spawn this reward
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

	// Default generic rewards (fallback)
	List<WeightedReward> EnemySpaceShip::GetDefaultRewards()
	{
		return {
			{CreateRewardHealth, 0.2f},   // 20% - Health
			{CreateRewardThreeWayShooter, 0.1f},  // 10% - Three Way
			{CreateRewardFrontalWiper, 0.1f},     // 10% - Frontal Wiper
			{CreateRewardLife, 0.05f}   // 5% - Extra Life
			// 55% - Nothing spawns
		};
	}
}