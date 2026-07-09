#pragma once

#include "framework/Core.h"
#include "player/PlayerRespawnDefinition.h"
#include "framework/Delegate.h"
#include "framework/TimerManager.h"

namespace ly
{
	class Actor;
	class Object;
	class Player;
	class PlayerSpaceShip;
	class World;

	class PlayerRespawnSystem
	{
	public:

		void Initialize(World* world, weak_ptr<Object> timerOwner, const PlayerRespawnDefinition& respawnDefinition);
		void Clear();

		weak_ptr<PlayerSpaceShip> SpawnInitialPlayerShip();
		void ScheduleRespawn();

		weak_ptr<PlayerSpaceShip> GetCurrentPlayerShip() const { return mCurrentPlayerShip; };

		Delegate<weak_ptr<PlayerSpaceShip>> onPlayerShipSpawned;
		Delegate<Actor*> onPlayerShipDestroyed;
		Delegate<> onRespawnScheduled;
		Delegate<> onRespawnFailed;

	private:
		Player* GetOrCreatePlayer() const;
		weak_ptr<PlayerSpaceShip> SpawnPlayerShip();
		void HandlePlayerShipDestroyed(Actor* destroyedActor);
		void FailRespawn();

		World* mWorld = nullptr;
		weak_ptr<Object> mTimerOwner;
		PlayerRespawnDefinition mRespawnDefinition;
		weak_ptr<PlayerSpaceShip> mCurrentPlayerShip;
		TimerHandle mRespawnTimerHandle;
		bool mHasPendingRespawn = false;
	};
}