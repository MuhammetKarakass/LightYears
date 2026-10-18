#include "gameFramework/LightYearsAssets.h"
#include "framework/Actor.h"
#include "framework/TimerManager.h"
#include "framework/World.h"
#include "framework/WorldServices.h"
#include "gameplay/ability/nanoPlague/NanoPlagueControllerRegistry.h"
#include "gameplay/ability/closedCircuit/ClosedCircuitFieldRegistry.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/projectile/ProjectileReflectionParticipant.h"
#include "gameplay/projectile/ProjectileReflectionRegistry.h"
#include "gameplay/projectile/ProjectileReflectionService.h"
#include "gameplay/projectile/ProjectileReflectionSurface.h"

#include <iostream>
#include <memory>

namespace
{
	class TestReflectionDefender final : public vega::Actor
	{
	public:
		explicit TestReflectionDefender(vega::World* world)
			: Actor(world)
		{
		}
	};

	class TestAbilityProjectile final : public vega::AbilityWorldActor
	{
	public:
		explicit TestAbilityProjectile(vega::World* world)
			: AbilityWorldActor(world, nullptr)
		{
		}

		bool IsProjectileActor() const override { return true; }
	};

	class TestReceiver final : public vega::ProjectileReflectionReceiver
	{
	public:
		bool TryReflectIncomingProjectile(
			vega::AbilityWorldActor& projectile,
			vega::Actor& defender
		) override
		{
			lastProjectile = &projectile;
			lastDefender = &defender;
			++reflectionCount;
			return true;
		}

		vega::AbilityWorldActor* lastProjectile = nullptr;
		vega::Actor* lastDefender = nullptr;
		int reflectionCount = 0;
	};

	class TestProjectile final : public vega::Actor, public vega::ProjectileReflectionParticipant
	{
	public:
		explicit TestProjectile(vega::World* world = nullptr) : Actor(world) {}

		bool CanBeReflected() const override { return true; }
		bool TryReflectProjectile(const vega::ProjectileReflectionRequest& request) override
		{
			lastOwner = &request.newOwner;
			++reflectionCount;
			return true;
		}

		vega::Actor* lastOwner = nullptr;
		int reflectionCount = 0;
	};

	class TestSurface final : public vega::Actor, public vega::ProjectileReflectionSurface
	{
	public:
		explicit TestSurface(vega::Actor& owner) : Actor(nullptr), mOwner(owner) {}

		bool BuildProjectileReflectionResponse(
			const vega::Actor&,
			vega::ProjectileReflectionSurfaceResponse& outResponse
		) const override
		{
			outResponse = { &mOwner, 1.f, lockDuration, true };
			return true;
		}

		float lockDuration = 0.1f;

	private:
		vega::Actor& mOwner;
	};

	int Fail(const char* message)
	{
		std::cerr << message << '\n';
		return 1;
	}

	bool Reflect(TestProjectile& projectile, TestSurface& surface)
	{
		return vega::ProjectileReflectionService::TryReflectFromSurface(
			projectile,
			surface,
			{ { 10.f, 20.f }, { -1.f, 0.f } }
		);
	}

	int CountRegistries(vega::World& world)
	{
		return world.Services().Find<vega::ProjectileReflectionRegistry>() ? 1 : 0;
	}

	bool TestWorldOwnedReceiverRegistry()
	{
		vega::World firstWorld{ nullptr };
		vega::World secondWorld{ nullptr };
		const vega::shared_ptr<TestReflectionDefender> firstDefender =
			firstWorld.SpawnActor<TestReflectionDefender>().lock();
		const vega::shared_ptr<TestReflectionDefender> secondDefender =
			secondWorld.SpawnActor<TestReflectionDefender>().lock();
		const vega::shared_ptr<TestReflectionDefender> siblingDefender =
			firstWorld.SpawnActor<TestReflectionDefender>().lock();
		const vega::shared_ptr<TestAbilityProjectile> firstProjectile =
			firstWorld.SpawnActor<TestAbilityProjectile>().lock();
		const vega::shared_ptr<TestAbilityProjectile> secondProjectile =
			secondWorld.SpawnActor<TestAbilityProjectile>().lock();
		const vega::shared_ptr<TestAbilityProjectile> siblingProjectile =
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
		auto firstRegistration = vega::ProjectileReflectionService::RegisterReceiver(
			*firstDefender, firstReceiver
		);
		auto secondRegistration = vega::ProjectileReflectionService::RegisterReceiver(
			*secondDefender, secondReceiver
		);
		auto siblingRegistration = vega::ProjectileReflectionService::RegisterReceiver(
			*siblingDefender, siblingReceiver
		);
		if (!firstRegistration.IsValid() || !secondRegistration.IsValid() ||
			!siblingRegistration.IsValid() ||
			CountRegistries(firstWorld) != 1 || CountRegistries(secondWorld) != 1)
		{
			return false;
		}

		if (!vega::ProjectileReflectionService::TryReflectProjectile(
				*firstProjectile, *firstDefender
			) ||
			!vega::ProjectileReflectionService::TryReflectProjectile(
				*secondProjectile, *secondDefender
			) ||
			!vega::ProjectileReflectionService::TryReflectProjectile(
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

		if (!vega::ProjectileReflectionService::TryReflectProjectileAlongPath(
				*firstProjectile, { -10.f, 0.f }, { 10.f, 0.f }
			) ||
			!vega::ProjectileReflectionService::TryReflectProjectileAlongPath(
				*secondProjectile, { -10.f, 0.f }, { 10.f, 0.f }
			) ||
			firstReceiver.reflectionCount != 2 || secondReceiver.reflectionCount != 2 ||
			siblingReceiver.reflectionCount != 1)
		{
			return false;
		}

		TestReceiver replacementReceiver;
		auto replacementRegistration = vega::ProjectileReflectionService::RegisterReceiver(
			*firstDefender, replacementReceiver
		);
		firstRegistration.Reset();
		if (!replacementRegistration.IsValid() ||
			!vega::ProjectileReflectionService::TryReflectProjectile(
				*firstProjectile, *firstDefender
			) ||
			replacementReceiver.reflectionCount != 1 || firstReceiver.reflectionCount != 2)
		{
			return false;
		}

		replacementRegistration.Reset();
		return !vega::ProjectileReflectionService::TryReflectProjectile(
			*firstProjectile, *firstDefender
		);
	}
	// This probe is owned only by World, and checks access while World's destructor is running.
	class RegistryTeardownProbe final : public vega::Actor
	{
	public:
		RegistryTeardownProbe(vega::World* world, bool* checked) : Actor(world), mWorld(*world), mChecked(*checked) {}
		~RegistryTeardownProbe() override
		{
			mChecked = !mWorld.Services().Find<vega::ProjectileReflectionRegistry>() &&
				!mWorld.Services().Find<vega::NanoPlagueControllerRegistry>() &&
				!mWorld.Services().Find<vega::ClosedCircuitFieldRegistry>() &&
				!vega::ProjectileReflectionRegistry::GetOrCreate(mWorld) &&
				!vega::NanoPlagueControllerRegistry::GetOrCreate(mWorld) &&
				!vega::ClosedCircuitFieldRegistry::GetOrCreate(mWorld);
		}

	private:
		vega::World& mWorld;
		bool& mChecked;
	};

	bool TestRegistryRemovalAndWorldTeardown()
	{
		vega::ProjectileReflectionService::Registration retainedRegistration;
		vega::shared_ptr<TestReflectionDefender> retainedDefender;
		TestReceiver receiver;
		bool teardownChecked = false;
		{
			vega::World world{ nullptr };
			retainedDefender = world.SpawnActor<TestReflectionDefender>().lock();
			auto projectile = world.SpawnActor<TestAbilityProjectile>().lock();
			if (!retainedDefender || !projectile) return false;
			auto oldRegistration = vega::ProjectileReflectionService::RegisterReceiver(*retainedDefender, receiver);
			if (!oldRegistration.IsValid() || !world.Services().Remove<vega::ProjectileReflectionRegistry>() ||
				world.Services().Find<vega::ProjectileReflectionRegistry>()) return false;
			retainedRegistration = vega::ProjectileReflectionService::RegisterReceiver(*retainedDefender, receiver);
			oldRegistration.Reset();
			if (!retainedRegistration.IsValid() ||
				!vega::ProjectileReflectionService::TryReflectProjectile(*projectile, *retainedDefender)) return false;
			if (!vega::NanoPlagueControllerRegistry::GetOrCreate(world) ||
				!vega::ClosedCircuitFieldRegistry::GetOrCreate(world)) return false;
			world.SpawnActor<RegistryTeardownProbe>(&teardownChecked);
			world.TickInternal(0.f);
		}
		retainedRegistration.Reset(); // No World or service exists at this point.
		if (!teardownChecked || retainedDefender->GetWorld()) return false;
		auto rejected = vega::ProjectileReflectionService::RegisterReceiver(*retainedDefender, receiver);
		if (rejected.IsValid()) return false;
		bool emptyTeardownChecked = false;
		{
			vega::World emptyWorld{ nullptr };
			emptyWorld.SpawnActor<RegistryTeardownProbe>(&emptyTeardownChecked);
		}
		return emptyTeardownChecked;
	}

}

int main()
{
	vega::InstallVegaEngineAssets();
	vega::World reflectionWorld{ nullptr };
	auto& timers = reflectionWorld.GetTimerManager();
	timers.ClearAllTimers();

	auto owner = std::make_shared<vega::Actor>(nullptr);
	auto surface = std::make_shared<TestSurface>(*owner);
	auto projectile = std::make_shared<TestProjectile>(&reflectionWorld);
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
	if (!TestRegistryRemovalAndWorldTeardown())
	{
		return Fail("Reflection registry removal, recreation or World teardown checks failed");
	}

	auto destroyedProjectile = std::make_shared<TestProjectile>(&reflectionWorld);
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
	auto independentProjectile = std::make_shared<TestProjectile>(&reflectionWorld);
	independentProjectile->SetVelocity({ 1.f, 0.f });
	if (!Reflect(*independentProjectile, *surface))
	{
		return Fail("A destroyed projectile left a process-wide surface lock behind");
	}

	timers.ClearAllTimers();
	return 0;
}
