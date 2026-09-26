#include "framework/Application.h"
#include "framework/PhysicsSystem.h"
#include "framework/World.h"
#include "gameplay/ability/GameAbility.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/ability/energySpear/EnergySpearContracts.h"
#include "gameplay/ability/loadout/AbilityLoadoutManager.h"
#include "gameplay/ability/nanoPlague/NanoPlagueControllerActor.h"
#include "gameplay/ability/nanoPlague/NanoPlagueContracts.h"
#include "gameplay/ability/returnProtocol/ReturnProtocolContracts.h"
#include "gameplay/ability/returnProtocol/ReturnProtocolVisualActor.h"
#include "gameplay/ability/rocket/RocketProjectileActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/projectile/ProjectileReflectionService.h"
#include "gameplay/projectile/ProjectileReflectionRegistryActor.h"
#include "gameplay/movement/MovementCollisionService.h"
#include "gameplay/tags/GameplayTags.h"
#include "spaceShip/SpaceShip.h"
#include <nlohmann/json.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace ly
{
	struct AuditFixesE2EAccess
	{
		static DelegateHandle Subscription(const NanoPlagueControllerActor& actor, const Actor& target)
		{
			for (const auto& [id, infection] : actor.mInfections)
				if (infection.target.lock().get() == &target) return infection.damageSubscription;
			return {};
		}
		static sf::Vector2f LastOrigin(const NanoPlagueControllerActor& actor) { return actor.mPulses.back().origin; }
		static int Ticks(const NanoPlagueControllerActor& actor, const Actor& target)
		{
			for (const auto& [id, infection] : actor.mInfections)
				if (infection.target.lock().get() == &target) return infection.ticksApplied;
			return -1;
		}
		static std::size_t Count(const NanoPlagueControllerActor& actor) { return actor.mInfections.size(); }
		static const void* FirstRecord(const NanoPlagueControllerActor& actor) { return &actor.mInfections.begin()->second; }
	};

	namespace
	{
		std::string& AuditTitle() { static std::string title = "Audit fixes E2E"; return title; }
		class AuditApplication final : public Application
		{
		public:
			AuditApplication() : Application({ 320, 240 }, 32, AuditTitle(), sf::Style::None)
			{ GetRenderWindow().setVisible(false); }
		};
		struct LoadoutObserver
		{
			LightYearsAbilitySystemComponent& system;
			bool clear = false;
			void Granted(sas::AbilityHandle)
			{
				if (clear) system.Clear();
				else throw std::runtime_error("Injected post-commit observer failure");
			}
		};
		class EndpointWall final : public Actor
		{
		public:
			explicit EndpointWall(World* world) : Actor(world)
			{ SetPhysicsBodyType(PhysicsBodyType::Static); SetCollisionLayer(CollisionLayer::Environment); }
			sf::Vector2f GetPhysicsCollisionBoxHalfExtents() const override { return { 10.f, 20.f }; }
		};
		class RotatingWall final : public Actor
		{
		public:
			explicit RotatingWall(World* world) : Actor(world)
			{ SetEnablePhysics(false); SetPhysicsBodyType(PhysicsBodyType::Static); SetCollisionLayer(CollisionLayer::Environment); }
			sf::Vector2f GetPhysicsCollisionBoxHalfExtents() const override { return { 400.f, 10.f }; }
		};
		struct LifecycleFaultObserver
		{
			LightYearsAbilitySystemComponent& system;
			sas::GameplayEffectDefinition effect;
			bool visited = false, liveDuringClear = false, mutationRejected = false, additionsRejected = false;
			bool throwOnEnd = false;
			bool throwAfterClear = false;
			void Activated(sas::AbilityHandle handle)
			{
				visited = true;
				mutationRejected = !system.RemoveAbility(handle) &&
					!system.RebindAbility(handle, sas::AbilitySlot::Ability2) &&
					!system.GrantAbility(*AbilityData::FindShippedAbilityDefinition(AbilityData::EnergySpear::AbilityId::Basic), sas::AbilitySlot::Ability1).IsValid();
				system.Clear();
				liveDuringClear = system.GetAbility(handle) != nullptr && system.GetAttributes().HasAttribute(OwnerAttributeIds::MaxHealth);
				if (throwAfterClear) throw std::runtime_error("Injected activation failure after Clear");
			}
			void Ended(sas::AbilityHandle, sas::AbilityEndReason)
			{
				system.Clear();
				system.AddOwnedTag(AbilityData::NanoPlague::State::Infected);
				additionsRejected = !system.ApplyGameplayEffect(effect).IsValid() &&
					!system.HasOwnedTag(AbilityData::NanoPlague::State::Infected);
				if (throwOnEnd) throw std::runtime_error("Injected ended failure");
			}
			void EffectRemoved(sas::GameplayEffectHandle) { throw std::runtime_error("Injected effect removal failure"); }
			void GameplayEvent(const sas::AbilityEvent& event)
			{
				if (event.eventTag == GameplayTags::Event::Ability::Activated) { visited = true; system.Clear(); }
			}
		};
		struct AttributeCleanupFault
		{
			sas::AttributeId target;
			std::string message;
			bool armed = false;
			bool triggered = false;
			void Changed(sas::AttributeId id, float, float)
			{
				if (armed && !triggered && id == target)
				{
					triggered = true;
					throw std::runtime_error(message);
				}
			}
		};
		struct AttributeModifierReentry
		{
			sas::AttributeSystem& attributes;
			sas::AttributeModifierHandle handle;
			bool armed = false;
			bool visited = false;
			void Changed(sas::AttributeId, float, float)
			{
				if (!armed || visited) return;
				visited = true;
				attributes.RemoveModifier(handle);
			}
		};
		struct AttributeRegisteredClearFault
		{
			sas::AttributeSystem& attributes;
			bool visited = false;
			void Registered(sas::AttributeId)
			{
				visited = true;
				attributes.Clear();
			}
		};
		struct AttributeRegisteredThrowFault
		{
			std::string message;
			bool visited = false;
			void Registered(sas::AttributeId)
			{
				visited = true;
				throw std::runtime_error(message);
			}
		};
		struct AttributeChangedClearFault
		{
			sas::AttributeSystem& attributes;
			sas::AttributeId target;
			bool clear = false;
			bool visited = false;
			std::string message;
			void Changed(sas::AttributeId id, float, float)
			{
				if (visited || id != target) return;
				visited = true;
				if (clear) attributes.Clear();
				else throw std::runtime_error(message);
			}
		};
		struct ActivationBoundaryObserver
		{
			LightYearsAbilitySystemComponent& system;
			GameplayTag phase;
			bool clear = false, cancel = false, visited = false, reentryRejected = false;
			bool cleanupFault = false, committedStateVisible = false;
			void Event(const sas::AbilityEvent& event)
			{
				if (cleanupFault && event.eventTag == AbilityData::ReturnProtocol::Event::Ended)
					throw std::runtime_error("Injected cleanup failure after activation failure");
				if (event.eventTag != phase || visited) return;
				visited = true;
				if (auto* ability = system.GetAbility(sas::AbilitySlot::Ability1))
				{
					committedStateVisible = ability->IsActive();
					reentryRejected = !ability->TryActivate() && !ability->SetLevel(2);
					if (cancel) ability->Cancel(sas::AbilityEndReason::Cancelled);
				}
				if (clear) system.Clear();
				if (!cancel) throw std::runtime_error("Injected precommit activation fault");
			}
		};
		struct NestedLoadoutObserver
		{
			AbilityLoadoutManager& loadout;
			bool equipped = true;
			void Event(const sas::AbilityEvent& event)
			{
				if (event.eventTag == GameplayTags::Event::Combat::DamageReceived)
					equipped = loadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability2);
			}
		};
		struct NanoReentryObserver
		{
			shared_ptr<NanoPlagueControllerActor> controller;
			shared_ptr<SpaceShip> original;
			std::vector<shared_ptr<SpaceShip>> additions;
			NanoPlagueControllerActor::Settings settings;
			bool visited = false;
			void Damaged(const DamageContext&)
			{
				if (visited) return;
				visited = true;
				for (const auto& target : additions) controller->ApplyOrRefreshInfection(*target, 1, settings);
				controller->ApplyOrRefreshInfection(*original, 1, settings);
				controller->Tick(10.f); // Nested Tick must not advance refreshed state.
			}
		};
	}

	int RunAuditFixesE2E(const char* artifactPath, const std::string& setupError)
	{
		nlohmann::json result{ { "scenario", "audit-fixes.runtime-lifecycle" }, { "passed", false },
			{ "input", { { "maxima", { 100, 120, 150 } }, { "temporaryExcess", 40 },
				{ "nanoSpreadTargets", 4 }, { "callbackFaultInjection", true }, { "nanoReentryTargets", 32 },
				{ "nanoTickDelta", 0.25f }, { "wallRotations", { 0, 90, 0 } } } } };
		auto check = [&](const char* name, bool passed)
		{
			result["assertions"][name] = passed;
			if (!passed) throw std::runtime_error(name);
		};
		try
		{
			if (!setupError.empty()) throw std::runtime_error(setupError);
			AuditApplication application;
			PhysicsSystem::Get().InitializeWorld({ 0.f, 0.f });
			World world{ &application };
			World otherWorld{ &application };
			const ShipDefinition definition{ "SpaceShooterRedux/PNG/Enemies/enemyRed5.png", 100.f, {}, 0.f, 0, 0, {}, {} };
			auto spawn = [&](float x, CollisionLayer layer)
			{
				auto ship = world.SpawnActor<SpaceShip>(definition).lock();
				ship->SetActorLocation({ x, 0.f });
				ship->SetCollisionLayer(layer);
				ship->SetCollisionMask(layer == CollisionLayer::Player ? CollisionLayer::Enemy : CollisionLayer::Player);
				return ship;
			};
			auto owner = spawn(0.f, CollisionLayer::Player);
			auto infected = spawn(100.f, CollisionLayer::Enemy);
			std::vector<shared_ptr<SpaceShip>> targets;
			for (int i = 0; i < 4; ++i) targets.push_back(spawn(130.f + 30.f * i, CollisionLayer::Enemy));
			world.TickInternal(0.f);
			auto& system = owner->GetAbilitySystemComponent();

			// Attribute event -> actual ship resources -> decay ledger.
			system.GetAttributes().SetBaseValue(OwnerAttributeIds::MaxHealth, 100.f);
			auto& health = owner->GetHealthComponent();
			auto& shield = owner->GetShieldComponent();
			owner->GetShipRuntime().GetAttributes().SetBaseValue(ShipAttributeIds::MaxShield, 100.f);
			health.GrantTemporaryOverhealth("audit", 40.f, 0.f, 10.f);
			shield.GrantTemporaryOvershield("audit", 40.f, 0.f, 10.f);
			for (float maximum : { 120.f, 150.f })
			{
				system.GetAttributes().SetBaseValue(OwnerAttributeIds::MaxHealth, maximum);
				owner->GetShipRuntime().GetAttributes().SetBaseValue(ShipAttributeIds::MaxShield, maximum);
				check("absoluteExcessPreserved", std::abs(health.GetHealth() - maximum - 40.f) < 0.001f &&
					std::abs(shield.GetShield() - maximum - 40.f) < 0.001f);
			}
			health.TickTemporaryOverhealths(1.f);
			shield.TickTemporaryOvershields(1.f);
			check("decayLedgerPreserved", health.GetHealth() == 180.f && shield.GetShield() == 180.f);
			result["outcome"]["healthAfterDecay"] = health.GetHealth();

			// Real damage resolution kills the direct infection; controller spreads.
			auto controller = NanoPlagueControllerActor::FindOrCreate(world, *owner, NanoPlaguePresentationProfile{});
			NanoPlagueControllerActor::Settings settings;
			settings.baseSpreadTargetCount = 4;
			settings.maximumSpreadTargetCount = 4;
			settings.baseTickDamage = 0.f;
			settings.energyPowerTickScale = 0.f;
			check("nanoApplied", controller->ApplyOrRefreshInfection(*infected, 0, settings));
			const auto oldSubscription = AuditFixesE2EAccess::Subscription(*controller, *infected);
			DamageContext damage;
			damage.target = infected.get();
			damage.source = owner.get();
			damage.originalDamage = 100000.f;
			damage.remainingDamage = 100000.f;
			infected->ReceiveDamage(damage);
			controller->Tick(0.01f);
			int spreadCount = 0;
			for (const auto& target : targets)
				if (target->GetAbilitySystemComponent().HasOwnedTag(AbilityData::NanoPlague::State::Infected)) ++spreadCount;
			check("nanoSpreadAfterRealKill", spreadCount == 4);
			check("spreadOriginStable", AuditFixesE2EAccess::LastOrigin(*controller) == sf::Vector2f{ 100.f, 0.f });
			check("deadInfectionUnsubscribed", !infected->GetCombatRuntime().onDamageResolved.UnbindAction(oldSubscription));
			const auto expiredSubscription = AuditFixesE2EAccess::Subscription(*controller, *targets.front());
			auto shortSettings = settings;
			shortSettings.duration = 0.01f;
			check("nanoRefresh", controller->ApplyOrRefreshInfection(*targets.front(), 1, shortSettings));
			controller->Tick(0.02f);
			check("expiryUnsubscribes", !targets.front()->GetCombatRuntime().onDamageResolved.UnbindAction(expiredSubscription));
			check("nanoReinfect", controller->ApplyOrRefreshInfection(*targets.front(), 1, settings));
			const auto liveSubscription = AuditFixesE2EAccess::Subscription(*controller, *targets.front());
			controller->Destroy();
			check("destroyUnsubscribes", !targets.front()->GetCombatRuntime().onDamageResolved.UnbindAction(liveSubscription));
			check("destroyClearsTag", !targets.front()->GetAbilitySystemComponent().HasOwnedTag(AbilityData::NanoPlague::State::Infected));

			// Shipped grants through public loadout, with real observer faults.
			AbilityLoadoutManager loadout{ system };
			LoadoutObserver observer{ system };
			auto subscription = system.onAbilityGranted.BindAction(&observer, &LoadoutObserver::Granted);
			bool threw = false;
			try { loadout.EquipAbility(AbilityData::EnergySpear::AbilityId::Basic, sas::AbilitySlot::Ability1); }
			catch (const std::runtime_error&) { threw = true; }
			system.onAbilityGranted.UnbindAction(subscription);
			const auto* bound = loadout.GetLoadout().FindAbility(sas::AbilitySlot::Ability1);
			check("exceptionCommitReconciled", threw && bound && *bound == AbilityData::EnergySpear::AbilityId::Basic &&
				system.GetAbility(sas::AbilitySlot::Ability1));
			check("reservationsReleased", loadout.EquipAbility(AbilityData::EnergySpear::AbilityId::Basic, sas::AbilitySlot::Ability2));
			subscription = system.onAbilityChanged.BindAction(&observer, &LoadoutObserver::Granted);
			threw = false;
			try { loadout.EquipAbility(AbilityData::EnergySpear::AbilityId::Basic, sas::AbilitySlot::Ability3); }
			catch (const std::runtime_error&) { threw = true; }
			system.onAbilityChanged.UnbindAction(subscription);
			check("rebindExceptionReconciled", threw && !loadout.GetLoadout().FindAbility(sas::AbilitySlot::Ability2) &&
				loadout.GetLoadout().FindAbility(sas::AbilitySlot::Ability3) && system.GetAbility(sas::AbilitySlot::Ability3));
			observer.clear = true;
			subscription = system.onAbilityGranted.BindAction(&observer, &LoadoutObserver::Granted);
			const bool granted = loadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1);
			system.onAbilityGranted.UnbindAction(subscription);
			check("callbackClearNoGhostSlot", !granted && !system.GetAbility(sas::AbilitySlot::Ability1) &&
				!loadout.GetLoadout().FindAbility(sas::AbilitySlot::Ability1) &&
				!loadout.GetLoadout().FindAbility(sas::AbilitySlot::Ability3));

			// Real Return Protocol receiver and Rocket reflection ownership.
			check("returnProtocolEquipped", loadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1));
			system.SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			system.Tick(0.01f);
			auto registry = world.FindServiceActor<ProjectileReflectionRegistryActor>();
			check("reflectionRegistryRegistered", registry != nullptr);
			auto foreign = otherWorld.SpawnActor<RocketProjectileActor>(targets.front().get(), RocketPresentationProfile{}, std::nullopt).lock();
			check("crossWorldRejected", !ProjectileReflectionService::TryReflectProjectile(*foreign, *owner));
			auto rocket = world.SpawnActor<RocketProjectileActor>(targets.front().get(), RocketPresentationProfile{}, std::nullopt).lock();
			check("sameWorldReflects", ProjectileReflectionService::TryReflectProjectile(*rocket, *owner) && rocket->GetOwnerActor() == owner.get());
			check("sameRegistryReused", world.FindServiceActor<ProjectileReflectionRegistryActor>() == registry);
			check("emptyWorldLookup", !otherWorld.FindServiceActor<ProjectileReflectionRegistryActor>());

			// Exact endpoint geometry, using the beam's static sweep policy.
			auto wall = world.SpawnActor<EndpointWall>().lock();
			wall->SetActorLocation({ 115.f, 500.f });
			world.TickInternal(0.f);
			movement::StaticGeometrySweepHit hit;
			check("endpointWallHit", movement::FindFirstStaticGeometryHit(*owner, { 0.f, 500.f }, { 100.f, 500.f }, 5.f, hit, false) && hit.fraction == 1.f);

			// Rotate an already indexed body-less box across manual-grid cells.
			auto rotating = world.SpawnActor<RotatingWall>().lock();
			rotating->SetActorLocation({ 128.f, 4128.f });
			world.TickInternal(0.f);
			check("rotationControlOpen", !movement::FindFirstStaticGeometryHit(*owner, { 100.f, 4450.f }, { 155.f, 4450.f }, 0.f, hit, false));
			rotating->SetActorRotation(90.f);
			check("rotationRefreshesCells", movement::FindFirstStaticGeometryHit(*owner, { 100.f, 4450.f }, { 155.f, 4450.f }, 0.f, hit, false));
			rotating->SetActorRotation(0.f);
			check("rotationRemovesOldCells", !movement::FindFirstStaticGeometryHit(*owner, { 100.f, 4450.f }, { 155.f, 4450.f }, 0.f, hit, false));

			// Real shipped activation; Clear runs from the activation delegate.
			auto lifecycleOwner = spawn(3000.f, CollisionLayer::Player);
			world.TickInternal(0.f);
			auto& lifecycle = lifecycleOwner->GetAbilitySystemComponent();
			AbilityLoadoutManager lifecycleLoadout{ lifecycle };
			LifecycleFaultObserver faults{ lifecycle };
			faults.effect.effectId = "E2E.Lifecycle.Persistent";
			faults.effect.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			check("lifecycleEffectApplied", lifecycle.ApplyGameplayEffect(faults.effect).IsValid());
			check("lifecycleAbilityEquipped", lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1));
			auto activatedToken = lifecycle.onAbilityActivated.BindAction(&faults, &LifecycleFaultObserver::Activated);
			auto endedToken = lifecycle.onAbilityEnded.BindAction(&faults, &LifecycleFaultObserver::Ended);
			lifecycle.SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			lifecycle.Tick(0.01f);
			check("tickClearDeferredUntilReturn", faults.visited && faults.liveDuringClear);
			check("liveInstanceMutationRejected", faults.mutationRejected);
			check("teardownRejectsReapplication", faults.additionsRejected);
			check("tickClearCleansWholeComponent", lifecycle.BuildAbilitySnapshots().empty() && lifecycle.BuildGameplayEffectSnapshots().empty() &&
				!lifecycle.GetAttributes().HasAttribute(OwnerAttributeIds::MaxHealth));
			lifecycle.onAbilityActivated.UnbindAction(activatedToken);
			lifecycle.onAbilityEnded.UnbindAction(endedToken);

			// Independent ended/effect-removal faults must not strand other state.
			check("reuseAfterClear", lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1));
			lifecycle.SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			lifecycle.Tick(0.01f);
			check("activeBeforeFaultedClear", lifecycle.GetAbility(sas::AbilitySlot::Ability1)->IsActive());
			check("firstCleanupEffect", lifecycle.ApplyGameplayEffect(faults.effect).IsValid());
			auto secondEffect = faults.effect;
			secondEffect.effectId = "E2E.Lifecycle.Second";
			check("secondCleanupEffect", lifecycle.ApplyGameplayEffect(secondEffect).IsValid());
			lifecycle.GetAttributes().SetBaseValue(OwnerAttributeIds::MaxHealth, 100.f);
			lifecycle.PushPrimaryWeaponOverride(sas::ContentId{ "E2E.Override" }, PrimaryWeaponDefinition{});
			lifecycleOwner->GetCombatRuntime().SetDamageProtection("E2E.Protection", true, true);
			faults.throwOnEnd = true;
			endedToken = lifecycle.onAbilityEnded.BindAction(&faults, &LifecycleFaultObserver::Ended);
			auto removedToken = lifecycle.onGameplayEffectRemoved.BindAction(&faults, &LifecycleFaultObserver::EffectRemoved);
			std::string cleanupError;
			try { lifecycleOwner->GetCombatRuntime().Clear(); }
			catch (const std::runtime_error& error) { cleanupError = error.what(); }
			lifecycle.onAbilityEnded.UnbindAction(endedToken);
			lifecycle.onGameplayEffectRemoved.UnbindAction(removedToken);
			check("cleanupPreservesFirstException", cleanupError == "Injected ended failure");
			check("faultedClearCleansWholeComponent", lifecycle.BuildAbilitySnapshots().empty() && lifecycle.BuildGameplayEffectSnapshots().empty() &&
				!lifecycle.GetAttributes().HasAttribute(OwnerAttributeIds::MaxHealth) && !lifecycle.GetActivePrimaryWeaponOverride());
			check("faultedClearCleansCombatOwner", !lifecycleOwner->GetCombatRuntime().BlocksIncomingDamage() && !lifecycleOwner->GetCombatRuntime().BlocksOutgoingDamage());
			check("reuseAfterFaultedClear", lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1));
			LoadoutObserver rebindClear{ lifecycle, true };
			auto rebindClearToken = lifecycle.onAbilityChanged.BindAction(&rebindClear, &LoadoutObserver::Granted);
			const bool rebindCleared = lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability2);
			lifecycle.onAbilityChanged.UnbindAction(rebindClearToken);
			check("rebindClearNoGhostSlot", !rebindCleared && lifecycle.BuildAbilitySnapshots().empty() &&
				!lifecycleLoadout.GetLoadout().FindAbility(sas::AbilitySlot::Ability2));
			check("reuseAfterRebindClear", lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1));
			lifecycle.GetAttributes().SetBaseValue(OwnerAttributeIds::MaxHealth, 100.f);
			faults.visited = false;
			activatedToken = lifecycle.onAbilityActivated.BindAction(&faults, &LifecycleFaultObserver::Activated);
			check("directActivationExecutes", lifecycle.GetAbility(sas::AbilitySlot::Ability1)->TryActivate());
			lifecycle.onAbilityActivated.UnbindAction(activatedToken);
			check("directActivationClearSafe", faults.visited && faults.liveDuringClear && lifecycle.BuildAbilitySnapshots().empty());
			check("prepareActivationFault", lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1));
			faults.throwAfterClear = true;
			activatedToken = lifecycle.onAbilityActivated.BindAction(&faults, &LifecycleFaultObserver::Activated);
			std::string activationError;
			try { lifecycle.GetAbility(sas::AbilitySlot::Ability1)->TryActivate(); }
			catch (const std::runtime_error& error) { activationError = error.what(); }
			lifecycle.onAbilityActivated.UnbindAction(activatedToken);
			check("clearCompletesDuringExceptionUnwind", activationError == "Injected activation failure after Clear" &&
				lifecycle.BuildAbilitySnapshots().empty() && lifecycle.BuildGameplayEffectSnapshots().empty());

			// Echo suppresses normal instance notifications, but must retain its
			// lifetime boundary when gameplay lifecycle observers request Clear.
			AbilityUseRecord echoRecord;
			echoRecord.abilityId = sas::ContentId{ AbilityData::ReturnProtocol::AbilityId::Basic };
			faults.visited = false;
			auto eventToken = lifecycle.onGameplayEvent.BindAction(&faults, &LifecycleFaultObserver::GameplayEvent);
			lifecycle.InvokeRecordedAbility(echoRecord, {}, 1.f, sas::AbilitySlot::Ability1, true);
			lifecycle.onGameplayEvent.UnbindAction(eventToken);
			check("echoClearCleansInvocation", faults.visited && lifecycle.GetActiveAbilityInvocationCount() == 0 && lifecycle.BuildAbilitySnapshots().empty());

			// Test preparation and committed publication, with/without Clear, plus Echo.
			auto countVisuals = [&]()
			{
				std::size_t count = 0;
				for (const auto& weakVisual : world.GetActorsByTypeIncludingPending<ReturnProtocolVisualActor>())
					if (const auto visual = weakVisual.lock(); visual && !visual->GetIsPendingDestroy()) ++count;
				return count;
			};
			for (const auto phase : { AbilityData::ReturnProtocol::Event::Started, GameplayTags::Event::Ability::Activated })
			{
				for (const bool clear : { false, true })
				{
					const auto baseline = countVisuals();
					if (!lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1))
						throw std::runtime_error("precommit setup grant");
					ActivationBoundaryObserver boundary{ lifecycle, phase, clear };
					boundary.cleanupFault = !clear;
					auto token = lifecycle.onGameplayEvent.BindAction(&boundary, &ActivationBoundaryObserver::Event);
					std::string error;
					try { lifecycle.GetAbility(sas::AbilitySlot::Ability1)->TryActivate(); }
					catch (const std::runtime_error& fault) { error = fault.what(); }
					lifecycle.onGameplayEvent.UnbindAction(token);
					auto probe = world.SpawnActor<RocketProjectileActor>(targets.front().get(), RocketPresentationProfile{}, std::nullopt).lock();
					const bool clean = boundary.visited && boundary.reentryRejected && error == "Injected precommit activation fault" &&
						countVisuals() == baseline && !lifecycle.HasOwnedTag(AbilityData::ReturnProtocol::State::Active) &&
						!ProjectileReflectionService::TryReflectProjectile(*probe, *lifecycleOwner);
					const bool committedPhase = phase == GameplayTags::Event::Ability::Activated;
					result["precommitCases"].push_back({ { "phase", committedPhase ? "committed" : "precommit" },
						{ "clear", clear }, { "cleanupFault", boundary.cleanupFault }, { "clean", clean },
						{ "stateMatchesPhase", boundary.committedStateVisible == committedPhase } });
					if (!clean) throw std::runtime_error("precommit resources leaked");
					if (boundary.committedStateVisible != committedPhase) throw std::runtime_error("activation published before commit");
					if (!clear)
					{
						auto* ability = lifecycle.GetAbility(sas::AbilitySlot::Ability1);
						if (phase == GameplayTags::Event::Ability::Activated)
						{
							if (!ability || !ability->IsOnCooldown()) throw std::runtime_error("committed failure must end with cooldown");
							ability->Tick(ability->GetCooldownRemaining() + 1.f);
						}
						if (!ability || ability->IsActive() || ability->GetCharges() != 1 || ability->IsOnCooldown() || !ability->TryActivate())
							throw std::runtime_error("precommit retry failed");
					}
					lifecycle.Clear();
				}
			}
			check("precommitFailureMatrix", result["precommitCases"].size() == 4);
			const auto visualBaseline = countVisuals();
			ActivationBoundaryObserver echoFault{ lifecycle, AbilityData::ReturnProtocol::Event::Started };
			auto echoFaultToken = lifecycle.onGameplayEvent.BindAction(&echoFault, &ActivationBoundaryObserver::Event);
			threw = false;
			try { lifecycle.InvokeRecordedAbility(echoRecord, {}, 1.f, sas::AbilitySlot::Ability1, true); }
			catch (const std::runtime_error&) { threw = true; }
			lifecycle.onGameplayEvent.UnbindAction(echoFaultToken);
			check("echoPrecommitFailureCleansResources", threw && echoFault.visited && countVisuals() == visualBaseline &&
				!lifecycle.HasOwnedTag(AbilityData::ReturnProtocol::State::Active) && lifecycle.GetActiveAbilityInvocationCount() == 0);
			check("preparePrecommitCancel", lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1));
			ActivationBoundaryObserver cancelStart{ lifecycle, AbilityData::ReturnProtocol::Event::Started, false, true };
			auto cancelToken = lifecycle.onGameplayEvent.BindAction(&cancelStart, &ActivationBoundaryObserver::Event);
			const bool cancelledStart = lifecycle.GetAbility(sas::AbilitySlot::Ability1)->TryActivate();
			lifecycle.onGameplayEvent.UnbindAction(cancelToken);
			check("precommitCancelNoChargeOrResources", !cancelledStart && cancelStart.visited && cancelStart.reentryRejected &&
				countVisuals() == visualBaseline && lifecycle.GetAbility(sas::AbilitySlot::Ability1)->GetCharges() == 1 &&
				!lifecycle.HasOwnedTag(AbilityData::ReturnProtocol::State::Active));
			lifecycle.Clear();

			// A retained view always reads the canonical runtime, even across nested Clear.
			const auto& liveView = lifecycleLoadout.GetLoadout();
			NestedLoadoutObserver nested{ lifecycleLoadout };
			LoadoutObserver nestedClear{ lifecycle, true };
			auto nestedEventToken = lifecycle.onGameplayEvent.BindAction(&nested, &NestedLoadoutObserver::Event);
			auto nestedClearToken = lifecycle.onAbilityGranted.BindAction(&nestedClear, &LoadoutObserver::Granted);
			sas::AbilityEvent nestedEvent;
			nestedEvent.eventTag = GameplayTags::Event::Combat::DamageReceived;
			lifecycle.HandleGameplayEvent(nestedEvent);
			lifecycle.onAbilityGranted.UnbindAction(nestedClearToken);
			check("nestedGrantClearNoGhost", !nested.equipped && lifecycle.BuildAbilitySnapshots().empty() &&
				!liveView.FindAbility(sas::AbilitySlot::Ability2));
			check("liveViewSeesDirectGrant", lifecycle.GrantAbility(*AbilityData::FindShippedAbilityDefinition(AbilityData::ReturnProtocol::AbilityId::Basic),
				sas::AbilitySlot::Ability1).IsValid() && liveView.FindAbility(sas::AbilitySlot::Ability1));
			nestedClearToken = lifecycle.onAbilityChanged.BindAction(&nestedClear, &LoadoutObserver::Granted);
			lifecycle.HandleGameplayEvent(nestedEvent);
			lifecycle.onAbilityChanged.UnbindAction(nestedClearToken);
			lifecycle.onGameplayEvent.UnbindAction(nestedEventToken);
			check("nestedRebindClearNoGhost", !nested.equipped && lifecycle.BuildAbilitySnapshots().empty() &&
				liveView.FindSlot(AbilityData::ReturnProtocol::AbilityId::Basic) == sas::AbilitySlot::None);

			result["effectCases"] = nlohmann::json::array();
			// Attribute callbacks reenter the production component and attribute
			// owner; handles are committed before notification and erased before it.
			auto& effectAttributes = lifecycle.GetAttributes();
			effectAttributes.RegisterAttribute(OwnerAttributeIds::CriticalChance, 1.f);
			const auto reentrantModifier = effectAttributes.AddModifier(
				sas::AttributeModifier{ OwnerAttributeIds::CriticalChance, 1.f }
			);
			AttributeModifierReentry reentryRemoval{ effectAttributes, reentrantModifier };
			const auto reentryToken = effectAttributes.onAttributeChanged.BindAction(
				&reentryRemoval, &AttributeModifierReentry::Changed
			);
			reentryRemoval.armed = true;
			effectAttributes.RemoveModifier(reentrantModifier);
			effectAttributes.onAttributeChanged.UnbindAction(reentryToken);
			const bool reentrantRemoveClean = reentrantModifier.IsValid() && reentryRemoval.visited &&
				effectAttributes.GetCurrentValue(OwnerAttributeIds::CriticalChance) == 1.f;
			result["effectCases"].push_back({ { "case_id", "E01" }, { "input", "attribute change observer removes the same handle" },
				{ "expected", "one removal with consistent current value" },
				{ "actual", { { "callbackVisited", reentryRemoval.visited },
					{ "currentValue", effectAttributes.GetCurrentValue(OwnerAttributeIds::CriticalChance) } } },
				{ "passed", reentrantRemoveClean } });
			check("attributeRemovalReentryIsIdempotent", reentrantRemoveClean);

			const sas::AttributeId clearRegistrationId{ "E2E.Attribute.RegistrationClear" };
			sas::GameplayEffectDefinition clearDuringRegistration;
			clearDuringRegistration.effectId = "E2E.Effect.AttributeRegistrationClear";
			clearDuringRegistration.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			clearDuringRegistration.modifiers.emplace_back(clearRegistrationId, 2.f);
			AttributeRegisteredClearFault registeredClear{ effectAttributes };
			const auto clearRegisteredToken = effectAttributes.onAttributeRegistered.BindAction(
				&registeredClear, &AttributeRegisteredClearFault::Registered
			);
			const auto clearedRegistrationHandle = lifecycle.ApplyGameplayEffect(clearDuringRegistration);
			effectAttributes.onAttributeRegistered.UnbindAction(clearRegisteredToken);
			const bool registrationClearSafe = registeredClear.visited && clearedRegistrationHandle.IsValid() &&
				!effectAttributes.HasAttribute(clearRegistrationId) &&
				effectAttributes.GetCurrentValue(clearRegistrationId) == 0.f;
			lifecycle.RemoveGameplayEffect(clearedRegistrationHandle);
			result["effectCases"].push_back({ { "case_id", "E02-registration-clear" },
				{ "input", "implicit registration observer clears AttributeSystem" },
				{ "expected", "in-flight modifier invalidated without stale access" },
				{ "actual", { { "callbackVisited", registeredClear.visited },
					{ "attributePresent", effectAttributes.HasAttribute(clearRegistrationId) },
					{ "effectRemoved", lifecycle.BuildGameplayEffectSnapshots().empty() } } },
				{ "passed", registrationClearSafe && lifecycle.BuildGameplayEffectSnapshots().empty() } });
			check("implicitRegistrationClearInvalidatesAddSafely", registrationClearSafe && lifecycle.BuildGameplayEffectSnapshots().empty());

			const sas::AttributeId throwRegistrationId{ "E2E.Attribute.RegistrationThrow" };
			sas::GameplayEffectDefinition throwDuringRegistration = clearDuringRegistration;
			throwDuringRegistration.effectId = "E2E.Effect.AttributeRegistrationThrow";
			throwDuringRegistration.modifiers.clear();
			throwDuringRegistration.modifiers.emplace_back(throwRegistrationId, 3.f);
			AttributeRegisteredThrowFault registeredThrow{ "Injected E02 registration failure" };
			const auto throwRegisteredToken = effectAttributes.onAttributeRegistered.BindAction(
				&registeredThrow, &AttributeRegisteredThrowFault::Registered
			);
			std::string registrationError;
			try { lifecycle.ApplyGameplayEffect(throwDuringRegistration); }
			catch (const std::runtime_error& error) { registrationError = error.what(); }
			effectAttributes.onAttributeRegistered.UnbindAction(throwRegisteredToken);
			const bool registrationThrowClean = registeredThrow.visited &&
				registrationError == registeredThrow.message &&
				lifecycle.BuildGameplayEffectSnapshots().empty();
			result["effectCases"].push_back({ { "case_id", "E02-registration-throw" },
				{ "input", "implicit registration observer throws after effect records modifier ownership" },
				{ "expected", "first error preserved and failed effect/ledger removed" },
				{ "actual", { { "error", registrationError },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() } } },
				{ "passed", registrationThrowClean } });
			check("implicitRegistrationThrowCleansCommittedHandle", registrationThrowClean);

			const sas::AttributeId clearChangedId{ "E2E.Attribute.ChangeClear" };
			effectAttributes.RegisterAttribute(clearChangedId, 0.f);
			sas::GameplayEffectDefinition clearDuringChange;
			clearDuringChange.effectId = "E2E.Effect.AttributeChangeClear";
			clearDuringChange.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			clearDuringChange.modifiers.emplace_back(clearChangedId, 4.f);
			AttributeChangedClearFault changedClear{ effectAttributes, clearChangedId, true };
			const auto changedClearToken = effectAttributes.onAttributeChanged.BindAction(
				&changedClear, &AttributeChangedClearFault::Changed
			);
			const auto clearedChangeHandle = lifecycle.ApplyGameplayEffect(clearDuringChange);
			effectAttributes.onAttributeChanged.UnbindAction(changedClearToken);
			const bool changedClearSafe = changedClear.visited && clearedChangeHandle.IsValid() &&
				!effectAttributes.HasAttribute(clearChangedId) &&
				effectAttributes.GetCurrentValue(clearChangedId) == 0.f;
			lifecycle.RemoveGameplayEffect(clearedChangeHandle);
			result["effectCases"].push_back({ { "case_id", "E02-change-clear" },
				{ "input", "attribute changed callback clears AttributeSystem after modifier commit" },
				{ "expected", "no stale modifier access and effect cleanup succeeds" },
				{ "actual", { { "callbackVisited", changedClear.visited },
					{ "attributePresent", effectAttributes.HasAttribute(clearChangedId) },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() } } },
				{ "passed", changedClearSafe && lifecycle.BuildGameplayEffectSnapshots().empty() } });
			check("attributeChangeClearLeavesNoStaleModifier", changedClearSafe && lifecycle.BuildGameplayEffectSnapshots().empty());

			const sas::AttributeId throwChangedId{ "E2E.Attribute.ChangeThrow" };
			effectAttributes.RegisterAttribute(throwChangedId, 0.f);
			sas::GameplayEffectDefinition throwDuringChange;
			throwDuringChange.effectId = "E2E.Effect.AttributeChangeThrow";
			throwDuringChange.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			throwDuringChange.modifiers.emplace_back(throwChangedId, 5.f);
			AttributeChangedClearFault changedThrow{ effectAttributes, throwChangedId, false, false,
				"Injected E02 attribute change failure" };
			const auto changedThrowToken = effectAttributes.onAttributeChanged.BindAction(
				&changedThrow, &AttributeChangedClearFault::Changed
			);
			std::string attributeChangeError;
			try { lifecycle.ApplyGameplayEffect(throwDuringChange); }
			catch (const std::runtime_error& error) { attributeChangeError = error.what(); }
			effectAttributes.onAttributeChanged.UnbindAction(changedThrowToken);
			const bool changedThrowClean = changedThrow.visited &&
				attributeChangeError == changedThrow.message &&
				lifecycle.BuildGameplayEffectSnapshots().empty() &&
				effectAttributes.GetCurrentValue(throwChangedId) == 0.f;
			result["effectCases"].push_back({ { "case_id", "E02-change-throw" },
				{ "input", "attribute changed callback throws after modifier commit" },
				{ "expected", "effect cleanup knows committed handle and rethrows original failure" },
				{ "actual", { { "error", attributeChangeError }, { "currentValue", effectAttributes.GetCurrentValue(throwChangedId) },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() } } },
				{ "passed", changedThrowClean } });
			check("attributeChangeThrowCleansCommittedHandle", changedThrowClean);

			// Effect callbacks exercise the production runtime against this real
			// SpaceShip component. Each case records its observed ownership state.
			const GameplayTag selfRemoveTag{ "E2E.Effect.SelfRemove" };
			sas::GameplayEffectDefinition selfRemoving;
			selfRemoving.effectId = "E2E.Effect.SelfRemoving";
			selfRemoving.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			selfRemoving.grantedTags.push_back(selfRemoveTag);
			selfRemoving.modifiers.emplace_back(OwnerAttributeIds::CriticalDamage, 0.5f);
			auto selfRemoveCallbacks = sas::AbilitySystemComponent::EffectCallbacks{};
			selfRemoveCallbacks.initialize = [&lifecycle](sas::ActiveGameplayEffect& effect)
			{
				lifecycle.RemoveGameplayEffect(effect.handle);
			};
			lifecycle.SetEffectRuntimeCallbacks(std::move(selfRemoveCallbacks));
			const float criticalDamageBeforeSelfRemove = lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalDamage);
			const auto selfRemovedHandle = lifecycle.ApplyGameplayEffect(selfRemoving);
			const bool selfRemoveClean = !selfRemovedHandle.IsValid() &&
				lifecycle.BuildGameplayEffectSnapshots().empty() &&
				!lifecycle.HasOwnedTag(selfRemoveTag, true) &&
				lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalDamage) == criticalDamageBeforeSelfRemove;
			result["effectCases"].push_back({ { "case_id", "E03" }, { "input", "initialize removes its own effect" },
				{ "expected", "invalid result with no owned resources" }, { "actual", {
					{ "validHandle", selfRemovedHandle.IsValid() },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() },
					{ "tagOwned", lifecycle.HasOwnedTag(selfRemoveTag, true) },
					{ "criticalDamage", lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalDamage) } } },
				{ "passed", selfRemoveClean } });
			check("effectApplySelfRemoveCleansAcquiredResources", selfRemoveClean);

			const GameplayTag refreshTag{ "E2E.Effect.RefreshRemove" };
			sas::GameplayEffectDefinition refreshRemoving;
			refreshRemoving.effectId = "E2E.Effect.RefreshRemoving";
			refreshRemoving.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
			refreshRemoving.duration = 10.f;
			refreshRemoving.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
			refreshRemoving.grantedTags.push_back(refreshTag);
			refreshRemoving.modifiers.emplace_back(OwnerAttributeIds::CriticalChance, 0.2f);
			auto refreshCallbacks = sas::AbilitySystemComponent::EffectCallbacks{};
			refreshCallbacks.changed = [&lifecycle](sas::ActiveGameplayEffect& effect)
			{
				lifecycle.RemoveGameplayEffect(effect.handle);
			};
			lifecycle.SetEffectRuntimeCallbacks(std::move(refreshCallbacks));
			const float criticalChanceBeforeRefresh = lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalChance);
			const auto refreshHandle = lifecycle.ApplyGameplayEffect(refreshRemoving);
			const bool refreshResult = refreshHandle.IsValid() && lifecycle.RefreshGameplayEffectDuration(refreshHandle);
			const bool refreshRemoved = refreshHandle.IsValid() && !refreshResult &&
				lifecycle.BuildGameplayEffectSnapshots().empty() && !lifecycle.HasOwnedTag(refreshTag, true) &&
				lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalChance) == criticalChanceBeforeRefresh;
			result["effectCases"].push_back({ { "case_id", "E04-refresh" }, { "input", "duration refresh callback removes active effect" },
				{ "expected", "false result and no active resources" }, { "actual", { { "refreshResult", refreshResult },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() }, { "tagOwned", lifecycle.HasOwnedTag(refreshTag, true) } } },
				{ "passed", refreshRemoved } });
			check("effectRefreshSelfRemoveCleansAcquiredResources", refreshRemoved);

			const GameplayTag stackTag{ "E2E.Effect.StackClear" };
			sas::GameplayEffectDefinition stackClearing;
			stackClearing.effectId = "E2E.Effect.StackClearing";
			stackClearing.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			stackClearing.stackingPolicy = sas::GameplayEffectStackingPolicy::Stack;
			stackClearing.maxStacks = 2;
			stackClearing.grantedTags.push_back(stackTag);
			stackClearing.modifiers.emplace_back(OwnerAttributeIds::Luck, 1.f);
			auto stackCallbacks = sas::AbilitySystemComponent::EffectCallbacks{};
			stackCallbacks.addStack = [&lifecycle](sas::ActiveGameplayEffect&) -> bool
			{
				lifecycle.Clear();
				return true;
			};
			lifecycle.SetEffectRuntimeCallbacks(std::move(stackCallbacks));
			const auto stackHandle = lifecycle.ApplyGameplayEffect(stackClearing);
			const auto stackReapply = lifecycle.ApplyGameplayEffect(stackClearing);
			const bool stackClearClean = stackHandle.IsValid() && !stackReapply.IsValid() &&
				lifecycle.BuildGameplayEffectSnapshots().empty() && !lifecycle.HasOwnedTag(stackTag, true) &&
				!lifecycle.GetAttributes().HasAttribute(OwnerAttributeIds::Luck);
			result["effectCases"].push_back({ { "case_id", "E04-stack-clear" }, { "input", "stack callback requests component Clear" },
				{ "expected", "invalid reapply and fully cleared component" }, { "actual", { { "reapplyValid", stackReapply.IsValid() },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() }, { "tagOwned", lifecycle.HasOwnedTag(stackTag, true) } } },
				{ "passed", stackClearClean } });
			check("effectStackClearStopsFurtherCallbacks", stackClearClean);

			// A callback failure on each cleanup position must not strand later
			// modifiers, tag ownership, or the effect record. Handles unwind LIFO.
			lifecycle.SetEffectRuntimeCallbacks({});
			const std::vector<sas::AttributeId> cleanupAttributes{
				OwnerAttributeIds::CriticalChance, OwnerAttributeIds::CriticalDamage, OwnerAttributeIds::Luck
			};
			for (std::size_t stage = 0; stage < cleanupAttributes.size(); ++stage)
			{
				sas::GameplayEffectDefinition cleanupEffect;
				cleanupEffect.effectId = "E2E.Effect.Cleanup." + std::to_string(stage);
				cleanupEffect.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
				const GameplayTag cleanupTag{ "E2E.Effect.Cleanup." + std::to_string(stage) };
				cleanupEffect.grantedTags.push_back(cleanupTag);
				for (const sas::AttributeId& id : cleanupAttributes) cleanupEffect.modifiers.emplace_back(id, 1.f);
				std::vector<float> before;
				for (const sas::AttributeId& id : cleanupAttributes) before.push_back(lifecycle.GetAttributes().GetCurrentValue(id));
				const auto cleanupHandle = lifecycle.ApplyGameplayEffect(cleanupEffect);
				AttributeCleanupFault fault{
					cleanupAttributes[cleanupAttributes.size() - 1 - stage],
					"Injected E05 cleanup stage " + std::to_string(stage), true, false
				};
				const auto token = lifecycle.GetAttributes().onAttributeChanged.BindAction(&fault, &AttributeCleanupFault::Changed);
				std::string cleanupFault;
				try { lifecycle.RemoveGameplayEffect(cleanupHandle); }
				catch (const std::runtime_error& error) { cleanupFault = error.what(); }
				lifecycle.GetAttributes().onAttributeChanged.UnbindAction(token);
				bool valuesRestored = true;
				for (std::size_t i = 0; i < cleanupAttributes.size(); ++i)
				{
					valuesRestored = valuesRestored &&
						std::abs(lifecycle.GetAttributes().GetCurrentValue(cleanupAttributes[i]) - before[i]) < 0.0001f;
				}
				const bool cleanupComplete = cleanupHandle.IsValid() && fault.triggered &&
					cleanupFault == fault.message && valuesRestored && !lifecycle.HasOwnedTag(cleanupTag, true) &&
					lifecycle.BuildGameplayEffectSnapshots().empty();
				result["effectCases"].push_back({ { "case_id", "E05-" + std::to_string(stage) },
					{ "input", { { "throwPosition", stage == 0 ? "first" : stage == 1 ? "middle" : "last" },
						{ "modifierCount", cleanupAttributes.size() } } },
					{ "expected", "all owned resources cleared; original callback error rethrown" },
					{ "actual", { { "error", cleanupFault }, { "valuesRestored", valuesRestored },
						{ "tagOwned", lifecycle.HasOwnedTag(cleanupTag, true) },
						{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() } } },
					{ "passed", cleanupComplete } });
				check(stage == 0 ? "effectCleanupThrowFirstContinues" : stage == 1 ? "effectCleanupThrowMiddleContinues" : "effectCleanupThrowLastContinues", cleanupComplete);
				const auto reuseHandle = lifecycle.ApplyGameplayEffect(cleanupEffect);
				lifecycle.RemoveGameplayEffect(reuseHandle);
				const std::string reuseAssertion = "effectRuntimeReusableAfterCleanupThrow" + std::to_string(stage);
				check(reuseAssertion.c_str(), reuseHandle.IsValid() && lifecycle.BuildGameplayEffectSnapshots().empty());
			}

			const GameplayTag retryCleanupTag{ "E2E.Effect.RetryCleanup" };
			sas::GameplayEffectDefinition retryCleanupEffect;
			retryCleanupEffect.effectId = "E2E.Effect.RetryCleanup";
			retryCleanupEffect.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			retryCleanupEffect.grantedTags.push_back(retryCleanupTag);
			retryCleanupEffect.modifiers.emplace_back(OwnerAttributeIds::EnergyPower, 1.f);
			int removalAttempts = 0;
			auto retryCallbacks = sas::AbilitySystemComponent::EffectCallbacks{};
			retryCallbacks.removing = [&removalAttempts](sas::ActiveGameplayEffect&)
			{
				if (removalAttempts++ == 0) throw std::runtime_error("Injected E05 incomplete presentation cleanup");
			};
			lifecycle.SetEffectRuntimeCallbacks(std::move(retryCallbacks));
			const auto retryCleanupHandle = lifecycle.ApplyGameplayEffect(retryCleanupEffect);
			std::string retryCleanupError;
			try { lifecycle.Clear(); }
			catch (const std::runtime_error& error) { retryCleanupError = error.what(); }
			const bool cleanupDebtRetained = retryCleanupError == "Injected E05 incomplete presentation cleanup" &&
				removalAttempts == 1 && lifecycle.BuildGameplayEffectSnapshots().size() == 1 &&
				!lifecycle.HasOwnedTag(retryCleanupTag, true) &&
				!lifecycle.ApplyGameplayEffect(retryCleanupEffect).IsValid();
			lifecycle.Clear();
			const auto retryAfterCleanupDebt = lifecycle.ApplyGameplayEffect(retryCleanupEffect);
			lifecycle.RemoveGameplayEffect(retryAfterCleanupDebt);
			const bool cleanupDebtRetried = retryCleanupHandle.IsValid() && cleanupDebtRetained &&
				removalAttempts == 3 && retryAfterCleanupDebt.IsValid() &&
				lifecycle.BuildGameplayEffectSnapshots().empty();
			result["effectCases"].push_back({ { "case_id", "E05-clear-retry" },
				{ "input", "removing cleanup callback fails once during full Clear" },
				{ "expected", "record retained as cleanup debt; apply blocked until retry succeeds" },
				{ "actual", { { "firstError", retryCleanupError }, { "attempts", removalAttempts },
					{ "recordRetainedBeforeRetry", cleanupDebtRetained },
					{ "reusableAfterRetry", retryAfterCleanupDebt.IsValid() } } },
				{ "passed", cleanupDebtRetried } });
			check("effectClearRetainsAndRetriesIncompleteCleanup", cleanupDebtRetried);

			// Shared tag counts and attribute modifiers remain owned independently.
			const GameplayTag sharedEffectTag{ "E2E.Effect.Shared" };
			sas::GameplayEffectDefinition sharedFirst;
			sharedFirst.effectId = "E2E.Effect.Shared.First";
			sharedFirst.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			sharedFirst.grantedTags.push_back(sharedEffectTag);
			sharedFirst.modifiers.emplace_back(OwnerAttributeIds::EnergyPower, 1.f);
			auto sharedSecond = sharedFirst;
			sharedSecond.effectId = "E2E.Effect.Shared.Second";
			const float energyBeforeShared = lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower);
			const auto sharedFirstHandle = lifecycle.ApplyGameplayEffect(sharedFirst);
			const auto sharedSecondHandle = lifecycle.ApplyGameplayEffect(sharedSecond);
			lifecycle.RemoveGameplayEffect(sharedFirstHandle);
			const bool sharedOwnershipIsolated = sharedFirstHandle.IsValid() && sharedSecondHandle.IsValid() &&
				lifecycle.HasOwnedTag(sharedEffectTag, true) &&
				lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower) == energyBeforeShared + 1.f;
			lifecycle.RemoveGameplayEffect(sharedSecondHandle);
			const bool sharedOwnershipReleased = sharedOwnershipIsolated && !lifecycle.HasOwnedTag(sharedEffectTag, true) &&
				lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower) == energyBeforeShared;
			result["effectCases"].push_back({ { "case_id", "E06" }, { "input", "two effects own same tag and attribute" },
				{ "expected", "removing one retains the other contribution" }, { "actual", { { "retainedAfterFirstRemoval", sharedOwnershipIsolated },
					{ "releasedAfterSecondRemoval", sharedOwnershipReleased } } }, { "passed", sharedOwnershipReleased } });
			check("effectSharedTagAndModifierOwnership", sharedOwnershipReleased);

			// Same stacking-key reentry is rejected, while an independent nested
			// effect is admitted through the same live component.
			sas::GameplayEffectDefinition nestedOuter;
			nestedOuter.effectId = "E2E.Effect.Nested.Outer";
			nestedOuter.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
			nestedOuter.duration = 10.f;
			nestedOuter.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
			sas::GameplayEffectDefinition nestedOther = nestedOuter;
			nestedOther.effectId = "E2E.Effect.Nested.Other";
			bool nestedApplied = false;
			sas::GameplayEffectHandle sameKeyResult, otherEffectResult;
			auto nestedCallbacks = sas::AbilitySystemComponent::EffectCallbacks{};
			nestedCallbacks.activated = [&](sas::ActiveGameplayEffect& effect)
			{
				if (effect.spec.definition.effectId != nestedOuter.effectId || nestedApplied) return;
				nestedApplied = true;
				sameKeyResult = lifecycle.ApplyGameplayEffect(nestedOuter);
				otherEffectResult = lifecycle.ApplyGameplayEffect(nestedOther);
			};
			lifecycle.SetEffectRuntimeCallbacks(std::move(nestedCallbacks));
			const auto nestedOuterResult = lifecycle.ApplyGameplayEffect(nestedOuter);
			const bool nestedKeyPolicy = nestedOuterResult.IsValid() && nestedApplied &&
				!sameKeyResult.IsValid() && otherEffectResult.IsValid() &&
				lifecycle.BuildGameplayEffectSnapshots().size() == 2;
			result["effectCases"].push_back({ { "case_id", "E07" }, { "input", "activated callback applies same stacking key and a distinct effect" },
				{ "expected", "same key rejected, independent effect accepted" }, { "actual", { { "sameKeyValid", sameKeyResult.IsValid() },
					{ "otherEffectValid", otherEffectResult.IsValid() }, { "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() } } },
				{ "passed", nestedKeyPolicy } });
			check("effectSameKeyNestedApplyRejectedOtherEffectAccepted", nestedKeyPolicy);
			lifecycle.SetEffectRuntimeCallbacks({});
			lifecycle.RemoveGameplayEffect(nestedOuterResult);
			lifecycle.RemoveGameplayEffect(otherEffectResult);
			check("effectNestedApplyCleanup", lifecycle.BuildGameplayEffectSnapshots().empty());

			// Damage callback grows indexed storage and refreshes its live entry.
			auto nanoTarget = spawn(3500.f, CollisionLayer::Enemy);
			auto reentryController = NanoPlagueControllerActor::FindOrCreate(world, *owner, NanoPlaguePresentationProfile{});
			NanoReentryObserver reentry{ reentryController, nanoTarget };
			reentry.settings.baseTickDamage = 1.f;
			reentry.settings.energyPowerTickScale = 0.f;
			for (int i = 0; i < 32; ++i) reentry.additions.push_back(spawn(3600.f + i * 20.f, CollisionLayer::Enemy));
			check("nanoReentryApplied", reentryController->ApplyOrRefreshInfection(*nanoTarget, 1, reentry.settings));
			world.TickInternal(0.f);
			const auto countBefore = AuditFixesE2EAccess::Count(*reentryController);
			const auto recordBefore = AuditFixesE2EAccess::FirstRecord(*reentryController);
			auto damageToken = nanoTarget->GetCombatRuntime().onDamageResolved.BindAction(&reentry, &NanoReentryObserver::Damaged);
			reentryController->Tick(0.25f);
			check("nanoCallbackGrowsStableStorage", reentry.visited && AuditFixesE2EAccess::Count(*reentryController) == countBefore + 32 &&
				AuditFixesE2EAccess::FirstRecord(*reentryController) == recordBefore);
			result["outcome"]["nanoRecordsBefore"] = countBefore;
			result["outcome"]["nanoRecordsAfter"] = AuditFixesE2EAccess::Count(*reentryController);
			result["outcome"]["cleanupError"] = cleanupError;
			result["outcome"]["activationError"] = activationError;
			check("nanoRefreshOwnsNewCounters", AuditFixesE2EAccess::Ticks(*reentryController, *nanoTarget) == 0);
			check("nanoNewInfectionsStartNextTick", AuditFixesE2EAccess::Ticks(*reentryController, *reentry.additions.front()) == 0);
			reentryController->Tick(0.25f);
			check("nanoContinuesAfterReentry", AuditFixesE2EAccess::Ticks(*reentryController, *nanoTarget) == 1 &&
				AuditFixesE2EAccess::Ticks(*reentryController, *reentry.additions.front()) == 1);
			nanoTarget->GetCombatRuntime().onDamageResolved.UnbindAction(damageToken);
			reentryController->Tick(10.f);
			check("nanoBatchExpiryClearsIndexedRecords", AuditFixesE2EAccess::Count(*reentryController) == 0 &&
				!nanoTarget->GetAbilitySystemComponent().HasOwnedTag(AbilityData::NanoPlague::State::Infected));
			reentryController->Destroy();
			result["passed"] = true;
		}
		catch (const std::exception& error) { result["error"] = error.what(); }
		const std::filesystem::path path{ artifactPath };
		std::filesystem::create_directories(path.parent_path());
		std::ofstream output{ path, std::ios::out | std::ios::trunc };
		output << result.dump(2) << '\n';
		return output.good() && result["passed"].get<bool>() ? 0 : 1;
	}
}
