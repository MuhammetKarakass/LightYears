#include "framework/ActorComponent.h"
#include "framework/Actor.h"

namespace ly
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
}
