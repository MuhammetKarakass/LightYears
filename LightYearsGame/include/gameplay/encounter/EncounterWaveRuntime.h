#pragma once

#include "content/ContentId.h"
#include "framework/Core.h"
#include "gameplay/enemy/EnemySpawnContext.h"

#include <functional>
#include <optional>
#include <string>

namespace ly
{
	class EnemyActor;

	struct EnemyWaveSpawnEntry
	{
		sas::ContentId enemyId;
		int count = 1;
		float spawnInterval = 0.f;
	};

	struct EnemyWaveDefinition
	{
		List<EnemyWaveSpawnEntry> entries;
		float nextWaveDelay = 1.f;
	};

	struct EncounterProgression
	{
		// Finite count progression only; the endless target-total path ignores it.
		int additionalEnemyEveryWaves = 2;
		// Shared: level timing and the endless count-drop event both derive from this.
		int enemyLevelEveryWaves = 2;
		// Finite count progression only; the endless target-total path ignores it.
		int maximumAdditionalEnemies = 0;
		// Shared: stat progression cap and the point where endless count drops stop.
		int maximumEnemyLevel = 1;
		uint32_t seed = 0;
		// Technical safety cap; zero preserves the uncapped runtime behavior.
		int maximumConcurrentEnemies = 0;
	};

	enum class EncounterWaveState
	{
		Idle,
		Spawning,
		WaitingForClear,
		InterWaveDelay,
		Completed,
		Failed
	};

	// Sequence lifetime, not numerical progression: a finite encounter consumes its
	// authored definitions once, an endless encounter reuses them as cyclic
	// composition templates while the absolute wave index keeps increasing.
	enum class EncounterSequenceMode
	{
		Finite,
		EndlessCycle
	};

	// Read-only projection of runtime wave state; owning runtime remains EncounterWaveRuntime.
	struct EncounterWaveSnapshot
	{
		EncounterWaveState state = EncounterWaveState::Idle;
		EncounterSequenceMode sequenceMode = EncounterSequenceMode::Finite;
		size_t currentWaveNumber = 0;
		// nullopt while idle and for every endless encounter: an endless sequence has
		// no total wave count, and a sentinel value would leak into HUD/analytics math.
		std::optional<size_t> totalWaveCount;
		int plannedEnemyCount = 0;
		int spawnedEnemyCount = 0;
		int aliveEnemyCount = 0;
		int remainingSpawnCount = 0;
		float interWaveRemainingTime = 0.f;
		int enemyLevel = 1;
	};

	using EnemySpawnFunction = std::function<weak_ptr<EnemyActor>(const sas::ContentId&, size_t, const EnemySpawnContext&)>;

	class EncounterWaveRuntime
	{
	public:
		bool Start(List<EnemyWaveDefinition> definitions, EnemySpawnFunction spawnEnemy, const EncounterProgression& progression = {},
			EncounterSequenceMode sequenceMode = EncounterSequenceMode::Finite, std::string* failureReason = nullptr);
		void Tick(float deltaTime);
		void Reset();

		EncounterWaveState GetState() const { return mState; }
		bool IsCompleted() const { return mState == EncounterWaveState::Completed; }
		bool HasFailed() const { return mState == EncounterWaveState::Failed; }
		const std::string& GetFailureReason() const { return mFailureReason; }
		size_t GetCurrentWaveIndex() const { return mCurrentWaveIndex; }
		const List<weak_ptr<EnemyActor>>& GetOwnedEnemies() const { return mOwnedEnemies; }
		EncounterWaveSnapshot BuildSnapshot() const;

	private:
		bool ValidateDefinitions(const List<EnemyWaveDefinition>& definitions, std::string* failureReason) const;
		bool ValidateProgression(const EncounterProgression& progression, std::string* failureReason) const;
		// Absolute wave index -> template index. Only definition lookup wraps in
		// endless mode; level, seed, telemetry and the HUD keep the absolute number.
		size_t ResolveTemplateIndex(size_t absoluteWaveIndex) const;
		const EnemyWaveDefinition& ResolveWaveDefinition(size_t absoluteWaveIndex) const;
		bool HasNextWave() const;
		int ResolveEntryCount(const EnemyWaveDefinition& wave, size_t entryIndex) const;
		int ResolveWaveEnemyCount(const EnemyWaveDefinition& wave) const;
		int ResolveWaveTargetEnemyCount() const;
		int ResolveWaveEnemyLevel() const;
		EnemySpawnContext ResolveSpawnContext(const sas::ContentId& enemyId) const;
		void Fail(const std::string& failureReason);
		void RemoveInactiveOwnedEnemies();
		int CountLiveOwnedEnemies() const;
		void BeginWave(size_t waveIndex);
		void AdvanceAfterClear();

		List<EnemyWaveDefinition> mDefinitions;
		EncounterProgression mProgression;
		EncounterSequenceMode mSequenceMode = EncounterSequenceMode::Finite;
		EnemySpawnFunction mSpawnEnemy;
		List<weak_ptr<EnemyActor>> mOwnedEnemies;
		EncounterWaveState mState = EncounterWaveState::Idle;
		size_t mCurrentWaveIndex = 0;
		size_t mCurrentEntryIndex = 0;
		int mEntrySpawnCount = 0;
		size_t mWaveSpawnIndex = 0;
		float mSpawnTimer = 0.f;
		float mInterWaveTimer = 0.f;
		std::string mFailureReason;
	};
}
