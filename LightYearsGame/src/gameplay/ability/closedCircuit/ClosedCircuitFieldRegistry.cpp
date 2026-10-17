#include "gameplay/ability/closedCircuit/ClosedCircuitFieldRegistry.h"

#include "framework/World.h"
#include "framework/WorldServices.h"
#include "gameplay/ability/closedCircuit/ClosedCircuitFieldActor.h"

#include <limits>
#include <utility>

namespace vega
{
	ClosedCircuitFieldRegistry::Registration::Registration(weak_ptr<State> state, unsigned int ownerId, std::uint64_t generation)
		: mState{ std::move(state) }, mOwnerId{ ownerId }, mGeneration{ generation }
	{
	}

	ClosedCircuitFieldRegistry::Registration::~Registration()
	{
		Reset();
	}

	ClosedCircuitFieldRegistry::Registration::Registration(Registration&& other) noexcept
		: mState{ std::move(other.mState) },
		mOwnerId{ other.mOwnerId },
		mGeneration{ other.mGeneration }
	{
		other.mOwnerId = 0u;
		other.mGeneration = 0u;
	}

	ClosedCircuitFieldRegistry::Registration& ClosedCircuitFieldRegistry::Registration::operator=(Registration&& other) noexcept
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

	void ClosedCircuitFieldRegistry::Registration::Reset()
	{
		if (const shared_ptr<State> state = mState.lock())
		{
			state->UnregisterField(mOwnerId, mGeneration);
		}
		mState.reset();
		mOwnerId = 0u;
		mGeneration = 0u;
	}

	ClosedCircuitFieldRegistry::ClosedCircuitFieldRegistry(World* world)
		: mWorld(world)
	{
	}

	ClosedCircuitFieldRegistry* ClosedCircuitFieldRegistry::GetOrCreate(World& world)
	{
		try { return &world.Services().GetOrCreate<ClosedCircuitFieldRegistry>(&world); }
		catch (const std::logic_error&)
		{
			// Services close before actor teardown; destructor-time lookup must be harmless.
			return nullptr;
		}
	}

	ClosedCircuitFieldRegistry::Registration ClosedCircuitFieldRegistry::RegisterField(Actor& owner, const shared_ptr<ClosedCircuitFieldActor>& field)
	{
		if (owner.GetIsPendingDestroy() || !field ||
			field->GetIsPendingDestroy() || owner.GetWorld() != mWorld ||
			field->GetWorld() != mWorld ||
			mNextGeneration == std::numeric_limits<std::uint64_t>::max())
		{
			return {};
		}

		const unsigned int ownerId = owner.GetUniqueID();
		const std::uint64_t generation = ++mNextGeneration;
		shared_ptr<ClosedCircuitFieldActor> previous;
		const auto existing = mState->entries.find(ownerId);
		if (existing != mState->entries.end()) previous = existing->second.field.lock();

		mState->entries.insert_or_assign(ownerId, Entry{ generation, field });
		if (previous && previous != field && !previous->GetIsPendingDestroy())
		{
			previous->Destroy();
		}
		return Registration{ mState, ownerId, generation };
	}

	shared_ptr<ClosedCircuitFieldActor> ClosedCircuitFieldRegistry::FindActiveField(Actor& owner)
	{
		if (owner.GetIsPendingDestroy() ||
			owner.GetWorld() != mWorld)
		{
			return {};
		}

		const unsigned int ownerId = owner.GetUniqueID();
		const auto existing = mState->entries.find(ownerId);
		if (existing == mState->entries.end()) return {};

		const std::uint64_t generation = existing->second.generation;
		const shared_ptr<ClosedCircuitFieldActor> field = existing->second.field.lock();
		if (!field || field->GetIsPendingDestroy() || field->GetWorld() != mWorld)
		{
			mState->UnregisterField(ownerId, generation);
			return {};
		}
		return field;
	}

	void ClosedCircuitFieldRegistry::State::UnregisterField(unsigned int ownerId, std::uint64_t generation)
	{
		const auto existing = entries.find(ownerId);
		if (existing != entries.end() && existing->second.generation == generation)
		{
			entries.erase(existing);
		}
	}
}
