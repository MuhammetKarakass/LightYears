#pragma once

#include "framework/reflection/ReflectionDeclare.h"

namespace ly
{
	class Actor;
	class World;

	// Unreal-style actor component: owned by exactly one Actor, with no transform
	// of its own (the transform stays on Actor; there is no scene-component
	// hierarchy). Lifecycle, driven by Actor:
	//   BeginPlay      - once, from Actor::BeginPlayInternal BEFORE Actor::BeginPlay
	//                    (or immediately when added to an actor that already began).
	//   TickComponent  - only when mCanEverTick and IsActive(); called from the base
	//                    Actor::Tick, so it runs exactly where the derived Tick
	//                    invokes Actor::Tick.
	//   EndPlay        - once, from Actor::Destroy; skipped if never begun.
	class ActorComponent
	{
		VEGA_CLASS(ActorComponent)
	public:
		explicit ActorComponent(Actor& owner);
		virtual ~ActorComponent();
		ActorComponent(const ActorComponent&) = delete;
		ActorComponent& operator=(const ActorComponent&) = delete;

		Actor& GetOwner() const { return mOwner; }
		World* GetWorld() const;

		virtual void BeginPlay();
		virtual void TickComponent(float deltaTime);
		virtual void EndPlay();

		bool CanEverTick() const { return mCanEverTick; }
		bool HasBegunPlay() const { return mBeganPlay; }
		bool IsActive() const { return mActive; }
		void SetActive(bool active) { mActive = active; }

	protected:
		// Set in the derived constructor (UE bCanEverTick).
		bool mCanEverTick = false;

	private:
		void BeginPlayInternal();
		void EndPlayInternal();

		Actor& mOwner;
		bool mBeganPlay = false;
		bool mEndedPlay = false;
		bool mActive = true;

		friend class Actor;
	};
}
