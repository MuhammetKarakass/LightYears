#pragma once

#include "gameplay/encounter/EncounterWaveRuntime.h"
#include "level/ArenaLevel.h"

#include <cstdint>
#include <string>

namespace ly
{
	enum class ArenaEncounterState : uint8_t
	{
		Idle,
		Running,
		Completed,
		Failed
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

	protected:
		virtual ArenaDefinition CreateArenaDefinition() const override;
		virtual PlayerRespawnDefinition CreatePlayerRespawnDefinition() const override;
		virtual void CreateHUDControllers() override;
		virtual void Tick(float deltaTime) override;
		virtual void OnGameStart() override;
		virtual void OnRestartLevel() override;
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

		EncounterWaveRuntime mEncounterWaveRuntime;
		ArenaEncounterState mEncounterState = ArenaEncounterState::Idle;
		weak_ptr<Actor> mEncounterPlayer;
		DelegateHandle mEncounterPlayerDestroyedDelegateHandle;
		bool mEncounterFailureReported = false;
	};
}


