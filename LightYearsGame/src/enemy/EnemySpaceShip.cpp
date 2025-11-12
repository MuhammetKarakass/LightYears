#include "enemy/EnemySpaceShip.h"

namespace ly
{
	EnemySpaceShip::EnemySpaceShip(World* owningWorld, const std::string& texturePath, float collisionDamage, const List<RewardFactoryFunc>& rewardFactories) :
		SpaceShip{owningWorld, texturePath},
		mCollisionDamage{collisionDamage},
		mRewardFactories{rewardFactories}
	{
	}
	void EnemySpaceShip::Tick(float deltaTime)
	{
		SpaceShip::Tick(deltaTime);  // Base class tick'i çaðýr

		if (IsActorOutOfWindow(GetActorGlobalBounds().size.x*2.f))
		{
			Destroy();
		}
	}

	void EnemySpaceShip::SetupCollisionLayers()
	{
		LOG("EnemySpaceShip::OnActorBeginOverlap called with actor: ");
		SetCollisionLayer(CollisionLayer::Enemy);  
		SetCollisionMask(CollisionLayer::Player | CollisionLayer::PlayerBullet);
	}

	void EnemySpaceShip::OnActorBeginOverlap(Actor* other)
	{
		SpaceShip::OnActorBeginOverlap(other);  // Base class'ý çaðýr
		if (other == nullptr) return;
		if (GetCanCollide())
		{
			other->ApplyDamage(mCollisionDamage);
		}
	}
	void EnemySpaceShip::SpawnReward()
	{
		if (mRewardFactories.size() == 0) return;
		int randIndex = RandRange(0, int(mRewardFactories.size() - 1));

		if(randIndex >=0 && randIndex < mRewardFactories.size())
		{
			
			weak_ptr<Reward> reward = mRewardFactories[randIndex](GetWorld());
			if (auto rewardPtr = reward.lock())
			{
				rewardPtr->SetActorLocation(GetActorLocation());
			}
		}
	}
	void EnemySpaceShip::Blew()
	{
		SpawnReward();
	}
}