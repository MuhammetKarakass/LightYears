#include "gameplay/ability/nanoPlague/NanoPlagueControllerRegistryActor.h"

#include "framework/World.h"
#include "gameplay/ability/nanoPlague/NanoPlagueControllerActor.h"

#include <limits>
#include <utility>

namespace ly
{
	NanoPlagueControllerRegistryActor::Registration::Registration(
		weak_ptr<NanoPlagueControllerRegistryActor> registry,
		unsigned int ownerId,
		std::uint64_t generation
	)
		: mRegistry{ std::move(registry) }, mOwnerId{ ownerId }, mGeneration{ generation }
	{
	}

	NanoPlagueControllerRegistryActor::Registration::~Registration()
	{
		Reset();
	}

	NanoPlagueControllerRegistryActor::Registration::Registration(Registration&& other) noexcept
		: mRegistry{ std::move(other.mRegistry) },
		mOwnerId{ other.mOwnerId },
		mGeneration{ other.mGeneration }
	{
		other.mOwnerId = 0u;
		other.mGeneration = 0u;
	}

	NanoPlagueControllerRegistryActor::Registration&
	NanoPlagueControllerRegistryActor::Registration::operator=(Registration&& other) noexcept
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

	void NanoPlagueControllerRegistryActor::Registration::Reset()
	{
		if (const shared_ptr<NanoPlagueControllerRegistryActor> registry = mRegistry.lock())
		{
			registry->UnregisterController(mOwnerId, mGeneration);
		}
		mRegistry.reset();
		mOwnerId = 0u;
		mGeneration = 0u;
	}

	NanoPlagueControllerRegistryActor::NanoPlagueControllerRegistryActor(World* world)
		: Actor(world)
	{
	}

	shared_ptr<NanoPlagueControllerRegistryActor>
	NanoPlagueControllerRegistryActor::GetOrCreate(World& world)
	{
		for (const weak_ptr<NanoPlagueControllerRegistryActor>& candidateWeak :
			world.GetActorsByTypeIncludingPending<NanoPlagueControllerRegistryActor>())
		{
			const shared_ptr<NanoPlagueControllerRegistryActor> candidate = candidateWeak.lock();
			if (candidate && !candidate->GetIsPendingDestroy()) return candidate;
		}
		return world.SpawnActor<NanoPlagueControllerRegistryActor>().lock();
	}

	NanoPlagueControllerRegistryActor::Registration
	NanoPlagueControllerRegistryActor::RegisterController(
		Actor& owner,
		const shared_ptr<NanoPlagueControllerActor>& controller
	)
	{
		const shared_ptr<Object> ownerObject = owner.GetWeakPtr().lock();
		if (GetIsPendingDestroy() || owner.GetIsPendingDestroy() || !ownerObject ||
			ownerObject.get() != &owner || !controller || controller->GetIsPendingDestroy() ||
			owner.GetWorld() != GetWorld() || controller->GetWorld() != GetWorld() ||
			mNextGeneration == std::numeric_limits<std::uint64_t>::max())
		{
			return {};
		}

		const shared_ptr<NanoPlagueControllerRegistryActor> self =
			std::dynamic_pointer_cast<NanoPlagueControllerRegistryActor>(GetWeakPtr().lock());
		if (!self) return {};

		const unsigned int ownerId = owner.GetUniqueID();
		const std::uint64_t generation = ++mNextGeneration;
		shared_ptr<NanoPlagueControllerActor> previous;
		const auto existing = mControllers.find(ownerId);
		if (existing != mControllers.end()) previous = existing->second.controller.lock();

		mControllers.insert_or_assign(ownerId, Entry{ generation, controller });
		if (previous && previous != controller && !previous->GetIsPendingDestroy())
		{
			previous->Destroy();
		}
		return Registration{ self, ownerId, generation };
	}

	shared_ptr<NanoPlagueControllerActor>
	NanoPlagueControllerRegistryActor::FindActiveController(Actor& owner)
	{
		if (GetIsPendingDestroy() || owner.GetIsPendingDestroy() || owner.GetWorld() != GetWorld())
		{
			return {};
		}

		const unsigned int ownerId = owner.GetUniqueID();
		const auto existing = mControllers.find(ownerId);
		if (existing == mControllers.end()) return {};

		const std::uint64_t generation = existing->second.generation;
		const shared_ptr<NanoPlagueControllerActor> controller = existing->second.controller.lock();
		if (!controller || controller->GetIsPendingDestroy() || controller->GetWorld() != GetWorld())
		{
			UnregisterController(ownerId, generation);
			return {};
		}
		return controller;
	}

	void NanoPlagueControllerRegistryActor::UnregisterController(
		unsigned int ownerId,
		std::uint64_t generation
	)
	{
		const auto existing = mControllers.find(ownerId);
		if (existing != mControllers.end() && existing->second.generation == generation)
		{
			mControllers.erase(existing);
		}
	}
}
