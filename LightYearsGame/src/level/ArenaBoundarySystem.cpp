#include "level/ArenaBoundarySystem.h"
#include "framework/Actor.h"
#include <algorithm>

namespace ly
{
	ArenaBoundarySystem::ArenaBoundarySystem()
		: mArenaDefinition{},
		mTrackedActor{},
		mInitialized(false),
		mWarningActive(false),
		mDeathTriggered(false),
		mOutOfBoundsElapsedTime(0.f)
	{
	}
	void ArenaBoundarySystem::Initialize(const ArenaDefinition& arenaDefinition)
	{
		mArenaDefinition = arenaDefinition;
		mInitialized = true;
		ResetArenaBoundarySystem();
	}
	void ArenaBoundarySystem::SetTrackedActor(weak_ptr<Actor> trackedActor)
	{
		mTrackedActor = trackedActor;
		ResetArenaBoundarySystem();
	}
	void ArenaBoundarySystem::Tick(float deltaTime)
	{
		if (!mInitialized || mDeathTriggered)
			return;

		auto trackedActor = mTrackedActor.lock();

		if (!trackedActor || trackedActor->GetIsPendingDestroy())
		{
			ResetArenaBoundarySystem();
			return;
		}

		if (IsActorInsideAllowedBounds(*trackedActor))
		{
			HandleActorInsideBounds();
		}
		else
		{
			HandleActorOutsideBounds(deltaTime);
		}
	}
	void ArenaBoundarySystem::ResetArenaBoundarySystem()
	{
		const bool wasWarningActive = mWarningActive;

		mWarningActive = false;
		mDeathTriggered = false;
		mOutOfBoundsElapsedTime = 0.f;

		if (wasWarningActive)
		{
			onBoundaryWarningCleared.Broadcast();
		}
	}
	float ArenaBoundarySystem::GetRemainingGraceTime() const
	{
		const float remainingTime = mArenaDefinition.outOfBoundsTime - mOutOfBoundsElapsedTime;
		return remainingTime > 0.0f ? remainingTime : 0.0f;
	}
	bool ArenaBoundarySystem::IsActorInsideAllowedBounds(const Actor& actor) const
	{
		const sf::Vector2f actorLocation = actor.GetActorLocation();
		const sf::Vector2f boundsPosition = mArenaDefinition.legalBounds.position;
		const sf::Vector2f boundsSize = mArenaDefinition.legalBounds.size;
		const float margin = std::max(0.f, mArenaDefinition.outOfBoundsMargin);

		const float left = boundsPosition.x - margin;
		const float right = boundsPosition.x + boundsSize.x + margin;
		const float top = boundsPosition.y - margin;
		const float bottom = boundsPosition.y + boundsSize.y + margin;

		return actorLocation.x >= left
			&& actorLocation.x <= right
			&& actorLocation.y >= top
			&& actorLocation.y <= bottom;
	}
	void ArenaBoundarySystem::HandleActorInsideBounds()
	{
		const bool wasWarningActive = mWarningActive;
		mWarningActive = false;
		mOutOfBoundsElapsedTime = 0.f;

		if(wasWarningActive)
		{
			onBoundaryWarningCleared.Broadcast();
		}
	}
	void ArenaBoundarySystem::HandleActorOutsideBounds(float deltaTime)
	{
		mWarningActive = true;
		mOutOfBoundsElapsedTime += deltaTime;

		onBoundaryWarningUpdated.Broadcast(GetRemainingGraceTime(),mArenaDefinition.outOfBoundsTime);

		if (mOutOfBoundsElapsedTime >= mArenaDefinition.outOfBoundsTime)
		{
			if (auto trackedActor = mTrackedActor.lock())
			{
				mDeathTriggered = true;
				onBoundaryPenaltyTriggered.Broadcast(trackedActor);
			}
		}
	}
}
