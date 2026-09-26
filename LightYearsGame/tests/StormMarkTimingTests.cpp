#include "attributes/AttributeSystem.h"
#include "framework/TimerManager.h"
#include "framework/World.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/stormMark/StormMarkAbility.h"
#include "gameplay/ability/stormMark/StormMarkContracts.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/DamageStatusBalanceCatalog.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/stormMark/StormMarkPresentationIds.h"
#include "presentation/ability/stormMark/StormMarkPresentationProfile.h"

#include <iostream>
#include <memory>

namespace
{
	class TestCombatant final : public ly::Actor, public ly::Combatant
	{
	public:
		explicit TestCombatant(ly::World* world = nullptr)
			: Actor{ world }, mHealth{ 100.f, 100.f }, mCombatRuntime{ *this }
		{
			mCombatRuntime.InitializeOwnerAttributes(100.f);
		}

		ly::CombatRuntime& GetCombatRuntime() override { return mCombatRuntime; }
		const ly::CombatRuntime& GetCombatRuntime() const override { return mCombatRuntime; }
		float GetHealth() const { return mHealth.GetHealth(); }
		void ReceiveDamage(ly::DamageContext context) override
		{
			mCombatRuntime.ProcessIncomingDamage(context);
			mCombatRuntime.ApplyHullDamageMitigation(context);
			mHealth.ChangeHealth(-context.remainingDamage);
			mCombatRuntime.NotifyDamageResolved(context);
		}

	private:
		ly::HealthComponent mHealth;
		ly::CombatRuntime mCombatRuntime;
	};

	int Fail(const char* message)
	{
		std::cerr << message << '\n';
		return 1;
	}
}

int main()
{
	using namespace ly;
	TimerManager& timers = TimerManager::GetGameTimerManager();
	timers.ClearAllTimers();
	std::string balanceLoadFailure;
	if (!content::DamageStatusBalanceCatalog::LoadFromFile(
		"LightYearsGame/assets/content/data/damage_status_balance.json",
		&balanceLoadFailure
	))
	{
		std::cerr << balanceLoadFailure << '\n';
		return 1;
	}
	PresentationProfileRegistry<StormMarkPresentationProfile>::Register(
		StormMarkPresentationProfile{ sas::ContentId{ StormMarkPresentationIds::LightningBasic } }
	);

	World world{ nullptr };
	const shared_ptr<TestCombatant> owner = world.SpawnActor<TestCombatant>().lock();
	const shared_ptr<TestCombatant> first = world.SpawnActor<TestCombatant>().lock();
	const shared_ptr<TestCombatant> second = world.SpawnActor<TestCombatant>().lock();
	const shared_ptr<TestCombatant> third = world.SpawnActor<TestCombatant>().lock();
	if (!owner || !first || !second || !third)
	{
		return Fail("Storm Mark timing fixture failed to spawn combatants");
	}
	owner->SetCollisionLayer(CollisionLayer::Player);
	owner->SetCollisionMask(CollisionLayer::Enemy);
	owner->SetActorLocation({ 0.f, 0.f });
	first->SetCollisionLayer(CollisionLayer::Enemy);
	first->SetCollisionMask(CollisionLayer::Player);
	first->SetActorLocation({ 20.f, 0.f });
	second->SetCollisionLayer(CollisionLayer::Enemy);
	second->SetCollisionMask(CollisionLayer::Player);
	second->SetActorLocation({ 40.f, 0.f });
	third->SetCollisionLayer(CollisionLayer::Enemy);
	third->SetCollisionMask(CollisionLayer::Player);
	third->SetActorLocation({ 60.f, 0.f });
	world.TickInternal(0.f);

	GameAbilityDefinition definition;
	definition.abilityId = AbilityData::StormMark::AbilityId::Basic;
	definition.abilityTags = {
		AbilityData::StormMark::CategoryTag,
		AbilityData::StormMark::FamilyTag
	};
	definition.damageTags = { DamageTypeSchema::Electric };
	definition.attributes = {
		{ CommonAttributeIds::Damage, 25.f },
		{ CommonAttributeIds::Range, 600.f },
		{ AbilityData::StormMark::Attribute::BaseTargetCount, 3.f },
		{ AbilityData::StormMark::Attribute::FocusDuration, 0.30f },
		{ AbilityData::StormMark::Attribute::StrikeDuration, 0.20f },
		{ AbilityData::StormMark::Attribute::LuckPerExtraTarget, 50.f },
		{ AbilityData::StormMark::Attribute::ElectricStacks, 1.f }
	};
	GameAbility instance{
		owner->GetCombatRuntime().GetAbilitySystemComponent(),
		{ 1 },
		definition,
		std::make_unique<StormMarkAbility>(),
		false
	};
	StormMarkAbility behavior;
	GameAbilityBehaviorContext context{
		owner->GetCombatRuntime().GetAbilitySystemComponent(),
		instance,
		*owner,
		definition
	};
	if (!behavior.Activate(context))
	{
		return Fail("Storm Mark failed to activate against valid targets");
	}
	world.TickInternal(0.f);
	if (world.GetActorsByType<AreaTelegraphActor>().size() != 1)
	{
		return Fail("Storm Mark did not create its focus telegraph");
	}

	const auto advance = [&](float deltaTime)
	{
		timers.UpdateTimer(deltaTime);
		world.TickInternal(deltaTime);
	};
	advance(0.29f);
	if (first->GetHealth() != 100.f || second->GetHealth() != 100.f ||
		third->GetHealth() != 100.f || world.GetActorsByType<AreaTelegraphActor>().size() != 1)
	{
		return Fail("Storm Mark struck before focus completed or removed its telegraph early");
	}
	advance(0.01f);
	advance(0.001f);
	advance(0.f);
	if (first->GetHealth() >= 100.f || second->GetHealth() != 100.f ||
		third->GetHealth() != 100.f || world.GetActorsByType<AreaTelegraphActor>().size() != 1)
	{
		return Fail("Storm Mark first impact did not begin at focus completion");
	}
	advance(0.099f);
	if (second->GetHealth() != 100.f)
	{
		return Fail("Storm Mark second impact arrived before its stagger boundary");
	}
	advance(0.002f);
	if (second->GetHealth() >= 100.f || third->GetHealth() != 100.f)
	{
		return Fail("Storm Mark impacts did not preserve their stagger order");
	}
	advance(0.097f);
	if (third->GetHealth() != 100.f || world.GetActorsByType<AreaTelegraphActor>().size() != 1)
	{
		return Fail("Storm Mark final impact or telegraph outlived the authored resolve window");
	}
	advance(0.002f);
	if (third->GetHealth() >= 100.f || !world.GetActorsByType<AreaTelegraphActor>().empty())
	{
		return Fail("Storm Mark final strike did not coincide with telegraph cleanup");
	}

	timers.ClearAllTimers();
	return 0;
}
