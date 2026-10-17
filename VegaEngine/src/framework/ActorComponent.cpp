#include "framework/ActorComponent.h"
#include "framework/Actor.h"
#include "framework/reflection/Reflection.h"

namespace vega
{
	ActorComponent::ActorComponent(Actor& owner) : mOwner{ owner }
	{
	}

	ActorComponent::~ActorComponent() = default;

	World* ActorComponent::GetWorld() const
	{
		return mOwner.GetWorld();
	}

	void ActorComponent::BeginPlay()
	{
	}

	void ActorComponent::TickComponent(float deltaTime)
	{
		(void)deltaTime;
	}

	void ActorComponent::EndPlay()
	{
	}

	void ActorComponent::BeginPlayInternal()
	{
		if (!mBeganPlay)
		{
			mBeganPlay = true;
			BeginPlay();
		}
	}

	void ActorComponent::EndPlayInternal()
	{
		if (mBeganPlay && !mEndedPlay)
		{
			mEndedPlay = true;
			EndPlay();
		}
	}

	VEGA_REFLECT_ROOT(ActorComponent)
	{
		VEGA_CATEGORY("Lifecycle");
		VEGA_PROPERTY("Active", IsActive, SetActive);
		VEGA_READONLY("Can Ever Tick", CanEverTick);
		VEGA_READONLY("Has Begun Play", HasBegunPlay);
	}
}
