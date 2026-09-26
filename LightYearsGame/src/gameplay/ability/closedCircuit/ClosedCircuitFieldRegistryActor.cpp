#include "gameplay/ability/closedCircuit/ClosedCircuitFieldRegistryActor.h"

#include "framework/World.h"
#include "gameplay/ability/closedCircuit/ClosedCircuitFieldActor.h"

#include <limits>
#include <utility>

namespace ly
{
	ClosedCircuitFieldRegistryActor::Registration::Registration(
		weak_ptr<ClosedCircuitFieldRegistryActor> registry,
		unsigned int ownerId,
		std::uint64_t generation
	)
		: mRegistry{ std::move(registry) }, mOwnerId{ ownerId }, mGeneration{ generation }
	{
	}

	ClosedCircuitFieldRegistryActor::Registration::~Registration()
	{
		Reset();
	}

	ClosedCircuitFieldRegistryActor::Registration::Registration(Registration&& other) noexcept
		: mRegistry{ std::move(other.mRegistry) },
		mOwnerId{ other.mOwnerId },
		mGeneration{ other.mGeneration }
	{
		other.mOwnerId = 0u;
		other.mGeneration = 0u;
	}

	ClosedCircuitFieldRegistryActor::Registration&
	ClosedCircuitFieldRegistryActor::Registration::operator=(Registration&& other) noexcept
	{
		if (this == &other) return *this;
		Reset();
		mRegistry = std::move(other.mRegistry);
		mOwnerId = other.mOwnerId;
		mGeneration = other.mGeneration;
		other.mOwnerId = 0u;
		other.mGeneration = 0u;
		return *this;
	}

	void ClosedCircuitFieldRegistryActor::Registration::Reset()
	{
		if (const shared_ptr<ClosedCircuitFieldRegistryActor> registry = mRegistry.lock())
		{
			registry->UnregisterField(mOwnerId, mGeneration);
		}
		mRegistry.reset();
		mOwnerId = 0u;
		mGeneration = 0u;
	}

	ClosedCircuitFieldRegistryActor::ClosedCircuitFieldRegistryActor(World* world)
		: Actor(world)
	{
	}

	shared_ptr<ClosedCircuitFieldRegistryActor>
	ClosedCircuitFieldRegistryActor::GetOrCreate(World& world)
	{
		for (const weak_ptr<ClosedCircuitFieldRegistryActor>& candidateWeak :
			world.GetActorsByTypeIncludingPending<ClosedCircuitFieldRegistryActor>())
		{
			const shared_ptr<ClosedCircuitFieldRegistryActor> candidate = candidateWeak.lock();
			if (candidate && !candidate->GetIsPendingDestroy()) return candidate;
		}
		return world.SpawnActor<ClosedCircuitFieldRegistryActor>().lock();
	}

	ClosedCircuitFieldRegistryActor::Registration
	ClosedCircuitFieldRegistryActor::RegisterField(
		Actor& owner,
		const shared_ptr<ClosedCircuitFieldActor>& field
	)
	{
		if (GetIsPendingDestroy() || owner.GetIsPendingDestroy() || !field ||
			field->GetIsPendingDestroy() || owner.GetWorld() != GetWorld() ||
			field->GetWorld() != GetWorld() ||
			mNextGeneration == std::numeric_limits<std::uint64_t>::max())
		{
			return {};
		}

		const shared_ptr<ClosedCircuitFieldRegistryActor> self =
			std::dynamic_pointer_cast<ClosedCircuitFieldRegistryActor>(GetWeakPtr().lock());
		if (!self) return {};

		const unsigned int ownerId = owner.GetUniqueID();
		const std::uint64_t generation = ++mNextGeneration;
		shared_ptr<ClosedCircuitFieldActor> previous;
		const auto existing = mFields.find(ownerId);
		if (existing != mFields.end()) previous = existing->second.field.lock();

		mFields.insert_or_assign(ownerId, Entry{ generation, field });
		if (previous && previous != field && !previous->GetIsPendingDestroy())
		{
			previous->Destroy();
		}
		return Registration{ self, ownerId, generation };
	}

	shared_ptr<ClosedCircuitFieldActor>
	ClosedCircuitFieldRegistryActor::FindActiveField(Actor& owner)
	{
		if (GetIsPendingDestroy() || owner.GetIsPendingDestroy() ||
			owner.GetWorld() != GetWorld())
		{
			return {};
		}

		const unsigned int ownerId = owner.GetUniqueID();
		const auto existing = mFields.find(ownerId);
		if (existing == mFields.end()) return {};

		const std::uint64_t generation = existing->second.generation;
		const shared_ptr<ClosedCircuitFieldActor> field = existing->second.field.lock();
		if (!field || field->GetIsPendingDestroy() || field->GetWorld() != GetWorld())
		{
			UnregisterField(ownerId, generation);
			return {};
		}
		return field;
	}

	void ClosedCircuitFieldRegistryActor::UnregisterField(
		unsigned int ownerId,
		std::uint64_t generation
	)
	{
		const auto existing = mFields.find(ownerId);
		if (existing != mFields.end() && existing->second.generation == generation)
		{
			mFields.erase(existing);
		}
	}
}
