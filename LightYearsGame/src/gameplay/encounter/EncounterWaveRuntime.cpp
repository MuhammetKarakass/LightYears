#include "gameplay/encounter/EncounterWaveRuntime.h"

#include "enemy/EnemyActor.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		uint32_t HashCombine(uint32_t seed, uint32_t value)
		{
			seed ^= value + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
			return seed;
		}

		uint32_t HashContentId(const sas::ContentId& contentId)
		{
			uint32_t hash = 2166136261u;
			for (const char character : contentId.ToString()) hash = (hash ^ static_cast<uint8_t>(character)) * 16777619u;
			return hash;
		}

		bool IsInactiveEnemy(const weak_ptr<EnemyActor>& enemy)
		{
			const shared_ptr<EnemyActor> actor = enemy.lock();
			return !actor || actor->GetIsPendingDestroy();
		}
	}

	bool EncounterWaveRuntime::Start(
		List<EnemyWaveDefinition> definitions,
		EnemySpawnFunction spawnEnemy,
		const EncounterProgression& progression,
		std::string* failureReason)
	{
		if (mState != EncounterWaveState::Idle)
		{
			if (failureReason) *failureReason = "Encounter wave runtime must be reset before starting again.";
			return false;
		}
		if (!spawnEnemy)
		{
			if (failureReason) *failureReason = "Encounter wave runtime requires an enemy spawn callback.";
			return false;
		}
		if (!ValidateDefinitions(definitions, failureReason) || !ValidateProgression(progression, failureReason)) return false;

		mDefinitions = std::move(definitions);
		mSpawnEnemy = std::move(spawnEnemy);
		mProgression = progression;
		mFailureReason.clear();
		BeginWave(0);
		return true;
	}

	void EncounterWaveRuntime::Tick(float deltaTime)
	{
		if (mState == EncounterWaveState::Idle || mState == EncounterWaveState::Completed ||
			mState == EncounterWaveState::Failed)
		{
			return;
		}

		RemoveInactiveOwnedEnemies();

		float remainingTime = std::isfinite(deltaTime) ? std::max(0.f, deltaTime) : 0.f;
		for (int transitions = 0; transitions < 1024; ++transitions)
		{
			if (mState == EncounterWaveState::Spawning)
			{
				if (mCurrentEntryIndex >= mDefinitions[mCurrentWaveIndex].entries.size())
				{
					mState = EncounterWaveState::WaitingForClear;
					continue;
				}
				if (mSpawnTimer > 0.f)
				{
					const float elapsed = std::min(mSpawnTimer, remainingTime);
					mSpawnTimer -= elapsed;
					remainingTime -= elapsed;
					if (mSpawnTimer > 0.f) return;
				}

				const EnemyWaveSpawnEntry& entry = mDefinitions[mCurrentWaveIndex].entries[mCurrentEntryIndex];
				weak_ptr<EnemyActor> spawned = mSpawnEnemy(entry.enemyId, mWaveSpawnIndex, ResolveSpawnContext(entry.enemyId));
				if (spawned.expired())
				{
					Fail("Encounter wave failed to spawn enemy '" + entry.enemyId.ToString() + "'.");
					return;
				}
				mOwnedEnemies.push_back(std::move(spawned));
				++mEntrySpawnCount;
				++mWaveSpawnIndex;
				if (mEntrySpawnCount >= ResolveEntryCount(mDefinitions[mCurrentWaveIndex], mCurrentEntryIndex))
				{
					++mCurrentEntryIndex;
					mEntrySpawnCount = 0;
				}
				else mSpawnTimer = entry.spawnInterval;
				continue;
			}

			if (mState == EncounterWaveState::WaitingForClear)
			{
				if (CountLiveOwnedEnemies() > 0) return;
				if (mCurrentWaveIndex + 1 >= mDefinitions.size())
				{
					mState = EncounterWaveState::Completed;
					continue;
				}
				mInterWaveTimer = mDefinitions[mCurrentWaveIndex].nextWaveDelay;
				mState = EncounterWaveState::InterWaveDelay;
				continue;
			}

			if (mState == EncounterWaveState::InterWaveDelay)
			{
				const float elapsed = std::min(mInterWaveTimer, remainingTime);
				mInterWaveTimer -= elapsed;
				remainingTime -= elapsed;
				if (mInterWaveTimer > 0.f) return;
				AdvanceAfterClear();
				continue;
			}
			return;
		}

		Fail("Encounter wave runtime exceeded its state-transition safety limit.");
	}

	EncounterWaveSnapshot EncounterWaveRuntime::BuildSnapshot() const
	{
		EncounterWaveSnapshot snapshot;
		snapshot.state = mState;
		snapshot.totalWaveCount = mDefinitions.size();
		snapshot.aliveEnemyCount = CountLiveOwnedEnemies();
		if (mState == EncounterWaveState::InterWaveDelay) snapshot.interWaveRemainingTime = mInterWaveTimer;
		if (mState == EncounterWaveState::Idle || mCurrentWaveIndex >= mDefinitions.size()) return snapshot;

		snapshot.currentWaveNumber = mCurrentWaveIndex + 1;
		snapshot.plannedEnemyCount = ResolveWaveEnemyCount(mDefinitions[mCurrentWaveIndex]);
		snapshot.spawnedEnemyCount = static_cast<int>(mWaveSpawnIndex);
		snapshot.remainingSpawnCount = std::max(0, snapshot.plannedEnemyCount - snapshot.spawnedEnemyCount);
		snapshot.enemyLevel = ResolveWaveEnemyLevel();
		return snapshot;
	}

	void EncounterWaveRuntime::Reset()
	{
		mDefinitions.clear();
		mProgression = {};
		mSpawnEnemy = {};
		mOwnedEnemies.clear();
		mState = EncounterWaveState::Idle;
		mCurrentWaveIndex = 0;
		mCurrentEntryIndex = 0;
		mEntrySpawnCount = 0;
		mWaveSpawnIndex = 0;
		mSpawnTimer = 0.f;
		mInterWaveTimer = 0.f;
		mFailureReason.clear();
	}

	bool EncounterWaveRuntime::ValidateDefinitions(
		const List<EnemyWaveDefinition>& definitions,
		std::string* failureReason) const
	{
		if (definitions.empty())
		{
			if (failureReason) *failureReason = "Encounter wave runtime requires at least one wave.";
			return false;
		}
		for (const EnemyWaveDefinition& wave : definitions)
		{
			if (wave.entries.empty() || !std::isfinite(wave.nextWaveDelay) || wave.nextWaveDelay < 0.f)
			{
				if (failureReason) *failureReason = "Encounter wave has invalid entries or next-wave delay.";
				return false;
			}
			for (const EnemyWaveSpawnEntry& entry : wave.entries)
			{
				if (!entry.enemyId.IsValid() || entry.count <= 0 || !std::isfinite(entry.spawnInterval) ||
					entry.spawnInterval < 0.f)
				{
					if (failureReason) *failureReason = "Encounter wave spawn entry has an invalid enemy ID, count, or interval.";
					return false;
				}
			}
		}
		return true;
	}

	bool EncounterWaveRuntime::ValidateProgression(const EncounterProgression& progression, std::string* failureReason) const
	{
		if (progression.additionalEnemyEveryWaves <= 0 || progression.enemyLevelEveryWaves <= 0 ||
			progression.maximumAdditionalEnemies < 0 || progression.maximumEnemyLevel < 1)
		{
			if (failureReason) *failureReason = "Encounter progression has invalid intervals or caps.";
			return false;
		}
		return true;
	}

	int EncounterWaveRuntime::ResolveEntryCount(const EnemyWaveDefinition& wave, size_t entryIndex) const
	{
		const int additional = std::min(static_cast<int>(mCurrentWaveIndex) / mProgression.additionalEnemyEveryWaves,
			mProgression.maximumAdditionalEnemies);
		const int distributed = (additional + static_cast<int>(wave.entries.size()) - 1 - static_cast<int>(entryIndex)) /
			static_cast<int>(wave.entries.size());
		return wave.entries[entryIndex].count + std::max(0, distributed);
	}

	int EncounterWaveRuntime::ResolveWaveEnemyCount(const EnemyWaveDefinition& wave) const
	{
		int totalCount = 0;
		for (size_t entryIndex = 0; entryIndex < wave.entries.size(); ++entryIndex)
			totalCount += ResolveEntryCount(wave, entryIndex);
		return totalCount;
	}

	int EncounterWaveRuntime::ResolveWaveEnemyLevel() const
	{
		return std::min(1 + static_cast<int>(mCurrentWaveIndex) / mProgression.enemyLevelEveryWaves,
			mProgression.maximumEnemyLevel);
	}

	EnemySpawnContext EncounterWaveRuntime::ResolveSpawnContext(const sas::ContentId& enemyId) const
	{
		uint32_t seed = HashCombine(mProgression.seed, static_cast<uint32_t>(mCurrentWaveIndex));
		seed = HashCombine(seed, static_cast<uint32_t>(mWaveSpawnIndex));
		return { ResolveWaveEnemyLevel(), HashCombine(seed, HashContentId(enemyId)) };
	}

	void EncounterWaveRuntime::Fail(const std::string& failureReason)
	{
		mState = EncounterWaveState::Failed;
		mFailureReason = failureReason;
	}

	void EncounterWaveRuntime::RemoveInactiveOwnedEnemies()
	{
		mOwnedEnemies.erase(
			std::remove_if(mOwnedEnemies.begin(), mOwnedEnemies.end(), IsInactiveEnemy),
			mOwnedEnemies.end()
		);
	}

	int EncounterWaveRuntime::CountLiveOwnedEnemies() const
	{
		return static_cast<int>(std::count_if(
			mOwnedEnemies.begin(),
			mOwnedEnemies.end(),
			[](const weak_ptr<EnemyActor>& enemy) { return !IsInactiveEnemy(enemy); }
		));
	}

	void EncounterWaveRuntime::BeginWave(size_t waveIndex)
	{
		mCurrentWaveIndex = waveIndex;
		mCurrentEntryIndex = 0;
		mEntrySpawnCount = 0;
		mWaveSpawnIndex = 0;
		mSpawnTimer = 0.f;
		mInterWaveTimer = 0.f;
		mOwnedEnemies.clear();
		mState = EncounterWaveState::Spawning;
	}

	void EncounterWaveRuntime::AdvanceAfterClear()
	{
		const size_t nextWaveIndex = mCurrentWaveIndex + 1;
		if (nextWaveIndex >= mDefinitions.size())
		{
			mState = EncounterWaveState::Completed;
			return;
		}
		BeginWave(nextWaveIndex);
	}
}
