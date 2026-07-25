#pragma once
#include <spaceShip/SpaceShip.h>
#include "player/Reward.h"

namespace ly
{
	class EnemySpaceShip : public SpaceShip
	{
	public:
		EnemySpaceShip(World* owningWorld, const ShipDefinition& shipDef);

		virtual void Tick(float deltaTime) override;
		virtual void SetupCollisionLayers() override;

		virtual void SetScoreAmt(unsigned int scoreAmt) { mScoreAmt = scoreAmt; };
		unsigned int GetScoreAmt() const { return mScoreAmt; };
		virtual void SetShipXPReward(float amount) { mShipXPReward = amount; }
		float GetShipXPReward() const { return mShipXPReward; }
		
		float GetCollisionDamage() const { return mCollisionDamage; }

		Delegate<unsigned int> onScoreAwarded;
		Delegate<float> onShipXPAwarded;

	protected:
		void SetCollisionDamage(float damage) { mCollisionDamage = damage; }

	private:
		virtual void OnActorBeginOverlap(Actor* otherActor) override;
		void SpawnReward();
		virtual void Blew() override;
		
		float mCollisionDamage;
		List<WeightedReward> mWeightedRewards;
		unsigned int mScoreAmt;
		float mShipXPReward;
	};
}

