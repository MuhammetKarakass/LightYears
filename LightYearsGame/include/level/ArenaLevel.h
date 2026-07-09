#pragma once

#include "level/GameLevel.h"
#include "level/ArenaDefinition.h"
#include "level/ArenaBoundarySystem.h"
#include "framework/TimerManager.h"
#include "framework/Delegate.h"
#include "framework/camera/CameraManager.h"
#include "player/PlayerRespawnDefinition.h"
#include "player/PlayerRespawnSystem.h"

namespace ly
{
	class ArenaBoundaryIndicator;
	class PlayerSpaceShip;

	class ArenaLevel : public GameLevel
	{
	public:
		ArenaLevel(Application* owningApp);
	protected:
		void InitializeLevelSystems() override;

		virtual void Tick(float deltaTime) override;

		virtual void OnGameStart() override;
		virtual PlayerRespawnDefinition CreatePlayerRespawnDefinition() const;
		virtual CameraSettings CreateCameraSettings() const;

		virtual ArenaDefinition CreateArenaDefinition() const;
		virtual bool ShouldUseArenaBoundaryIndicator() const;

		bool HasArenaDefinition() const { return mHasArenaDefinition; };
		const ArenaDefinition& GetArenaDefinition() const { return mArenaDefinition; };

		void SetArenaTrackedActor(weak_ptr<Actor> trackedActor);

		weak_ptr<ArenaBoundaryIndicator> GetArenaBoundaryIndicator() const { return mArenaBoundaryIndicator; }

		virtual void OnArenaBoundaryPenaltyTriggered(weak_ptr<Actor> trackedActor);
	private:
		void InitializeArena();
		void InitializeArenaCamera();
		void UpdateArenaCameraInputs();
		void SpawnArenaBoundaryIndicator();
		void InitializeArenaBoundarySystem();
		void UpdateArenaBoundaryVisuals();
		void ApplyArenaBoundaryPenalty(weak_ptr<Actor> trackedActor);

		void InitializePlayerRespawnSystem();
		void StartPlayerRespawn();

		void OnPlayerShipSpawned(weak_ptr<PlayerSpaceShip> playerShip);
		void OnPlayerShipDestroyed(Actor* destroyedActor);
		void OnPlayerRespawnFailed();

		void OnArenaBoundaryWarningUpdated(float remainingTime, float totalTime);
		void OnArenaBoundaryWarningCleared();

		ArenaDefinition mArenaDefinition;
		bool mHasArenaDefinition = false;

		ArenaBoundarySystem mArenaBoundarySystem;
		weak_ptr<ArenaBoundaryIndicator> mArenaBoundaryIndicator;
		TimerHandle mBoundaryPenaltyTimerHandle;

		PlayerRespawnDefinition mPlayerRespawnDefinition;
		PlayerRespawnSystem mPlayerRespawnSystem;
		weak_ptr<PlayerSpaceShip> mCameraFollowShip;
	};
}
