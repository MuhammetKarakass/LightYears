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
         mArenaBoundarySystem{}
     {
     }
     
     void ArenaLevel::InitializeLevelSystems()
     {
		 GameLevel::InitializeLevelSystems();
		 InitializeArena();
     }

     void ArenaLevel::Tick(float deltaTime)
     {
         GameLevel::Tick(deltaTime);

		 mArenaBoundarySystem.Tick(deltaTime);
		 UpdateArenaBoundaryVisuals();
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
             const float shieldHealth = playerSpaceShip->GetShield().IsActive() ? playerSpaceShip->GetShield().GetHealth() : 0.f;
             const float lethalDamage = playerSpaceShip->GetHealthComponent().GetHealth() + shieldHealth + 1.f;

             playerSpaceShip->SetInvulnerability(false);
             playerSpaceShip->ApplyDamage(lethalDamage);
             return;
         }

         actor->ApplyDamage(9999999.f);
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
