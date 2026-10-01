#include "framework/World.h"
#include "gameplay/ability/nanoPlague/NanoPlagueContracts.h"
#include "gameplay/ability/nanoPlague/NanoPlagueControllerActor.h"
#include "gameplay/ability/nanoPlague/NanoPlagueControllerRegistryActor.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"

#include <algorithm>
#include <iostream>

namespace
{
	using namespace ly;

	bool Expect(bool condition, const char* message)
	{
		if (condition) return true;
		std::cerr << "FAILED: " << message << '\n';
		return false;
	}

	class TestCombatant final : public Actor, public Combatant
	{
	public:
		explicit TestCombatant(World* world)
			: Actor(world), mCombatRuntime(*this)
		{
		}

		CombatRuntime& GetCombatRuntime() override { return mCombatRuntime; }
		const CombatRuntime& GetCombatRuntime() const override { return mCombatRuntime; }
		LightYearsAbilitySystemComponent& GetAbilitySystemComponent() override
		{
			return mCombatRuntime.GetAbilitySystemComponent();
		}
		const LightYearsAbilitySystemComponent& GetAbilitySystemComponent() const override
		{
			return mCombatRuntime.GetAbilitySystemComponent();
		}
		float GetPhysicsCollisionRadius() const override { return 20.f; }

		void ReceiveDamage(DamageContext context) override
		{
			++receivedDamageCount;
			mCombatRuntime.ProcessIncomingDamage(context);
			mCombatRuntime.ApplyHullDamageMitigation(context);
			const float appliedDamage = std::max(0.f, context.remainingDamage);
			mHealth = std::max(0.f, mHealth - appliedDamage);
			context.appliedDamage = appliedDamage;
			context.targetWasKilled = mHealth <= 0.f;
			mCombatRuntime.NotifyDamageResolved(context);
		}

		int receivedDamageCount = 0;

	private:
		CombatRuntime mCombatRuntime;
		float mHealth = 100.f;
	};

	bool HasNanoPlague(const TestCombatant& actor)
	{
		return actor.GetAbilitySystemComponent().HasOwnedTag(AbilityData::NanoPlague::State::Infected);
	}

	bool TestWorldOwnedRegistryReuseAndIsolation()
	{
		World firstWorld{ nullptr };
		World secondWorld{ nullptr };
		NanoPlaguePresentationProfile profile{};
		const shared_ptr<TestCombatant> firstOwner = firstWorld.SpawnActor<TestCombatant>().lock();
		const shared_ptr<TestCombatant> secondOwner = secondWorld.SpawnActor<TestCombatant>().lock();
		if (!Expect(firstOwner && secondOwner, "Could not spawn owners in separate worlds")) return false;

		const shared_ptr<NanoPlagueControllerActor> firstController =
			NanoPlagueControllerActor::FindOrCreate(firstWorld, *firstOwner, profile);
		const shared_ptr<NanoPlagueControllerActor> repeatedController =
			NanoPlagueControllerActor::FindOrCreate(firstWorld, *firstOwner, profile);
		const shared_ptr<NanoPlagueControllerActor> secondController =
			NanoPlagueControllerActor::FindOrCreate(secondWorld, *secondOwner, profile);
		const auto firstRegistries =
			firstWorld.GetActorsByTypeIncludingPending<NanoPlagueControllerRegistryActor>();
		const auto secondRegistries =
			secondWorld.GetActorsByTypeIncludingPending<NanoPlagueControllerRegistryActor>();
		return Expect(firstController && secondController, "Could not create Nano Plague controllers") &&
			Expect(repeatedController == firstController,
				"Repeated casts for one owner created separate controllers") &&
			Expect(firstController != secondController,
				"Controllers were shared across different worlds") &&
			Expect(firstRegistries.size() == 1u && secondRegistries.size() == 1u &&
				firstRegistries.front().lock() != secondRegistries.front().lock(),
				"Nano Plague registries were not isolated per world");
	}

	bool TestRejectsUnmanagedAndForeignOwners()
	{
		World world{ nullptr };
		World otherWorld{ nullptr };
		NanoPlaguePresentationProfile profile{};
		TestCombatant unmanagedOwner{ &world };
		const shared_ptr<TestCombatant> foreignOwner = otherWorld.SpawnActor<TestCombatant>().lock();
		if (!Expect(static_cast<bool>(foreignOwner), "Could not spawn the foreign owner")) return false;

		const shared_ptr<NanoPlagueControllerActor> unmanagedController =
			NanoPlagueControllerActor::FindOrCreate(world, unmanagedOwner, profile);
		const shared_ptr<NanoPlagueControllerActor> foreignController =
			NanoPlagueControllerActor::FindOrCreate(world, *foreignOwner, profile);
		return Expect(!unmanagedController, "An unmanaged stack owner received a controller") &&
			Expect(!foreignController, "An owner from another world received a controller") &&
			Expect(world.GetActorsByTypeIncludingPending<NanoPlagueControllerRegistryActor>().empty(),
				"Invalid owners caused a world registry to be created");
	}

	bool TestRegistrationReplacementKeepsNewestGeneration()
	{
		World world{ nullptr };
		NanoPlaguePresentationProfile profile{};
		const shared_ptr<TestCombatant> owner = world.SpawnActor<TestCombatant>().lock();
		if (!Expect(static_cast<bool>(owner), "Could not spawn the registration test owner")) return false;

		const shared_ptr<NanoPlagueControllerActor> first =
			NanoPlagueControllerActor::FindOrCreate(world, *owner, profile);
		const shared_ptr<NanoPlagueControllerRegistryActor> registry =
			NanoPlagueControllerRegistryActor::GetOrCreate(world);
		const shared_ptr<NanoPlagueControllerActor> replacement =
			world.SpawnActor<NanoPlagueControllerActor>(owner.get(), profile).lock();
		if (!Expect(first && registry && replacement,
			"Could not spawn controllers for the replacement test")) return false;

		auto replacementRegistration = registry->RegisterController(*owner, replacement);
		if (!Expect(replacementRegistration.IsValid(), "Replacement controller was not registered") ||
			!Expect(first->GetIsPendingDestroy(), "Registering a replacement did not destroy the old controller") ||
			!Expect(registry->FindActiveController(*owner) == replacement,
				"Old controller cleanup erased the replacement generation"))
		{
			return false;
		}

		replacementRegistration.Reset();
		const bool removed = !registry->FindActiveController(*owner);
		replacement->Destroy();
		return Expect(removed, "Resetting the current registration left a stale controller entry");
	}

	bool TestInfectionRefreshAndDeathSpread()
	{
		World world{ nullptr };
		NanoPlaguePresentationProfile profile{};
		const shared_ptr<TestCombatant> owner = world.SpawnActor<TestCombatant>().lock();
		const shared_ptr<TestCombatant> infectedTarget = world.SpawnActor<TestCombatant>().lock();
		const shared_ptr<TestCombatant> spreadTarget = world.SpawnActor<TestCombatant>().lock();
		if (!Expect(owner && infectedTarget && spreadTarget, "Could not spawn infection test combatants")) return false;
		owner->SetCollisionLayer(CollisionLayer::Player);
		owner->SetCollisionMask(CollisionLayer::Enemy);
		owner->SetActorLocation({ 0.f, 0.f });
		infectedTarget->SetCollisionLayer(CollisionLayer::Enemy);
		infectedTarget->SetCollisionMask(CollisionLayer::Player);
		infectedTarget->SetActorLocation({ 100.f, 0.f });
		spreadTarget->SetCollisionLayer(CollisionLayer::Enemy);
		spreadTarget->SetCollisionMask(CollisionLayer::Player);
		spreadTarget->SetActorLocation({ 120.f, 0.f });

		const shared_ptr<NanoPlagueControllerActor> controller =
			NanoPlagueControllerActor::FindOrCreate(world, *owner, profile);
		if (!Expect(static_cast<bool>(controller), "Could not create the infection controller")) return false;

		NanoPlagueControllerActor::Settings settings;
		settings.baseTickDamage = 7.f;
		settings.duration = 1.f;
		settings.spreadRadius = 250.f;
		settings.baseSpreadTargetCount = 1;
		if (!Expect(controller->ApplyOrRefreshInfection(*infectedTarget, 0, settings),
			"Initial Nano Plague infection failed") ||
			!Expect(HasNanoPlague(*infectedTarget), "Initial infection did not add the Infected tag"))
		{
			return false;
		}

		world.TickInternal(0.f);
		controller->Tick(0.125f);
		if (!Expect(controller->ApplyOrRefreshInfection(*infectedTarget, 1, settings),
			"Overlapping application did not refresh the infection")) return false;
		controller->Tick(0.25f);
		if (!Expect(infectedTarget->receivedDamageCount == 1,
			"Overlapping applications produced more than one damage stream")) return false;

		DamageContext killContext;
		killContext.target = infectedTarget.get();
		killContext.appliedDamage = 1.f;
		killContext.targetWasKilled = true;
		infectedTarget->GetCombatRuntime().NotifyDamageResolved(killContext);
		controller->Tick(0.016f);
		return Expect(!HasNanoPlague(*infectedTarget), "Killed infection retained its Infected tag") &&
			Expect(HasNanoPlague(*spreadTarget), "A direct infection did not spread once on a kill");
	}

	bool TestOwnerDestructionStopsDamageAndClearsInfection()
	{
		World world{ nullptr };
		NanoPlaguePresentationProfile profile{};
		const shared_ptr<TestCombatant> owner = world.SpawnActor<TestCombatant>().lock();
		const shared_ptr<TestCombatant> target = world.SpawnActor<TestCombatant>().lock();
		if (!Expect(owner && target, "Could not spawn owner-destruction test combatants")) return false;

		const shared_ptr<NanoPlagueControllerActor> controller =
			NanoPlagueControllerActor::FindOrCreate(world, *owner, profile);
		NanoPlagueControllerActor::Settings settings;
		settings.duration = 0.5f;
		if (!Expect(controller && controller->ApplyOrRefreshInfection(*target, 0, settings),
			"Could not apply the owner-destruction test infection")) return false;

		owner->Destroy();
		const shared_ptr<NanoPlagueControllerActor> afterDestroy =
			NanoPlagueControllerActor::FindOrCreate(world, *owner, profile);
		controller->Tick(0.25f);
		controller->Tick(0.25f);
		return Expect(!afterDestroy, "A pending owner received a new controller") &&
			Expect(target->receivedDamageCount == 0,
				"Nano Plague dealt damage after its owner entered destruction") &&
			Expect(!HasNanoPlague(*target), "Expired infection left an Infected tag behind");
	}

	bool TestWorldTeardownWithRetainedController()
	{
		weak_ptr<NanoPlagueControllerRegistryActor> registryWeak;
		shared_ptr<NanoPlagueControllerActor> retainedController;
		shared_ptr<TestCombatant> retainedTarget;
		{
			World world{ nullptr };
			NanoPlaguePresentationProfile profile{};
			const shared_ptr<TestCombatant> owner = world.SpawnActor<TestCombatant>().lock();
			if (!Expect(static_cast<bool>(owner), "Could not spawn the teardown test owner")) return false;
			retainedTarget = world.SpawnActor<TestCombatant>().lock();
			retainedController = NanoPlagueControllerActor::FindOrCreate(world, *owner, profile);
			const shared_ptr<NanoPlagueControllerRegistryActor> registry =
				NanoPlagueControllerRegistryActor::GetOrCreate(world);
			if (!Expect(owner && retainedTarget && retainedController && registry,
				"Could not spawn world-teardown test actors")) return false;

			NanoPlagueControllerActor::Settings settings;
			if (!Expect(retainedController->ApplyOrRefreshInfection(*retainedTarget, 0, settings),
				"Could not apply the teardown test infection")) return false;
			registryWeak = registry;
		}

		if (!Expect(registryWeak.expired(), "World teardown left the Nano Plague registry alive") ||
			!Expect(HasNanoPlague(*retainedTarget),
				"Retained controller cleaned its aura before its own destruction")) return false;

		retainedController.reset();
		return Expect(!HasNanoPlague(*retainedTarget),
			"Retained-target aura survived controller destruction after World teardown");
	}
}

int main()
{
	if (!TestWorldOwnedRegistryReuseAndIsolation() ||
		!TestRejectsUnmanagedAndForeignOwners() ||
		!TestRegistrationReplacementKeepsNewestGeneration() ||
		!TestInfectionRefreshAndDeathSpread() ||
		!TestOwnerDestructionStopsDamageAndClearsInfection() ||
		!TestWorldTeardownWithRetainedController())
	{
		return 1;
	}

	std::cout << "Nano Plague runtime tests passed.\n";
	return 0;
}
