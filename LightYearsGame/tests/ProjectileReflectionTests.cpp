#include "framework/Actor.h"
#include "framework/TimerManager.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/projectile/ProjectileReflectionParticipant.h"
#include "gameplay/projectile/ProjectileReflectionRegistryActor.h"
#include "gameplay/projectile/ProjectileReflectionService.h"
#include "gameplay/projectile/ProjectileReflectionSurface.h"

#include <iostream>
#include <memory>

namespace
{
	class TestReflectionDefender final : public ly::Actor
	{
	public:
		explicit TestReflectionDefender(ly::World* world)
			: Actor(world)
		{
		}
	};

	class TestAbilityProjectile final : public ly::AbilityWorldActor
	{
	public:
		explicit TestAbilityProjectile(ly::World* world)
			: AbilityWorldActor(world, nullptr)
		{
		}

		bool IsProjectileActor() const override { return true; }
	};

	class TestReceiver final : public ly::ProjectileReflectionReceiver
	{
	public:
		bool TryReflectIncomingProjectile(
			ly::AbilityWorldActor& projectile,
			ly::Actor& defender
		) override
		{
			lastProjectile = &projectile;
			lastDefender = &defender;
			++reflectionCount;
			return true;
		}

		ly::AbilityWorldActor* lastProjectile = nullptr;
		ly::Actor* lastDefender = nullptr;
		int reflectionCount = 0;
	};

	class TestProjectile final : public ly::Actor, public ly::ProjectileReflectionParticipant
	{
	public:
		TestProjectile() : Actor(nullptr) {}

		bool CanBeReflected() const override { return true; }
		bool TryReflectProjectile(const ly::ProjectileReflectionRequest& request) override
		{
			lastOwner = &request.newOwner;
			++reflectionCount;
			return true;
		}

		ly::Actor* lastOwner = nullptr;
		int reflectionCount = 0;
	};

	class TestSurface final : public ly::Actor, public ly::ProjectileReflectionSurface
	{
	public:
		explicit TestSurface(ly::Actor& owner) : Actor(nullptr), mOwner(owner) {}

		bool BuildProjectileReflectionResponse(
			const ly::Actor&,
			ly::ProjectileReflectionSurfaceResponse& outResponse
		) const override
		{
			outResponse = { &mOwner, 1.f, lockDuration, true };
			return true;
		}

		float lockDuration = 0.1f;

	private:
		ly::Actor& mOwner;
	};

	int Fail(const char* message)
	{
		std::cerr << message << '\n';
		return 1;
	}

	bool Reflect(TestProjectile& projectile, TestSurface& surface)
	{
		return ly::ProjectileReflectionService::TryReflectFromSurface(
			projectile,
			surface,
			{ { 10.f, 20.f }, { -1.f, 0.f } }
		);
	}

	int CountRegistryActors(ly::World& world)
	{
		int count = 0;
		for (const ly::weak_ptr<ly::ProjectileReflectionRegistryActor>& registryWeak :
			world.GetActorsByTypeIncludingPending<ly::ProjectileReflectionRegistryActor>())
		{
			const ly::shared_ptr<ly::ProjectileReflectionRegistryActor> registry =
				registryWeak.lock();
			if (registry && !registry->GetIsPendingDestroy())
			{
				++count;
			}
		}
		return count;
	}

	bool TestWorldOwnedReceiverRegistry()
	{
		ly::World firstWorld{ nullptr };
		ly::World secondWorld{ nullptr };
		const ly::shared_ptr<TestReflectionDefender> firstDefender =
			firstWorld.SpawnActor<TestReflectionDefender>().lock();
		const ly::shared_ptr<TestReflectionDefender> secondDefender =
			secondWorld.SpawnActor<TestReflectionDefender>().lock();
		const ly::shared_ptr<TestReflectionDefender> siblingDefender =
			firstWorld.SpawnActor<TestReflectionDefender>().lock();
		const ly::shared_ptr<TestAbilityProjectile> firstProjectile =
			firstWorld.SpawnActor<TestAbilityProjectile>().lock();
		const ly::shared_ptr<TestAbilityProjectile> secondProjectile =
			secondWorld.SpawnActor<TestAbilityProjectile>().lock();
		const ly::shared_ptr<TestAbilityProjectile> siblingProjectile =
			firstWorld.SpawnActor<TestAbilityProjectile>().lock();
		if (!firstDefender || !secondDefender || !siblingDefender || !firstProjectile ||
			!secondProjectile || !siblingProjectile)
		{
			return false;
		}
		siblingDefender->SetActorLocation({ 1000.f, 1000.f });

		TestReceiver firstReceiver;
		TestReceiver secondReceiver;
		TestReceiver siblingReceiver;
		auto firstRegistration = ly::ProjectileReflectionService::RegisterReceiver(
			*firstDefender, firstReceiver
		);
		auto secondRegistration = ly::ProjectileReflectionService::RegisterReceiver(
			*secondDefender, secondReceiver
		);
		auto siblingRegistration = ly::ProjectileReflectionService::RegisterReceiver(
			*siblingDefender, siblingReceiver
		);
		if (!firstRegistration.IsValid() || !secondRegistration.IsValid() ||
			!siblingRegistration.IsValid() ||
			CountRegistryActors(firstWorld) != 1 || CountRegistryActors(secondWorld) != 1)
		{
			return false;
		}

		if (!ly::ProjectileReflectionService::TryReflectProjectile(
				*firstProjectile, *firstDefender
			) ||
			!ly::ProjectileReflectionService::TryReflectProjectile(
				*secondProjectile, *secondDefender
			) ||
			!ly::ProjectileReflectionService::TryReflectProjectile(
				*siblingProjectile, *siblingDefender
			) ||
			firstReceiver.reflectionCount != 1 || secondReceiver.reflectionCount != 1 ||
			siblingReceiver.reflectionCount != 1 ||
			firstReceiver.lastProjectile != firstProjectile.get() ||
			secondReceiver.lastDefender != secondDefender.get() ||
			siblingReceiver.lastProjectile != siblingProjectile.get())
		{
			return false;
		}

		if (!ly::ProjectileReflectionService::TryReflectProjectileAlongPath(
				*firstProjectile, { -10.f, 0.f }, { 10.f, 0.f }
			) ||
			!ly::ProjectileReflectionService::TryReflectProjectileAlongPath(
				*secondProjectile, { -10.f, 0.f }, { 10.f, 0.f }
			) ||
			firstReceiver.reflectionCount != 2 || secondReceiver.reflectionCount != 2 ||
			siblingReceiver.reflectionCount != 1)
		{
			return false;
		}

		TestReceiver replacementReceiver;
		auto replacementRegistration = ly::ProjectileReflectionService::RegisterReceiver(
			*firstDefender, replacementReceiver
		);
		firstRegistration.Reset();
		if (!replacementRegistration.IsValid() ||
			!ly::ProjectileReflectionService::TryReflectProjectile(
				*firstProjectile, *firstDefender
			) ||
			replacementReceiver.reflectionCount != 1 || firstReceiver.reflectionCount != 2)
		{
			return false;
		}

		replacementRegistration.Reset();
		return !ly::ProjectileReflectionService::TryReflectProjectile(
			*firstProjectile, *firstDefender
		);
	}
}

int main()
{
	auto& timers = ly::TimerManager::GetGameTimerManager();
	timers.ClearAllTimers();

	auto owner = std::make_shared<ly::Actor>(nullptr);
	auto surface = std::make_shared<TestSurface>(*owner);
	auto projectile = std::make_shared<TestProjectile>();
	projectile->SetVelocity({ 1.f, 0.f });
	if (!Reflect(*projectile, *surface))
	{
		return Fail("Reflection service rejected a valid surface response");
	}
	const uint64_t surfaceId = surface->GetUniqueID();
	if (!projectile->IsSurfaceLocked(surfaceId))
	{
		return Fail("Successful reflection did not lock the contacted surface");
	}
	timers.UpdateTimer(0.04f);
	if (!projectile->IsSurfaceLocked(surfaceId) || Reflect(*projectile, *surface))
	{
		return Fail("Surface lock expired before its configured duration");
	}
	timers.UpdateTimer(0.07f);
	if (projectile->IsSurfaceLocked(surfaceId) || !Reflect(*projectile, *surface))
	{
		return Fail("Surface lock did not expire after its configured duration");
	}
	timers.ClearAllTimers();

	TestProjectile generationProjectile;
	const uint64_t olderGeneration = generationProjectile.LockSurface(surfaceId);
	const uint64_t newerGeneration = generationProjectile.LockSurface(surfaceId);
	generationProjectile.UnlockSurface(surfaceId, olderGeneration);
	if (!generationProjectile.IsSurfaceLocked(surfaceId))
	{
		return Fail("An older lock generation removed the current surface lock");
	}
	generationProjectile.UnlockSurface(surfaceId, newerGeneration);
	if (generationProjectile.IsSurfaceLocked(surfaceId))
	{
		return Fail("The current lock generation could not release its surface lock");
	}
	if (!TestWorldOwnedReceiverRegistry())
	{
		return Fail("World-owned reflection receiver registration failed isolation or cleanup checks");
	}

	auto destroyedProjectile = std::make_shared<TestProjectile>();
	destroyedProjectile->SetVelocity({ 1.f, 0.f });
	surface->lockDuration = 0.05f;
	if (!Reflect(*destroyedProjectile, *surface))
	{
		return Fail("Reflection setup for early-destroy cleanup failed");
	}
	destroyedProjectile->Destroy();
	timers.UpdateTimer(0.1f);
	if (!destroyedProjectile->GetIsPendingDestroy() ||
		!destroyedProjectile->IsSurfaceLocked(surfaceId))
	{
		return Fail("Destroyed actor timer did not skip its callback as expected");
	}
	auto independentProjectile = std::make_shared<TestProjectile>();
	independentProjectile->SetVelocity({ 1.f, 0.f });
	if (!Reflect(*independentProjectile, *surface))
	{
		return Fail("A destroyed projectile left a process-wide surface lock behind");
	}

	timers.ClearAllTimers();
	return 0;
}
