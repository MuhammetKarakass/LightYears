#include "framework/Application.h"
#include "framework/Actor.h"
#include "framework/AssetManager.h"
#include "framework/TimerManager.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/chainLightning/ChainLightningAbility.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/weapon/visuals/ElectricArcVisualActor.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

namespace
{
	class ChainLightningTestCombatant final : public ly::Actor, public ly::Combatant
	{
	public:
		explicit ChainLightningTestCombatant(ly::World* world)
			: ly::Actor(world), mCombatRuntime(*this)
		{
		}

		ly::CombatRuntime& GetCombatRuntime() override { return mCombatRuntime; }
		const ly::CombatRuntime& GetCombatRuntime() const override { return mCombatRuntime; }
		ly::LightYearsAbilitySystemComponent& GetAbilitySystemComponent() override
		{
			return mCombatRuntime.GetAbilitySystemComponent();
		}
		const ly::LightYearsAbilitySystemComponent& GetAbilitySystemComponent() const override
		{
			return mCombatRuntime.GetAbilitySystemComponent();
		}
		float GetPhysicsCollisionRadius() const override { return 20.f; }
		sf::Vector2f GetPhysicsCollisionBoxHalfExtents() const override { return { 20.f, 20.f }; }

		void ReceiveDamage(ly::DamageContext context) override
		{
			++receivedDamageCount;
			lastReceivedContext = context;
			mCombatRuntime.ProcessIncomingDamage(lastReceivedContext);
			mCombatRuntime.ApplyHullDamageMitigation(lastReceivedContext);
			lastProcessedContext = lastReceivedContext;
			health = std::max(0.f, health - lastProcessedContext.remainingDamage);
		}

		ly::CombatRuntime mCombatRuntime;
		ly::DamageContext lastReceivedContext{};
		ly::DamageContext lastProcessedContext{};
		float health{ 100.f };
		int receivedDamageCount{ 0 };
	};

	struct WorldFixture
	{
		std::shared_ptr<ly::World> world;
		std::shared_ptr<ChainLightningTestCombatant> owner;
		std::shared_ptr<ChainLightningTestCombatant> target;

		void Create(ly::Application& application, int ownerCount = 1)
		{
			world = std::make_shared<ly::World>(&application);
			for (int index = 0; index < ownerCount; ++index)
			{
				auto ownerActor = world->SpawnActor<ChainLightningTestCombatant>().lock();
				ownerActor->SetCollisionLayer(CollisionLayer::Player);
				ownerActor->SetCollisionMask(CollisionLayer::Enemy);
				ownerActor->SetActorLocation(sf::Vector2f{ 100.f + 32.f * index, 128.f });
				ownerActor->mCombatRuntime.InitializeOwnerAttributes(100.f);
				if (index == 0) owner = std::move(ownerActor);
			}

			target = world->SpawnActor<ChainLightningTestCombatant>().lock();
			target->SetCollisionLayer(CollisionLayer::Enemy);
			target->SetCollisionMask(CollisionLayer::Player);
			target->SetActorLocation(sf::Vector2f{ 160.f, 128.f });
			target->mCombatRuntime.InitializeOwnerAttributes(100.f);
			world->TickInternal(0.f);
		}
};

	bool ActivateChainLightning(
		ChainLightningTestCombatant& owner,
		unsigned int handleId,
		ly::ChainLightningAbility& behavior
	)
	{
		const ly::GameAbilityDefinition* definition = AbilityData::FindShippedAbilityDefinition(
			AbilityData::ChainLightning::AbilityId::Basic
		);
		if (!definition) return false;

		ly::GameAbility instance{
			owner.GetAbilitySystemComponent(),
			sas::AbilityHandle{ handleId },
			*definition,
			std::make_unique<ly::ChainLightningAbility>(),
			false
		};
		ly::GameAbilityBehaviorContext context{
			owner.GetAbilitySystemComponent(), instance, owner, *definition
		};
		return behavior.Activate(context);
	}

	std::size_t CountArcs(const ly::World& world)
	{
		return world.GetActorsByType<ly::ElectricArcVisualActor>().size();
	}

	int Fail(const char* message)
	{
		std::cerr << "[Chain Lightning lifetime test] " << message << '\n';
		return 1;
	}

	void ClearGameTimers()
	{
		ly::TimerManager::GetGameTimerManager().ClearAllTimers();
	}
}

int RunChainLightningLifetimeTests()
{
	using namespace ly;
	AssetManager::GetAssetManager().SetAssetRootDirectory("LightYearsGame/assets/");
	if (!GameContentBootstrap::Register()) return Fail("Game content bootstrap failed");
	ClearGameTimers();

	std::string windowTitle = "Chain Lightning lifetime test";
	Application application{ sf::Vector2u{ 256u, 256u }, 32u, windowTitle, sf::Style::None };
	application.GetRenderWindow().setVisible(false);

	{
		WorldFixture fixture;
		fixture.Create(application);
		ChainLightningAbility behavior;
		if (!ActivateChainLightning(*fixture.owner, 1, behavior))
			return Fail("A valid single-target Chain Lightning cast was rejected");
		TimerManager::GetGameTimerManager().UpdateTimer(0.23f);
		if (fixture.target->receivedDamageCount != 1)
			return Fail("The normal delayed chain link did not hit exactly once");
		fixture.world->TickInternal(0.f);
		if (CountArcs(*fixture.world) != 1)
			return Fail("The normal delayed link did not spawn exactly one arc");
		TimerManager::GetGameTimerManager().UpdateTimer(1.f);
		if (fixture.target->receivedDamageCount != 1)
			return Fail("A completed one-link cast applied another hit");
	}
	ClearGameTimers();

	{
		WorldFixture fixture;
		fixture.Create(application);
		auto secondOwner = fixture.world->SpawnActor<ChainLightningTestCombatant>().lock();
		secondOwner->SetCollisionLayer(CollisionLayer::Player);
		secondOwner->SetCollisionMask(CollisionLayer::Enemy);
		secondOwner->SetActorLocation(sf::Vector2f{ 196.f, 128.f });
		secondOwner->mCombatRuntime.InitializeOwnerAttributes(100.f);
		fixture.world->TickInternal(0.f);
		ChainLightningAbility firstBehavior;
		ChainLightningAbility secondBehavior;
		if (!ActivateChainLightning(*fixture.owner, 2, firstBehavior) ||
			!ActivateChainLightning(*secondOwner, 3, secondBehavior))
		{
			return Fail("Repeated independent casts were rejected");
		}
		fixture.owner->Destroy();
		TimerManager::GetGameTimerManager().UpdateTimer(0.23f);
		if (fixture.target->receivedDamageCount != 1)
			return Fail("Destroying one owner cancelled another cast or allowed the destroyed owner's link");
		fixture.world->TickInternal(0.f);
		if (CountArcs(*fixture.world) != 1)
			return Fail("Owner teardown spawned an arc for the cancelled cast");
		TimerManager::GetGameTimerManager().UpdateTimer(1.f);
		if (fixture.target->receivedDamageCount != 1)
			return Fail("A cancelled or completed cast produced a later duplicate link");
	}
	ClearGameTimers();

	{
		WorldFixture fixture;
		fixture.Create(application);
		const sf::Vector2f initialTargetPosition = fixture.world->GetMouseWorldPosition();
		fixture.owner->SetActorLocation(initialTargetPosition - sf::Vector2f{ 60.f, 0.f });
		fixture.target->SetActorLocation(initialTargetPosition);
		auto secondTarget = fixture.world->SpawnActor<ChainLightningTestCombatant>().lock();
		secondTarget->SetCollisionLayer(CollisionLayer::Enemy);
		secondTarget->SetCollisionMask(CollisionLayer::Player);
		secondTarget->SetActorLocation(initialTargetPosition + sf::Vector2f{ 60.f, 0.f });
		secondTarget->mCombatRuntime.InitializeOwnerAttributes(100.f);
		fixture.world->TickInternal(0.f);
		ChainLightningAbility behavior;
		if (!ActivateChainLightning(*fixture.owner, 4, behavior))
			return Fail("Target teardown fixture cast was rejected");
		fixture.target->Destroy();
		TimerManager::GetGameTimerManager().UpdateTimer(0.23f);
		TimerManager::GetGameTimerManager().UpdateTimer(1.f);
		if (fixture.target->receivedDamageCount != 0 || secondTarget->receivedDamageCount != 0)
			return Fail("A destroyed link target or its later bounce received damage");
		fixture.world->TickInternal(0.f);
		if (CountArcs(*fixture.world) != 0)
			return Fail("A target destroyed during link travel still spawned an arc");
	}
	ClearGameTimers();

	{
		WorldFixture fixture;
		fixture.Create(application);
		auto behavior = std::make_unique<ChainLightningAbility>();
		if (!ActivateChainLightning(*fixture.owner, 5, *behavior))
			return Fail("Ability teardown fixture cast was rejected");
		behavior.reset();
		TimerManager::GetGameTimerManager().UpdateTimer(0.23f);
		if (fixture.target->receivedDamageCount != 0)
			return Fail("Destroying the ability behavior during link travel still applied damage");
		fixture.world->TickInternal(0.f);
		if (CountArcs(*fixture.world) != 0)
			return Fail("Destroying the ability behavior during link travel still spawned an arc");
	}
	ClearGameTimers();

	{
		WorldFixture fixture;
		fixture.Create(application);
		ChainLightningAbility behavior;
		if (!ActivateChainLightning(*fixture.owner, 6, behavior))
			return Fail("World teardown fixture cast was rejected");
		fixture.world.reset();
		TimerManager::GetGameTimerManager().UpdateTimer(0.23f);
		if (fixture.target->receivedDamageCount != 0)
			return Fail("An externally retained target was damaged after its World was destroyed");
	}
	ClearGameTimers();
	return 0;
}
