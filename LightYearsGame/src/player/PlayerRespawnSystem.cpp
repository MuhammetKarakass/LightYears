#include "player/PlayerRespawnSystem.h"

#include "framework/Actor.h"
#include "framework/World.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"

namespace ly
{
	PlayerRespawnSystem::~PlayerRespawnSystem()
	{
		Clear();
	}

	void PlayerRespawnSystem::Initialize(World* world, weak_ptr<Object> timerOwner, const PlayerRespawnDefinition& respawnDefinition)
	{
		Clear();
		mWorld = world;
		mTimerOwner = timerOwner;
		mRespawnDefinition = respawnDefinition;
	}
	void PlayerRespawnSystem::Clear()
	{
		TimerManager::GetGameTimerManager().ClearTimer(mRespawnTimerHandle);
		if (const shared_ptr<PlayerSpaceShip> playerShip = mCurrentPlayerShip.lock(); playerShip && mPlayerShipDestroyedDelegateHandle.IsValid())
		{
			playerShip->onActorDestroyed.UnbindAction(mPlayerShipDestroyedDelegateHandle);
		}
		mPlayerShipDestroyedDelegateHandle.Reset();
		mHasPendingRespawn = false;
		mCurrentPlayerShip.reset();
	}
	weak_ptr<PlayerSpaceShip> PlayerRespawnSystem::SpawnInitialPlayerShip()
	{
		return SpawnPlayerShip();
	}
	void PlayerRespawnSystem::ScheduleRespawn()
	{
		if (mHasPendingRespawn)
			return;
		if(!mRespawnDefinition.respawnWhenDestroyed)
		{
			FailRespawn();
			return;
		}

		mHasPendingRespawn = true;
		onRespawnScheduled.Broadcast();

		mRespawnTimerHandle = TimerManager::GetGameTimerManager().SetTimer(
			mTimerOwner,
			[this]()
			{
				mHasPendingRespawn = false;
				SpawnPlayerShip();
			},mRespawnDefinition.respawnDelay,false
			);
	}
	Player* PlayerRespawnSystem::GetOrCreatePlayer() const
	{
		PlayerManager& playerManager = PlayerManager::GetPlayerManager();
		if(Player* player = playerManager.GetPlayer(mRespawnDefinition.playerIndex))
		{
			return player;
		}

		if (mRespawnDefinition.playerIndex == 0 && mRespawnDefinition.createPlayerIfMissing)
		{
			return &playerManager.CreateNewPlayer();
		}
		return nullptr;
	}
	weak_ptr<PlayerSpaceShip> PlayerRespawnSystem::SpawnPlayerShip()
	{
		if (!mWorld)
		{
			FailRespawn();
			return weak_ptr<PlayerSpaceShip>();
		}

		Player* player = GetOrCreatePlayer();
		if (!player)
		{
			FailRespawn();
			return weak_ptr<PlayerSpaceShip>{};
		}

		mCurrentPlayerShip = player->SpawnSpaceShip(mWorld);

		if (auto playerShip = mCurrentPlayerShip.lock())
		{
			playerShip->SetActorLocation(mRespawnDefinition.spawnLocation);
			playerShip->SetUseScreenClamp(mRespawnDefinition.useScreenClamp);
			mPlayerShipDestroyedDelegateHandle = playerShip->onActorDestroyed.BindAction(
				this, &PlayerRespawnSystem::HandlePlayerShipDestroyed
			);

			onPlayerShipSpawned.Broadcast(mCurrentPlayerShip);
			return mCurrentPlayerShip;
		}

		FailRespawn();
		return weak_ptr<PlayerSpaceShip>{};
	}
	void PlayerRespawnSystem::HandlePlayerShipDestroyed(Actor* destroyedActor)
	{
		if (auto currentPlayerShip = mCurrentPlayerShip.lock())
		{
			if (currentPlayerShip.get() != destroyedActor)
			{
				return;
			}
		}

		mCurrentPlayerShip.reset();
		mPlayerShipDestroyedDelegateHandle.Reset();
		onPlayerShipDestroyed.Broadcast(destroyedActor);
		ScheduleRespawn();
	}
	void PlayerRespawnSystem::FailRespawn()
	{
		TimerManager::GetGameTimerManager().ClearTimer(mRespawnTimerHandle);
		mHasPendingRespawn = false;
		mPlayerShipDestroyedDelegateHandle.Reset();
		mCurrentPlayerShip.reset();
		onRespawnFailed.Broadcast();
	}
}


