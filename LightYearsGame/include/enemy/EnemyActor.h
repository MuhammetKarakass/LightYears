#pragma once

#include "spaceShip/SpaceShip.h"
#include "gameplay/enemy/EnemyBehaviorRuntime.h"
#include "gameplay/enemy/EnemyRuntime.h"
#include "gameplay/enemy/EnemySpawnContext.h"
#include "player/Reward.h"

namespace ly
{
	class EnemyActor final : public SpaceShip
	{
	public:
		EnemyActor(World* owningWorld, const ShipDefinition& shipDefinition, const std::string& enemyId,
			const EnemyCombatProfile& combatProfile, const EnemyBehaviorProfile& behaviorProfile,
			float encounterDamageMultiplier = 1.f, const EnemySpawnContext& spawnContext = {});
		~EnemyActor() override;

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void SetupCollisionLayers() override;

		const std::string& GetEnemyId() const noexcept { return mEnemyId; }
		const std::string& GetCombatProfileId() const noexcept { return mCombatProfileId; }
		int GetEnemyLevel() const noexcept { return mSpawnContext.level; }
		EnemyRuntime& GetEnemyRuntime() { return mEnemyRuntime; }
		const EnemyRuntime& GetEnemyRuntime() const { return mEnemyRuntime; }
		EnemyBehaviorRuntime& GetEnemyBehaviorRuntime() { return mBehaviorRuntime; }
		const EnemyBehaviorRuntime& GetEnemyBehaviorRuntime() const { return mBehaviorRuntime; }

		void SetScoreAmt(unsigned int scoreAmt) { mScoreAmt = scoreAmt; }
		unsigned int GetScoreAmt() const noexcept { return mScoreAmt; }
		void SetShipXPReward(float amount) { mShipXPReward = amount; }
		float GetShipXPReward() const noexcept { return mShipXPReward; }
		float GetCollisionDamage() const noexcept { return mCollisionDamage; }

		Delegate<unsigned int> onScoreAwarded;
		Delegate<float> onShipXPAwarded;

	private:
		void ResetControlIntents();
		void ApplyBehaviorIntent(const EnemyBehaviorIntent& intent, float deltaTime);
		void OnActorBeginOverlap(Actor* otherActor) override;
		void SpawnReward();
		void Blew() override;

		EnemyRuntime mEnemyRuntime;
		EnemyBehaviorRuntime mBehaviorRuntime;
		EnemyCombatProfile mCombatProfile;
		EnemyBehaviorProfile mBehaviorProfile;
		std::string mEnemyId;
		std::string mCombatProfileId;
		float mCollisionDamage = 0.f;
		List<WeightedReward> mWeightedRewards;
		unsigned int mScoreAmt = 0;
		float mShipXPReward = 0.f;
		float mEncounterDamageMultiplier = 1.f;
		EnemySpawnContext mSpawnContext;
		bool mWindowCullEnabled = true;
		bool mHasChassisPrimaryWeapon = false;
	};
}
