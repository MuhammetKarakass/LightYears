#pragma once

#include "framework/Core.h"
#include "level/ArenaDefinition.h"
#include "framework/Delegate.h"

namespace ly
{
	class Actor;
	
	class ArenaBoundarySystem
	{
	public:
		ArenaBoundarySystem();

		void Initialize(const ArenaDefinition& arenaDefinition);

		void SetTrackedActor(weak_ptr<Actor> trackedActor);

		virtual void Tick(float deltaTime);

		void ResetArenaBoundarySystem();

		bool IsInitialized() const { return mInitialized; };
		bool IsWarningActive() const { return mWarningActive; };
		float GetOutOfBoundsElapsedTime() const {return mOutOfBoundsElapsedTime; };
		float GetRemainingGraceTime() const;

		Delegate<weak_ptr<Actor>> onBoundaryPenaltyTriggered;
		Delegate<float, float> onBoundaryWarningUpdated;
		Delegate<> onBoundaryWarningCleared;

	private:
		bool IsActorInsideAllowedBounds(const Actor& actor) const;

		void HandleActorInsideBounds();

		void HandleActorOutsideBounds(float deltaTime);

		ArenaDefinition mArenaDefinition;
		weak_ptr<Actor> mTrackedActor;

		bool mInitialized = false;
		bool mWarningActive = false;
		bool mDeathTriggered = false;

		float mOutOfBoundsElapsedTime = 0.f;

	};
}

