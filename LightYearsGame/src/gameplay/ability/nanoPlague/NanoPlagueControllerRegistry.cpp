#include "gameplay/ability/nanoPlague/NanoPlagueControllerRegistry.h"

#include "framework/World.h"
#include "framework/WorldServices.h"
#include "gameplay/ability/nanoPlague/NanoPlagueControllerActor.h"

#include <limits>
#include <utility>

namespace vega
{
	NanoPlagueControllerRegistry::Registration::Registration(weak_ptr<State> state, unsigned int ownerId, std::uint64_t generation)
		: mState{ std::move(state) }, mOwnerId{ ownerId }, mGeneration{ generation }
	{
	}

	NanoPlagueControllerRegistry::Registration::~Registration()
	{
		Reset();
	}

	NanoPlagueControllerRegistry::Registration::Registration(Registration&& other) noexcept
		: mState{ std::move(other.mState) },
		mOwnerId{ other.mOwnerId },
		mGeneration{ other.mGeneration }
	{
		other.mOwnerId = 0u;
		other.mGeneration = 0u;
	}

	NanoPlagueControllerRegistry::Registration& NanoPlagueControllerRegistry::Registration::operator=(Registration&& other) noexcept
	{
		if (this == &other) return *this;
		Reset();
		mState = std::move(other.mState);
		mOwnerId = other.mOwnerId;
		mGeneration = other.mGeneration;
		other.mOwnerId = 0u;
		other.mGeneration = 0u;
		return *this;
	}

	void NanoPlagueControllerRegistry::Registration::Reset()
	{
		if (const shared_ptr<State> state = mState.lock())
		{
			state->UnregisterController(mOwnerId, mGeneration);
		}
		mState.reset();
		mOwnerId = 0u;
		mGeneration = 0u;
	}

	NanoPlagueControllerRegistry::NanoPlagueControllerRegistry(World* world)
		: mWorld(world)
	{
	}

	NanoPlagueControllerRegistry* NanoPlagueControllerRegistry::GetOrCreate(World& world)
	{
		try { return &world.Services().GetOrCreate<NanoPlagueControllerRegistry>(&world); }
		catch (const std::logic_error&)
		{
			// Services close before actor teardown; destructor-time lookup must be harmless.
			return nullptr;
		}
	}

	NanoPlagueControllerRegistry::Registration NanoPlagueControllerRegistry::RegisterController(Actor& owner, const shared_ptr<NanoPlagueControllerActor>& controller)
	{
		const shared_ptr<Object> ownerObject = owner.GetWeakPtr().lock();
		if (owner.GetIsPendingDestroy() || !ownerObject ||
			ownerObject.get() != &owner || !controller || controller->GetIsPendingDestroy() ||
			owner.GetWorld() != mWorld || controller->GetWorld() != mWorld ||
			mNextGeneration == std::numeric_limits<std::uint64_t>::max())
		{
			return {};
		}

		const unsigned int ownerId = owner.GetUniqueID();
		const std::uint64_t generation = ++mNextGeneration;
		shared_ptr<NanoPlagueControllerActor> previous;
		const auto existing = mState->entries.find(ownerId);
		if (existing != mState->entries.end()) previous = existing->second.controller.lock();

		mState->entries.insert_or_assign(ownerId, Entry{ generation, controller });
		if (previous && previous != controller && !previous->GetIsPendingDestroy())
		{
			previous->Destroy();
		}
		return Registration{ mState, ownerId, generation };
	}

	shared_ptr<NanoPlagueControllerActor> NanoPlagueControllerRegistry::FindActiveController(Actor& owner)
	{
		if (owner.GetIsPendingDestroy() || owner.GetWorld() != mWorld)
		{
			return {};
		}

		const unsigned int ownerId = owner.GetUniqueID();
		const auto existing = mState->entries.find(ownerId);
		if (existing == mState->entries.end()) return {};

		const std::uint64_t generation = existing->second.generation;
		const shared_ptr<NanoPlagueControllerActor> controller = existing->second.controller.lock();
		if (!controller || controller->GetIsPendingDestroy() || controller->GetWorld() != mWorld)
		{
			mState->UnregisterController(ownerId, generation);
			return {};
		}
		return controller;
	}

	void NanoPlagueControllerRegistry::State::UnregisterController(unsigned int ownerId, std::uint64_t generation)
	{
		const auto existing = entries.find(ownerId);
		if (existing != entries.end() && existing->second.generation == generation)
		{
			entries.erase(existing);
		}
	}
}
