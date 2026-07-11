#include "level/ArenaLevel.h"
#include "level/ArenaBoundaryIndicator.h"
#include "player/PlayerSpaceShip.h"
#include "framework/TimerManager.h"

namespace ly
{ 
	     ArenaLevel::ArenaLevel(Application* owningApp):
	         GameLevel(owningApp),
		 mArenaDefinition{},
		 mHasArenaDefinition{ false },
         mArenaBoundaryIndicator{},
         mArenaBoundarySystem{},
         mPlayerRespawnDefinition{},
         mPlayerRespawnSystem{},
         mCameraFollowShip{}
     {
     }
     
     void ArenaLevel::InitializeLevelSystems()
     {
		 GameLevel::InitializeLevelSystems();
		 InitializeArena();
         InitializeArenaCamera();
         InitializePlayerRespawnSystem();
     }

     void ArenaLevel::Tick(float deltaTime)
     {
         GameLevel::Tick(deltaTime);

		 mArenaBoundarySystem.Tick(deltaTime);
		 UpdateArenaBoundaryVisuals();
         UpdateArenaCameraInputs();
     }

     void ArenaLevel::OnGameStart()
     {
         GameLevel::OnGameStart();

         StartPlayerRespawn();
     }

     PlayerRespawnDefinition ArenaLevel::CreatePlayerRespawnDefinition() const
     {
         PlayerRespawnDefinition respawnDefinition;

         respawnDefinition.useScreenClamp = false;

         if (mHasArenaDefinition)
         {
             const sf::FloatRect& legalBounds = mArenaDefinition.legalBounds;

             respawnDefinition.spawnLocation = sf::Vector2f{
                 legalBounds.position.x + legalBounds.size.x * 0.5f,
                 legalBounds.position.y + legalBounds.size.y * 0.5f
             };
         }
         return respawnDefinition;
     }

     CameraSettings ArenaLevel::CreateCameraSettings() const
     {
         return CameraSettings{};
     }
     
     ArenaDefinition ArenaLevel::CreateArenaDefinition() const
     {
     	return ArenaDefinition();
     }
     
     bool ArenaLevel::ShouldUseArenaBoundaryIndicator() const
     {
     	return mHasArenaDefinition && mArenaDefinition.boundaryVisual.enabled;
     }
     
     void ArenaLevel::SetArenaTrackedActor(weak_ptr<Actor> trackedActor)
     {
		 mArenaBoundarySystem.SetTrackedActor(trackedActor);
     }


     void ArenaLevel::InitializeArena()
     {
         if (mHasArenaDefinition)
         {
             return;
         }

         mArenaDefinition = CreateArenaDefinition();
         mHasArenaDefinition = true;

         InitializeArenaBoundarySystem();

         if (ShouldUseArenaBoundaryIndicator())
         {
             SpawnArenaBoundaryIndicator();
         }
     }

     void ArenaLevel::InitializeArenaCamera()
     {
         SetCameraSettings(CreateCameraSettings());

         if (mHasArenaDefinition)
         {
             SetCameraWorldBounds(mArenaDefinition.legalBounds);
         }
     }

     void ArenaLevel::UpdateArenaCameraInputs()
     {
         auto ship = mCameraFollowShip.lock();
         if (!ship || ship->GetIsPendingDestroy())
         {
             ClearCameraExternalVelocity();
             ClearCameraLookAheadWorldPosition();
             return;
         }

         SetCameraExternalVelocity(ship->GetVelocity());
         SetCameraLookAheadWorldPosition(GetMouseWorldPosition());
     }
     
     void ArenaLevel::SpawnArenaBoundaryIndicator()
     {
         if (!mHasArenaDefinition || !mArenaBoundaryIndicator.expired())
             return;
		 mArenaBoundaryIndicator = SpawnActor<ArenaBoundaryIndicator>(mArenaDefinition);
     }
     void ArenaLevel::InitializeArenaBoundarySystem()
     {
         if(!mHasArenaDefinition)
             return;

         mArenaBoundarySystem.Initialize(mArenaDefinition);
         mArenaBoundarySystem.onBoundaryPenaltyTriggered.BindAction(GetWeakPtr(), &ArenaLevel::OnArenaBoundaryPenaltyTriggered);
         mArenaBoundarySystem.onBoundaryWarningUpdated.BindAction(GetWeakPtr(), &ArenaLevel::OnArenaBoundaryWarningUpdated);
         mArenaBoundarySystem.onBoundaryWarningCleared.BindAction(GetWeakPtr(), &ArenaLevel::OnArenaBoundaryWarningCleared);
     }
     void ArenaLevel::UpdateArenaBoundaryVisuals()
     {
         if (auto indicator = mArenaBoundaryIndicator.lock())
         {
             indicator->SetWarningActive(mArenaBoundarySystem.IsWarningActive());
         }
     }

     void ArenaLevel::OnArenaBoundaryPenaltyTriggered(weak_ptr<Actor> trackedActor)
     {
         TimerManager::GetGameTimerManager().ClearTimer(mBoundaryPenaltyTimerHandle);

         mBoundaryPenaltyTimerHandle = TimerManager::GetGameTimerManager().SetTimer(
             GetWeakPtr(),
             [this, trackedActor]()
             {
                 ApplyArenaBoundaryPenalty(trackedActor);
             },
             0.01f,
             false
         );
     }

     void ArenaLevel::ApplyArenaBoundaryPenalty(weak_ptr<Actor> trackedActor)
     {
         OnArenaBoundaryWarningCleared();

         auto actor = trackedActor.lock();
         if (!actor || actor->GetIsPendingDestroy())
         {
             return;
         }

         if (auto playerSpaceShip = dynamic_cast<PlayerSpaceShip*>(actor.get()))
         {
             float bonusHealth = 0.f;
             for (const GameplayEffectSnapshot& effectSnapshot : playerSpaceShip->GetCombatRuntime().GetEffects().BuildSnapshots())
             {
                 bonusHealth += FindGameplayAttributeValue(
                     effectSnapshot.runtimeAttributes,
                     BarrierEffectSchema::Capacity,
                     0.f
                 );
             }
             const float lethalDamage = playerSpaceShip->GetHealthComponent().GetHealth() + bonusHealth + 1.f;

             playerSpaceShip->SetInvulnerability(false);
             playerSpaceShip->ApplyDamage(lethalDamage);
             return;
         }

         actor->ApplyDamage(9999999.f);
     }
     void ArenaLevel::InitializePlayerRespawnSystem()
     {
         mPlayerRespawnDefinition = CreatePlayerRespawnDefinition();

         mPlayerRespawnSystem.Initialize(this, GetWeakPtr(),mPlayerRespawnDefinition);
         mPlayerRespawnSystem.onPlayerShipSpawned.BindAction
         (GetWeakPtr(), &ArenaLevel::OnPlayerShipSpawned);
         mPlayerRespawnSystem.onPlayerShipDestroyed.BindAction
         (GetWeakPtr(), &ArenaLevel::OnPlayerShipDestroyed);
         mPlayerRespawnSystem.onRespawnFailed.BindAction
         (GetWeakPtr(), &ArenaLevel::OnPlayerRespawnFailed);
     }
     void ArenaLevel::StartPlayerRespawn()
     {
         mPlayerRespawnSystem.SpawnInitialPlayerShip();
     }
     void ArenaLevel::OnPlayerShipSpawned(weak_ptr<PlayerSpaceShip> playerShip)
     {
         mCameraFollowShip = playerShip;

         if (auto ship = playerShip.lock())
         {
             ship->SetMovementMode(ShipMovementMode::ThrustDrift);
         }

         SetViewTarget(playerShip);
         SetArenaTrackedActor(playerShip);
     }
     void ArenaLevel::OnPlayerShipDestroyed(Actor* destroyedActor)
     {
         mCameraFollowShip.reset();
         ClearViewTarget();
         ClearCameraExternalVelocity();
         ClearCameraLookAheadWorldPosition();
         SetArenaTrackedActor(weak_ptr<Actor>());
         OnArenaBoundaryWarningCleared();
     }
     void ArenaLevel::OnPlayerRespawnFailed()
     {
         if (mPlayerRespawnDefinition.gameOverWhenRespawnFails)
         {
             GameOver();
         }
     }
     void ArenaLevel::OnArenaBoundaryWarningUpdated(float remainingTime, float totalTime)
     {
         GameplayWarning warning;
         warning.type = GameplayWarningType::ArenaBoundary;
         warning.title = "UNAUTHORIZED REGION";
         warning.message = "RETURN IN";
         warning.remainingTime = remainingTime;
         warning.totalTime = totalTime;
         warning.hasCountdown = true;

         BroadcastGameplayWarning(warning);
     }
     void ArenaLevel::OnArenaBoundaryWarningCleared()
     {
         ClearGameplayWarning(GameplayWarningType::ArenaBoundary);
     }
}


