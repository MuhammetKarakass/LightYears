#pragma once

#include "gameplay/encounter/EncounterWaveRuntime.h"
#include "level/ArenaLevel.h"

#include <cstdint>
#include <string>

namespace ly
{
	class DamageContext;
	class EnemyActor;

	enum class ArenaEncounterState : uint8_t
	{
		Idle,
		Running,
		Completed,
		Failed
	};

	// Observational-only playtest measurement for one arena encounter run. This is
	// plain data: gameplay never reads it, it is never authoritative, and it is
	// reset for every run. Values are only ever written by the level while it
	// observes the encounter, the player, and its own spawn path.
	struct ArenaPlaytestWaveMetrics
	{
		size_t waveNumber = 0;
		double startTime = 0.0;
		double endTime = 0.0;
		double duration = 0.0;
		int spawned = 0;
		int killed = 0;
		int peakAlive = 0;
		double aliveIntegral = 0.0;
		double aliveSampledDuration = 0.0;
		bool started = false;
		bool ended = false;
	};

	struct ArenaPlaytestEnemyTypeMetrics
	{
		std::string enemyId;
		int kills = 0;
		int ttkSamples = 0;
		double totalTtk = 0.0;
	};

	struct ArenaPlaytestLiveEnemy
	{
		weak_ptr<EnemyActor> enemy;
		std::string enemyId;
		double spawnTime = 0.0;
		size_t waveNumber = 0;
	};

	struct ArenaPlaytestMetrics
	{
		bool active = false;
		bool completed = false;
		bool failed = false;
		int summaryEmitCount = 0;
		double duration = 0.0;

		List<ArenaPlaytestWaveMetrics> waves;

		int totalSpawned = 0;
		int totalKilled = 0;
		List<ArenaPlaytestEnemyTypeMetrics> enemyTypes;

		double totalDamageTaken = 0.0;
		double shieldDamageTaken = 0.0;
		double hullDamageTaken = 0.0;

		int reloadCount = 0;
		double totalReloadTime = 0.0;
		double damageWhileReloading = 0.0;

		int playerDeathCount = 0;
		size_t playerDeathWave = 0;
		double playerDeathTime = 0.0;
		bool hasDeathCause = false;
		std::string deathCauseDeliveryType;
		std::string deathCauseAbilityId;
		std::string deathCauseDamageTags;

		int boundaryPenaltyCount = 0;

		int peakAlive = 0;
		double aliveIntegral = 0.0;
		double aliveSampledDuration = 0.0;
	};

	class ArenaTestLevel : public ArenaLevel
	{
	public:
		ArenaTestLevel(Application* owningApp);
		~ArenaTestLevel() override;

		bool StartEncounter();
		bool RestartEncounter();
		ArenaEncounterState GetEncounterState() const { return mEncounterState; }
		EncounterWaveSnapshot GetEncounterWaveSnapshot() const { return mEncounterWaveRuntime.BuildSnapshot(); }
		const ArenaPlaytestMetrics& GetPlaytestMetrics() const { return mPlaytestMetrics; }

	protected:
		virtual ArenaDefinition CreateArenaDefinition() const override;
		virtual PlayerRespawnDefinition CreatePlayerRespawnDefinition() const override;
		virtual void CreateHUDControllers() override;
		virtual void Tick(float deltaTime) override;
		virtual void OnGameStart() override;
		virtual void OnRestartLevel() override;
		virtual void OnArenaBoundaryPenaltyTriggered(weak_ptr<Actor> trackedActor) override;
		virtual weak_ptr<PlayerSpaceShip> ResolveEncounterPlayerShip() const;

	private:
		void SpawnDummyTargets();
		bool BindEncounterPlayerDeath();
		void UnbindEncounterPlayerDeath();
		void HandleEncounterPlayerDestroyed(Actor* destroyedActor);
		void ResetEncounter();
		void FailEncounter(const std::string& failureReason);
		void DestroyEncounterEnemies();
		weak_ptr<EnemyActor> SpawnEncounterEnemy(const sas::ContentId& enemyId, size_t spawnIndex, const EnemySpawnContext& context);

		void ResetPlaytestMetrics();
		void SamplePlaytestMeasurements(double deltaTime);
		void SamplePlaytestWave(const EncounterWaveSnapshot& snapshot, double deltaTime, double frameStartTime);
		void SamplePlaytestReload(double deltaTime);
		void UpdatePlaytestEnemyRecords();
		void EnsurePlaytestPlayerDamageObservation();
		void UnbindPlaytestPlayerDamageObservation();
		void HandlePlaytestPlayerDamage(const DamageContext& context);
		void EmitPlaytestSummaryIfPending();
		void FinalizePlaytestTerminalState();
		ArenaPlaytestWaveMetrics& EnsurePlaytestWave(size_t waveNumber, double startTime);
		ArenaPlaytestWaveMetrics* FindPlaytestWave(size_t waveNumber);
		ArenaPlaytestEnemyTypeMetrics& GetOrCreatePlaytestEnemyType(const std::string& enemyId);
		void ClearPlaytestLiveEnemies();

		EncounterWaveRuntime mEncounterWaveRuntime;
		ArenaEncounterState mEncounterState = ArenaEncounterState::Idle;
		weak_ptr<Actor> mEncounterPlayer;
		DelegateHandle mEncounterPlayerDestroyedDelegateHandle;
		bool mEncounterFailureReported = false;

		ArenaPlaytestMetrics mPlaytestMetrics;
		List<ArenaPlaytestLiveEnemy> mPlaytestLiveEnemies;
		DelegateHandle mPlaytestDamageDelegateHandle;
		weak_ptr<PlayerSpaceShip> mPlaytestDamageShip;
		bool mPlaytestWasReloading = false;
		bool mPlaytestSummaryPending = false;
		int mPlaytestTerminalFinalizeFrames = 0;
		static constexpr int MaxPlaytestTerminalFinalizeFrames = 8;
	};
}


