#pragma once
#include <spaceShip/SpaceShip.h>
#include "player/Reward.h"

namespace ly
{
	class EnemySpaceShip : public SpaceShip
	{
	public:
		EnemySpaceShip(World* owningWorld, const std::string& texturePath, float collisionDamage=75.f, 
			const List<WeightedReward>& weightedRewards = {});

		virtual void Tick(float deltaTime) override;
		virtual void SetupCollisionLayers() override;

		virtual void SetScoreAmt(unsigned int scoreAmt) { mScoreAmt = scoreAmt; };
		unsigned int GetScoreAmt() const { return mScoreAmt; };
		
		float GetCollisionDamage() const { return mCollisionDamage; }

		// Event: Enemy defeated, score awarded
		Delegate<unsigned int> onScoreAwarded;

	protected:
		// Derived classes override this to provide custom reward tables
		static List<WeightedReward> GetDefaultRewards();
		void SetCollisionDamage(float damage) { mCollisionDamage = damage; }

	private:
		virtual void OnActorBeginOverlap(Actor* other) override;
		void SpawnReward();
		virtual void Blew() override;
		
		float mCollisionDamage;
		List<WeightedReward> mWeightedRewards;
		unsigned int mScoreAmt;
	};
}