#pragma once
#include <spaceShip/SpaceShip.h>
#include "player/Reward.h"

namespace ly
{
	class EnemySpaceShip : public SpaceShip
	{
	public:
		EnemySpaceShip(World* owningWorld, const std::string& texturePath, float collisionDamage=75.f, 
			const List<RewardFactoryFunc>& rewardFactories = 
			{
				CreateRewardHealth,
				CreateRewardThreeWayShooter,
				CreateRewardFrontalWiper
			});


		virtual void Tick(float deltaTime) override;
		virtual void SetupCollisionLayers() override;
		
		float GetCollisionDamage() const { return mCollisionDamage; }

	private:
		virtual void OnActorBeginOverlap(Actor* other) override;
		void SpawnReward();
		virtual void Blew() override;
		float mCollisionDamage;
		List<RewardFactoryFunc> mRewardFactories;
	};
}