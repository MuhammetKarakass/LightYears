#include "EnemyCombatFoundationTests.h"
#include "gameplay/enemy/EnemyRuntime.h"
#include "gameplay/enemy/EnemyCombatProfile.h"
#include "gameplay/enemy/EnemyIds.h"
#include "gameplay/enemy/EnemyBehaviorDecision.h"
#include "gameplay/enemy/EnemyBehaviorProfileValidator.h"
#include "gameplay/enemy/EnemyBehaviorRuntime.h"
#include "gameplay/encounter/EncounterWaveRuntime.h"
#include "gameplay/content/EnemyCombatProfileCatalog.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/CombatRuntimeModifiers.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/GameAbility.h"
#include "enemy/EnemyActor.h"
#include "enemy/DummyEnemy.h"
#include "framework/World.h"
#include "framework/Actor.h"
#include "framework/Application.h"
#include "framework/AssetManager.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h"
#include "gameplay/weapon/wave/ExpandingWaveWeaponActor.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/EnemyFactory.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/damage/DamageContext.h"
#include "level/ArenaTestLevel.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "presentation/hud/encounter/EncounterHUDController.h"
#include "widget/GameHUD.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "abilities/AbilityTypes.h"
#include "abilities/AbilityHandle.h"
#include "abilities/AbilityPolicies.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/content/AbilityContentCatalog.h"

#include <iostream>
#include <algorithm>
#include <filesystem>
#include <cmath>
#include <limits>
#include <vector>

namespace ly
{
	namespace
	{
		bool Fail(const char* message)
		{
			std::cerr << "[FAIL] " << message << '\n';
			return false;
		}

		bool NearlyEqual(float a, float b, float epsilon = 0.001f)
		{
			return std::fabs(a - b) <= epsilon;
		}

		bool SameSnapshot(const EncounterWaveSnapshot& left, const EncounterWaveSnapshot& right)
		{
			return left.state == right.state &&
				left.sequenceMode == right.sequenceMode &&
				left.currentWaveNumber == right.currentWaveNumber &&
				left.totalWaveCount == right.totalWaveCount &&
				left.plannedEnemyCount == right.plannedEnemyCount &&
				left.spawnedEnemyCount == right.spawnedEnemyCount &&
				left.aliveEnemyCount == right.aliveEnemyCount &&
				left.remainingSpawnCount == right.remainingSpawnCount &&
				NearlyEqual(left.interWaveRemainingTime, right.interWaveRemainingTime) &&
				left.enemyLevel == right.enemyLevel;
		}

		class MockEnemyAbilityOperations final : public IEnemyAbilityOperations
		{
		public:
			explicit MockEnemyAbilityOperations(CombatRuntime& combatRuntime)
				: mCombatRuntime{ combatRuntime }
			{
			}

			void SetFailOnRemoveHandle(sas::AbilityHandle handle)
			{
				mFailRemoveHandle = handle;
			}

			void SetFailOnRemoveIndex(int index)
			{
				mFailRemoveIndex = index;
			}

			void SetFailOnGrant(bool fail)
			{
				mFailGrant = fail;
			}

			void SetFailOnGrantIndex(int index)
			{
				mGrantCallCount = 0;
				mFailGrantIndex = index;
			}

			sas::AbilityHandle GrantAbility(
				const GameAbilityDefinition& definition,
				sas::AbilitySlot slot = sas::AbilitySlot::None,
				std::string* failureReason = nullptr
			) override
			{
				mGrantCallCount++;
				if (mFailGrant || mGrantCallCount == mFailGrantIndex)
				{
					if (failureReason) *failureReason = "Simulated grant failure";
					return {};
				}
				return mCombatRuntime.GetAbilitySystemComponent().GrantAbility(definition, slot, failureReason);
			}

			bool RemoveAbility(
				sas::AbilityHandle handle,
				sas::AbilityEndReason reason = sas::AbilityEndReason::Cancelled
			) override
			{
				mRemoveCallCount++;
				if (handle == mFailRemoveHandle || mRemoveCallCount == mFailRemoveIndex)
				{
					return false;
				}
				return mCombatRuntime.GetAbilitySystemComponent().RemoveAbility(handle, reason);
			}

			bool SetAbilityLevel(sas::AbilityHandle handle, int level) override
			{
				return mCombatRuntime.GetAbilitySystemComponent().SetAbilityLevel(handle, level);
			}

			const GameAbility* GetAbility(sas::AbilitySlot slot) const override
			{
				return mCombatRuntime.GetAbilitySystemComponent().GetAbility(slot);
			}

			const GameAbility* GetAbility(sas::AbilityHandle handle) const override
			{
				return mCombatRuntime.GetAbilitySystemComponent().GetAbility(handle);
			}

			const GameAbility* GetAbilityById(const std::string& abilityId) const override
			{
				return mCombatRuntime.GetAbilitySystemComponent().GetAbilityById(abilityId);
			}

			std::size_t GetPassiveAbilityCount() const override
			{
				return mCombatRuntime.GetAbilitySystemComponent().GetPassiveAbilityCount();
			}

			std::size_t GetMaxPassiveAbilityCount() const override
			{
				return LightYearsAbilitySystemComponent::MaxPassiveAbilities;
			}

			int mRemoveCallCount = 0;
			int mGrantCallCount = 0;

		private:
			CombatRuntime& mCombatRuntime;
			sas::AbilityHandle mFailRemoveHandle;
			int mFailRemoveIndex = -1;
			int mFailGrantIndex = -1;
			bool mFailGrant = false;
		};

		class TestCombatant : public Actor, public Combatant
		{
		public:
			explicit TestCombatant(World* world)
				: Actor(world)
				, mCombatRuntime(*this)
			{
			}

			CombatRuntime& GetCombatRuntime() override { return mCombatRuntime; }
			const CombatRuntime& GetCombatRuntime() const override { return mCombatRuntime; }
			LightYearsAbilitySystemComponent& GetAbilitySystemComponent() override { return mCombatRuntime.GetAbilitySystemComponent(); }
			const LightYearsAbilitySystemComponent& GetAbilitySystemComponent() const override { return mCombatRuntime.GetAbilitySystemComponent(); }
			float GetPhysicsCollisionRadius() const override { return 20.f; }
			sf::Vector2f GetPhysicsCollisionBoxHalfExtents() const override { return { 20.f, 20.f }; }

			void ReceiveDamage(DamageContext context) override
			{
				lastReceivedContext = context;
				mCombatRuntime.ProcessIncomingDamage(lastReceivedContext);
				mCombatRuntime.ApplyHullDamageMitigation(lastReceivedContext);
				lastProcessedContext = lastReceivedContext;
				mTestHealth = std::max(0.f, mTestHealth - lastProcessedContext.remainingDamage);
			}

			void InitializeTestHealth(float health) { mTestHealth = health; }
			float GetTestHealth() const { return mTestHealth; }

			CombatRuntime mCombatRuntime;
			DamageContext lastReceivedContext{};
			DamageContext lastProcessedContext{};
			float mTestHealth = 0.f;
		};

		class ArenaLifecycleTestLevel final : public ArenaTestLevel
		{
		public:
			explicit ArenaLifecycleTestLevel(Application* owningApp)
				: ArenaTestLevel(owningApp)
			{
			}

			void SetEncounterPlayer(weak_ptr<PlayerSpaceShip> playerShip)
			{
				mTestPlayerShip = playerShip;
			}

			void TickEncounter(float deltaTime)
			{
				Tick(deltaTime);
			}

			void RestartLevelForTest()
			{
				OnRestartLevel();
			}

			void TriggerBoundaryPenaltyForTest()
			{
				OnArenaBoundaryPenaltyTriggered(weak_ptr<Actor>{});
			}

		protected:
			void OnGameStart() override
			{
				ArenaLevel::OnGameStart();
			}

			weak_ptr<PlayerSpaceShip> ResolveEncounterPlayerShip() const override
			{
				return mTestPlayerShip.expired()
					? ArenaTestLevel::ResolveEncounterPlayerShip()
					: mTestPlayerShip;
			}

		private:
			weak_ptr<PlayerSpaceShip> mTestPlayerShip;
		};

		// GameHUD keeps its widget list protected, so the encounter HUD checks observe it through this probe.
		class EncounterHUDTestHUD final : public GameHUD
		{
		public:
			size_t GetWidgetCount() const { return mWidgets.size(); }
			size_t GetVisibleWidgetCount() const
			{
				size_t visibleCount = 0;
				for (const shared_ptr<Widget>& widget : mWidgets)
				{
					if (widget && widget->GetVisibility()) ++visibleCount;
				}
				return visibleCount;
			}
		};

		// Mirrors the provider ArenaTestLevel registers: a weak level that degrades to a neutral snapshot.
		EncounterHUDController::SnapshotProvider MakeWeakLevelSnapshotProvider(weak_ptr<ArenaTestLevel> weakLevel)
		{
			return [weakLevel]()
			{
				if (const shared_ptr<ArenaTestLevel> level = weakLevel.lock()) return level->GetEncounterWaveSnapshot();
				return EncounterWaveSnapshot{};
			};
		}

	}

	int RunEnemyCombatFoundationTests()
	{
		std::cout << std::unitbuf;
		const std::filesystem::path candidateAssetRoots[] = {
			"build/LightYearsGame/assets",
			"LightYearsGame/assets",
			"assets"
		};
		for (const auto& candidate : candidateAssetRoots)
		{
			if (std::filesystem::exists(candidate))
			{
				AssetManager::GetAssetManager().SetAssetRootDirectory(candidate.generic_string() + "/");
				break;
			}
		}
		GameContentBootstrap::Register();

		std::cout << "[TEST Behavior Contracts]" << std::endl;
		{
			EnemyBehaviorProfile approach{ "EnemyBehavior.Test.Approach", 1000.f, 0.2f, 400.f, 200.f, 0.f, EnemyMovementMode::Approach, 1.f, 1.f };
			approach.slotRules.push_back({ sas::AbilitySlot::PrimaryFire, EnemySlotInputMode::Hold, true, 0.f, 500.f, 0.9f, 0.25f, 0 });
			const EnemyBehaviorProfile holdRange{ "EnemyBehavior.Test.HoldRange", 1000.f, 0.2f, 0.f, 300.f, 600.f, EnemyMovementMode::HoldRange, 1.f, 1.f };
			const EnemyBehaviorProfile strafe{ "EnemyBehavior.Test.Strafe", 1000.f, 0.2f, 0.f, 300.f, 600.f, EnemyMovementMode::Strafe, 1.f, 1.f };
			for (const EnemyBehaviorProfile& profile : { approach, holdRange, strafe })
			{
				std::string failureReason;
				if (!EnemyBehaviorProfileValidator::Validate(profile, &failureReason)) return Fail(failureReason.c_str()) ? 0 : 1;
			}
			EnemyBehaviorProfile invalid = approach;
			invalid.targetSearchRange = std::numeric_limits<float>::quiet_NaN();
			if (EnemyBehaviorProfileValidator::Validate(invalid)) return Fail("Behavior validator accepted NaN") ? 0 : 1;
			invalid = holdRange;
			invalid.minimumDistance = invalid.maximumDistance;
			if (EnemyBehaviorProfileValidator::Validate(invalid)) return Fail("Behavior validator accepted invalid HoldRange bounds") ? 0 : 1;
			invalid = strafe;
			invalid.strafeDirectionChangeInterval = 0.f;
			if (EnemyBehaviorProfileValidator::Validate(invalid)) return Fail("Behavior validator accepted zero strafe interval") ? 0 : 1;
			invalid = approach;
			invalid.movementMode = static_cast<EnemyMovementMode>(999);
			if (EnemyBehaviorProfileValidator::Validate(invalid)) return Fail("Behavior validator accepted an unknown movement mode") ? 0 : 1;
			invalid = approach;
			invalid.slotRules.front().maximumRange = invalid.targetSearchRange + 1.f;
			if (EnemyBehaviorProfileValidator::Validate(invalid)) return Fail("Behavior validator accepted out-of-search slot rule") ? 0 : 1;
			invalid = approach;
			invalid.slotRules.push_back(invalid.slotRules.front());
			if (EnemyBehaviorProfileValidator::Validate(invalid)) return Fail("Behavior validator accepted duplicate slot rules") ? 0 : 1;
			invalid = approach;
			invalid.slotRules.front().slot = sas::AbilitySlot::None;
			if (EnemyBehaviorProfileValidator::Validate(invalid)) return Fail("Behavior validator accepted a None slot rule") ? 0 : 1;
			if (!IsEnemyInputModeCompatible(sas::AbilityActivationPolicy::WhileHeld, EnemySlotInputMode::Hold) ||
				!IsEnemyInputModeCompatible(sas::AbilityActivationPolicy::OnPressed, EnemySlotInputMode::Pulse) ||
				IsEnemyInputModeCompatible(sas::AbilityActivationPolicy::WhileHeld, EnemySlotInputMode::Pulse) ||
				IsEnemyInputModeCompatible(sas::AbilityActivationPolicy::Toggle, EnemySlotInputMode::Hold))
				return Fail("Enemy slot input mode compatibility contract is incorrect") ? 0 : 1;

			const sf::Vector2f approachDirection = ResolveEnemyMovementDirection({ EnemyMovementMode::Approach, 500.f, 400.f, 200.f, 0.f, 1.f, { 1.f, 0.f } });
			const sf::Vector2f retreatDirection = ResolveEnemyMovementDirection({ EnemyMovementMode::Approach, 100.f, 400.f, 200.f, 0.f, 1.f, { 1.f, 0.f } });
			const sf::Vector2f holdDirection = ResolveEnemyMovementDirection({ EnemyMovementMode::HoldRange, 400.f, 0.f, 300.f, 600.f, 1.f, { 1.f, 0.f } });
			const sf::Vector2f strafeDirection = ResolveEnemyMovementDirection({ EnemyMovementMode::Strafe, 400.f, 0.f, 300.f, 600.f, 1.f, { 1.f, 0.f } });
			if (approachDirection.x <= 0.f || retreatDirection.x >= 0.f || GetVectorLength(holdDirection) > 0.001f || strafeDirection.y <= 0.f || GetVectorLength(strafeDirection) > 1.001f)
				return Fail("Behavior movement decisions violate their contract") ? 0 : 1;
			const sf::Vector2f zeroDistance = ResolveEnemyMovementDirection({ EnemyMovementMode::Strafe, 0.f, 0.f, 300.f, 600.f, 1.f, {} });
			const EnemySlotDecisionRule holdRule{ sas::AbilitySlot::PrimaryFire, EnemySlotInputMode::Hold, true, 0.f, 500.f, 0.9f, 0.25f, 0 };
			const EnemySlotDecisionRule pulseRule{ sas::AbilitySlot::Ability2, EnemySlotInputMode::Pulse, true, 100.f, 500.f, 0.9f, 0.25f, 0 };
			if (!std::isfinite(zeroDistance.x) || !std::isfinite(zeroDistance.y) ||
				!ShouldActivateEnemySlot(holdRule, { true, 500.f, 0.9f }) ||
				ShouldActivateEnemySlot(holdRule, { true, 501.f, 0.9f }) ||
				ShouldActivateEnemySlot(holdRule, { true, 500.f, 0.8f }) ||
				ShouldActivateEnemySlot(pulseRule, { true, 50.f, 1.f }) ||
				!ShouldActivateEnemySlot(pulseRule, { true, 100.f, 0.9f }) ||
				ShouldActivateEnemySlot(pulseRule, { false, 100.f, 1.f }))
				return Fail("Behavior fire or zero-distance decision violated its contract") ? 0 : 1;

			World targetWorld{ nullptr };
			const shared_ptr<TestCombatant> source = targetWorld.SpawnActor<TestCombatant>().lock();
			source->SetCollisionLayer(CollisionLayer::Enemy);
			source->SetCollisionMask(CollisionLayer::Player);
			EnemyBehaviorRuntime behaviorRuntime;
			if (!behaviorRuntime.Initialize(approach)) return Fail("Failed to initialize target cadence behavior runtime") ? 0 : 1;
			if (behaviorRuntime.Tick(*source, 0.f).HasTarget()) return Fail("Behavior runtime found a target in an empty world") ? 0 : 1;
			const shared_ptr<TestCombatant> target = targetWorld.SpawnActor<TestCombatant>().lock();
			target->SetCollisionLayer(CollisionLayer::Player);
			target->SetCollisionMask(CollisionLayer::Enemy);
			target->SetActorLocation({ 300.f, 0.f });
			targetWorld.TickInternal(0.f);
			if (behaviorRuntime.Tick(*source, 0.1f).HasTarget()) return Fail("Behavior runtime refreshed a missing target before its refresh interval") ? 0 : 1;
			if (!behaviorRuntime.Tick(*source, 0.1f).HasTarget()) return Fail("Behavior runtime did not refresh a missing target when its interval elapsed") ? 0 : 1;
		}

		std::cout << "[TEST Behavior Slot Commands]" << std::endl;
		{
			EnemyBehaviorProfile pulseProfile{ "EnemyBehavior.Test.Pulse", 1000.f, 0.2f, 0.f, 0.f, 0.f, EnemyMovementMode::Approach, 1.f, 1.f };
			pulseProfile.slotRules.push_back({ sas::AbilitySlot::Ability1, EnemySlotInputMode::Pulse, false, 0.f, 0.f, -1.f, 0.25f, 0 });
			EnemyBehaviorRuntime behaviorRuntime;
			if (!behaviorRuntime.Initialize(pulseProfile)) return Fail("Failed to initialize pulse behavior runtime") ? 0 : 1;
			World world{ nullptr };
			const shared_ptr<TestCombatant> source = world.SpawnActor<TestCombatant>().lock();
			source->SetCollisionLayer(CollisionLayer::Enemy);
			source->SetCollisionMask(CollisionLayer::Player);
			auto inputFor = [](const EnemyBehaviorIntent& intent, sas::AbilitySlot slot)
			{
				for (const EnemySlotCommand& command : intent.slotCommands)
					if (command.slot == slot) return command.inputHeld;
				return false;
			};
			if (!inputFor(behaviorRuntime.Tick(*source, 0.f), sas::AbilitySlot::Ability1) ||
				inputFor(behaviorRuntime.Tick(*source, 0.1f), sas::AbilitySlot::Ability1) ||
				inputFor(behaviorRuntime.Tick(*source, 0.1f), sas::AbilitySlot::Ability1) ||
				!inputFor(behaviorRuntime.Tick(*source, 0.1f), sas::AbilitySlot::Ability1) ||
				inputFor(behaviorRuntime.Tick(*source, 0.05f), sas::AbilitySlot::Ability1) ||
				!inputFor(behaviorRuntime.Tick(*source, 0.2f), sas::AbilitySlot::Ability1))
			{
				return Fail("Pulse slot command did not honor release and retry cadence") ? 0 : 1;
			}

			EnemyBehaviorProfile priorityProfile{ "EnemyBehavior.Test.Priority", 1000.f, 0.2f, 0.f, 0.f, 0.f, EnemyMovementMode::Approach, 1.f, 1.f };
			priorityProfile.slotRules = {
				{ sas::AbilitySlot::Ability1, EnemySlotInputMode::Hold, false, 0.f, 0.f, -1.f, 0.25f, 1 },
				{ sas::AbilitySlot::Ability2, EnemySlotInputMode::Hold, false, 0.f, 0.f, -1.f, 0.25f, 2 }
			};
			if (!behaviorRuntime.Initialize(priorityProfile)) return Fail("Failed to reinitialize priority behavior runtime") ? 0 : 1;
			const EnemyBehaviorIntent priorityIntent = behaviorRuntime.Tick(*source, -1.f);
			if (inputFor(priorityIntent, sas::AbilitySlot::Ability1) || !inputFor(priorityIntent, sas::AbilitySlot::Ability2))
				return Fail("Ability slot priority did not select the highest priority rule") ? 0 : 1;

			EnemyBehaviorProfile dynamicPriorityProfile{ "EnemyBehavior.Test.DynamicPriority", 1000.f, 0.01f, 0.f, 0.f, 0.f, EnemyMovementMode::Approach, 1.f, 1.f };
			dynamicPriorityProfile.slotRules = {
				{ sas::AbilitySlot::Ability1, EnemySlotInputMode::Hold, false, 0.f, 0.f, -1.f, 0.25f, 1 },
				{ sas::AbilitySlot::Ability2, EnemySlotInputMode::Hold, true, 0.f, 500.f, -1.f, 0.25f, 2 }
			};
			if (!behaviorRuntime.Initialize(dynamicPriorityProfile) ||
				!inputFor(behaviorRuntime.Tick(*source, 0.f), sas::AbilitySlot::Ability1))
				return Fail("Dynamic priority setup did not select the initially eligible hold rule") ? 0 : 1;
			const shared_ptr<TestCombatant> target = world.SpawnActor<TestCombatant>().lock();
			target->SetCollisionLayer(CollisionLayer::Player);
			target->SetCollisionMask(CollisionLayer::Enemy);
			target->SetActorLocation({ 100.f, 0.f });
			world.TickInternal(0.f);
			const EnemyBehaviorIntent promotedIntent = behaviorRuntime.Tick(*source, 0.01f);
			if (inputFor(promotedIntent, sas::AbilitySlot::Ability1) || !inputFor(promotedIntent, sas::AbilitySlot::Ability2))
				return Fail("A newly eligible higher-priority hold rule did not take over") ? 0 : 1;

			EnemyBehaviorProfile pulseFallbackProfile{ "EnemyBehavior.Test.PulseFallback", 1000.f, 0.2f, 0.f, 0.f, 0.f, EnemyMovementMode::Approach, 1.f, 1.f };
			pulseFallbackProfile.slotRules = {
				{ sas::AbilitySlot::Ability1, EnemySlotInputMode::Pulse, false, 0.f, 0.f, -1.f, 0.25f, 2 },
				{ sas::AbilitySlot::Ability2, EnemySlotInputMode::Hold, false, 0.f, 0.f, -1.f, 0.25f, 1 }
			};
			if (!behaviorRuntime.Initialize(pulseFallbackProfile)) return Fail("Failed to initialize pulse fallback behavior runtime") ? 0 : 1;
			const EnemyBehaviorIntent pulseStart = behaviorRuntime.Tick(*source, 0.f);
			const EnemyBehaviorIntent pulseRetry = behaviorRuntime.Tick(*source, 0.f);
			if (!inputFor(pulseStart, sas::AbilitySlot::Ability1) || inputFor(pulseRetry, sas::AbilitySlot::Ability1) ||
				!inputFor(pulseRetry, sas::AbilitySlot::Ability2))
				return Fail("A pulse rule in retry state incorrectly blocked a lower-priority eligible rule") ? 0 : 1;
			behaviorRuntime.Clear();
			const EnemyBehaviorIntent clearedIntent = behaviorRuntime.Tick(*source, 0.f);
			for (const EnemySlotCommand& command : clearedIntent.slotCommands)
				if (command.inputHeld) return Fail("Behavior Clear left a slot command held") ? 0 : 1;
		}

		std::filesystem::path weaponPath = "LightYearsGame/assets/content/data/weapons.json";
		if (!std::filesystem::exists(weaponPath))
		{
			weaponPath = "assets/content/data/weapons.json";
		}
		std::filesystem::path enemyProfilePath = "LightYearsGame/assets/content/data/enemy_combat_profiles.json";
		if (!std::filesystem::exists(enemyProfilePath))
		{
			enemyProfilePath = "assets/content/data/enemy_combat_profiles.json";
		}

		if (!content::WeaponContentCatalog::IsLoaded())
		{
			std::string loadError;
			if (!content::WeaponContentCatalog::LoadFromFile(weaponPath, &loadError))
			{
				return Fail(("Failed to load weapons.json: " + loadError).c_str()) ? 0 : 1;
			}
		}
		if (!content::EnemyCombatProfileCatalog::IsLoaded())
		{
			std::string loadError;
			if (!content::EnemyCombatProfileCatalog::LoadFromFile(enemyProfilePath, &loadError))
			{
				return Fail(("Failed to load enemy_combat_profiles.json: " + loadError).c_str()) ? 0 : 1;
			}
		}

		const EnemyCombatProfile* vanguardDef =
			content::EnemyCombatProfileCatalog::FindById("EnemyCombat.ApproachGunner.Basic");
		if (!vanguardDef)
		{
			return Fail("Failed to find shipped VanguardBasic profile in catalog") ? 0 : 1;
		}

		std::cout << "[TEST 1]" << std::endl;
		// 1. Enemy runtime AP and EP default to 0
		{
			World world{ nullptr };
			TestCombatant enemyCombatant{ &world };
			EnemyRuntime enemyRuntime{ enemyCombatant.GetCombatRuntime() };
			std::string initError;
			if (!enemyRuntime.Initialize(*vanguardDef, 1.f, &initError))
			{
				return Fail(("Failed to initialize EnemyRuntime with VanguardBasic: " + initError).c_str()) ? 0 : 1;
			}
			const auto& attrs = enemyCombatant.GetAbilitySystemComponent().GetAttributes();
			if (attrs.GetCurrentValue(OwnerAttributeIds::AttackPower) != 0.f)
			{
				return Fail("Enemy AttackPower is not 0 by default") ? 0 : 1;
			}
			if (attrs.GetCurrentValue(OwnerAttributeIds::EnergyPower) != 0.f)
			{
				return Fail("Enemy EnergyPower is not 0 by default") ? 0 : 1;
			}
			if (attrs.GetCurrentValue(OwnerAttributeIds::CriticalChance) != 0.f)
			{
				return Fail("Enemy CriticalChance is not 0 by default") ? 0 : 1;
			}
		}

		std::cout << "[TEST 2]" << std::endl;
		// 2. Damage formula: AuthoredBase * EncounterMultiplier * OutgoingMultiplier
		{
			World world{ nullptr };
			TestCombatant attacker{ &world };
			TestCombatant target{ &world };
			EnemyRuntime attackerRuntime{ attacker.GetCombatRuntime() };
			std::string err;
			if (!attackerRuntime.Initialize(*vanguardDef, 1.5f, &err)) return Fail(err.c_str()) ? 0 : 1;

			target.GetCombatRuntime().InitializeOwnerAttributes(100.f);
			ApplyCombatDamage(target, 10.f, &attacker);

			if (!NearlyEqual(target.lastReceivedContext.originalDamage, 15.f))
			{
				return Fail("Encounter multiplier was not applied correctly to authored damage") ? 0 : 1;
			}
		}

		std::cout << "[TEST 3]" << std::endl;
		// 3. AuthoredBase = 10, Encounter = 1.5, Outgoing = 0.8 => 12.0
		{
			World world{ nullptr };
			TestCombatant attacker{ &world };
			TestCombatant target{ &world };
			EnemyRuntime attackerRuntime{ attacker.GetCombatRuntime() };
			std::string err;
			if (!attackerRuntime.Initialize(*vanguardDef, 1.5f, &err)) return Fail(err.c_str()) ? 0 : 1;

			attacker.GetCombatRuntime().SetRuntimeModifier("Buff.Test", CombatRuntimeModifier{ 0.8f });
			target.GetCombatRuntime().InitializeOwnerAttributes(100.f);
			ApplyCombatDamage(target, 10.f, &attacker);

			if (!NearlyEqual(target.lastReceivedContext.originalDamage, 12.0f))
			{
				return Fail("Expected resolvedDamage to be 12.0f (10 * 1.5 * 0.8)") ? 0 : 1;
			}
		}

		std::cout << "[TEST 4]" << std::endl;
		// 4. Player armor mitigation applies correctly to enemy damage
		{
			World world{ nullptr };
			TestCombatant attacker{ &world };
			TestCombatant target{ &world };
			EnemyRuntime attackerRuntime{ attacker.GetCombatRuntime() };
			std::string err;
			if (!attackerRuntime.Initialize(*vanguardDef, 1.0f, &err)) return Fail(err.c_str()) ? 0 : 1;

			target.GetCombatRuntime().InitializeOwnerAttributes(100.f);
			target.GetAbilitySystemComponent().GetAttributes().RegisterAttribute(OwnerAttributeIds::Armor, 100.f);

			ApplyCombatDamage(target, 100.f, &attacker);

			if (!NearlyEqual(target.lastProcessedContext.remainingDamage, 50.0f))
			{
				return Fail("Armor mitigation did not apply 50% reduction for 100 armor") ? 0 : 1;
			}
		}

		std::cout << "[TEST 5]" << std::endl;
		// 5. Outgoing multiplier of 0 results in 0 resolved damage
		{
			World world{ nullptr };
			TestCombatant attacker{ &world };
			TestCombatant target{ &world };
			EnemyRuntime attackerRuntime{ attacker.GetCombatRuntime() };
			std::string err;
			if (!attackerRuntime.Initialize(*vanguardDef, 1.0f, &err)) return Fail(err.c_str()) ? 0 : 1;

			attacker.GetCombatRuntime().SetRuntimeModifier("Debuff.Disarmed", CombatRuntimeModifier{ 0.0f });
			target.GetCombatRuntime().InitializeOwnerAttributes(100.f);
			ApplyCombatDamage(target, 50.f, &attacker);

			if (target.lastReceivedContext.originalDamage != 0.0f)
			{
				return Fail("Target took damage when outgoing damage multiplier was 0") ? 0 : 1;
			}
		}

		std::cout << "[TEST 6]" << std::endl;
		// 6. Temporary damage protection blocks enemy damage
		{
			World world{ nullptr };
			TestCombatant attacker{ &world };
			TestCombatant target{ &world };
			EnemyRuntime attackerRuntime{ attacker.GetCombatRuntime() };
			std::string err;
			if (!attackerRuntime.Initialize(*vanguardDef, 1.0f, &err)) return Fail(err.c_str()) ? 0 : 1;

			target.GetCombatRuntime().InitializeOwnerAttributes(100.f);
			target.GetCombatRuntime().SetDamageProtection("Shield.Immunity", true, false);

			ApplyCombatDamage(target, 25.f, &attacker);

			if (target.lastReceivedContext.originalDamage != 0.0f)
			{
				return Fail("Damage protection did not block incoming enemy damage") ? 0 : 1;
			}
		}

		std::cout << "[TEST 7]" << std::endl;
		// 7. Profile swapping: Profile A to Profile B cleanly swaps handles
		{
			World world{ nullptr };
			TestCombatant combatant{ &world };
			EnemyRuntime enemyRuntime{ combatant.GetCombatRuntime() };

			EnemyCombatProfile profileA;
			profileA.profileId = "EnemyCombat.Test.ProfileA";
			profileA.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			profileA.powerScalingPolicy = EnemyPowerScalingPolicy::Allowed;

			std::string initError;
			if (!enemyRuntime.Initialize(profileA, 1.0f, &initError))
			{
				return Fail(("Failed to initialize Profile A: " + initError).c_str()) ? 0 : 1;
			}

			const sas::AbilityHandle primaryHandleA = enemyRuntime.GetOwnedLoadoutHandles().front();
			if (!primaryHandleA.IsValid())
			{
				return Fail("Profile A did not produce a valid primary weapon handle") ? 0 : 1;
			}

			EnemyCombatProfile profileB;
			profileB.profileId = "EnemyCombat.Test.ProfileB";
			profileB.weapons.push_back({ "Weapon.Projectile.EnemyTwinBladeScatter.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			profileB.powerScalingPolicy = EnemyPowerScalingPolicy::Allowed;

			if (!enemyRuntime.Initialize(profileB, 2.0f, &initError))
			{
				return Fail(("Failed to swap to Profile B: " + initError).c_str()) ? 0 : 1;
			}

			const sas::AbilityHandle primaryHandleB = enemyRuntime.GetOwnedLoadoutHandles().front();
			if (!primaryHandleB.IsValid() || primaryHandleB == primaryHandleA)
			{
				return Fail("Profile B did not receive a new distinct primary weapon handle") ? 0 : 1;
			}
			if (combatant.GetAbilitySystemComponent().GetAbility(primaryHandleA) != nullptr)
			{
				return Fail("Old primary weapon handle was not removed from ASC after swap") ? 0 : 1;
			}
			if (enemyRuntime.GetEncounterDamageMultiplier() != 2.0f)
			{
				return Fail("Encounter damage multiplier was not updated on profile swap") ? 0 : 1;
			}
		}

		std::cout << "[TEST 8]" << std::endl;
		// 8. Preflight slot collision validation preserves old profile
		{
			World world{ nullptr };
			TestCombatant combatant{ &world };
			EnemyRuntime enemyRuntime{ combatant.GetCombatRuntime() };

			EnemyCombatProfile profileInit;
			profileInit.profileId = "EnemyCombat.Test.CollidingInit";
			profileInit.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			profileInit.powerScalingPolicy = EnemyPowerScalingPolicy::Allowed;
			std::string initError;
			if (!enemyRuntime.Initialize(profileInit, 1.f, &initError))
			{
				return Fail(("Initial profile setup failed: " + initError).c_str()) ? 0 : 1;
			}

			EnemyCombatProfile profileColliding;
			profileColliding.profileId = "EnemyCombat.Test.Colliding";
			profileColliding.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			profileColliding.powerScalingPolicy = EnemyPowerScalingPolicy::Allowed;
			profileColliding.abilities.push_back({ "Ability.Movement.Dash.Basic", sas::AbilitySlot::Ability1, 1 });
			profileColliding.abilities.push_back({ "Ability.Defense.Shield.Basic", sas::AbilitySlot::Ability1, 1 });

			std::string replaceError;
			if (enemyRuntime.Initialize(profileColliding, 1.0f, &replaceError))
			{
				return Fail("Profile with duplicate slot was incorrectly accepted by EnemyRuntime") ? 0 : 1;
			}
			if (!enemyRuntime.GetCurrentProfile() || enemyRuntime.GetCurrentProfile()->profileId != "EnemyCombat.Test.CollidingInit")
			{
				return Fail("Previous profile was not preserved after preflight rejection") ? 0 : 1;
			}
		}

		std::cout << "[TEST 9]" << std::endl;
		// 9. Multi-ability binding and level configuration
		{
			World world{ nullptr };
			TestCombatant combatant{ &world };
			EnemyRuntime enemyRuntime{ combatant.GetCombatRuntime() };

			EnemyCombatProfile multiProfile;
			multiProfile.profileId = "EnemyCombat.Test.MultiAbility";
			multiProfile.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			multiProfile.powerScalingPolicy = EnemyPowerScalingPolicy::Allowed;
			multiProfile.abilities.push_back({ "Ability.Movement.Dash.Basic", sas::AbilitySlot::Ability1, 1 });
			multiProfile.abilities.push_back({ "Ability.Defense.Shield.Basic", sas::AbilitySlot::Ability2, 1 });

			std::string initError;
			if (!enemyRuntime.Initialize(multiProfile, 1.0f, &initError))
			{
				return Fail(("Failed to initialize multi-ability profile: " + initError).c_str()) ? 0 : 1;
			}

			const GameAbility* dash = combatant.GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability1);
			if (!dash || dash->GetLevel() != 1)
			{
				return Fail("Ability was not assigned the requested level (expected level 1)") ? 0 : 1;
			}
		}

		std::cout << "[TEST 9A]" << std::endl;
		// 9A. Ability-only profiles grant active abilities without a weapon.
		{
			World world{ nullptr };
			TestCombatant combatant{ &world };
			EnemyRuntime enemyRuntime{ combatant.GetCombatRuntime() };
			EnemyCombatProfile abilityOnly{ "EnemyCombat.Test.AbilityOnly", {}, {}, EnemyPowerScalingPolicy::Allowed, false };
			abilityOnly.abilities.push_back({ "Ability.Movement.Dash.Basic", sas::AbilitySlot::Ability1, 1 });
			std::string initError;
			if (!enemyRuntime.Initialize(abilityOnly, 1.f, &initError)) return Fail(initError.c_str()) ? 0 : 1;
			if (enemyRuntime.GetOwnedLoadoutHandles().size() != 1 ||
				combatant.GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::PrimaryFire) != nullptr)
			{
				return Fail("Ability-only profile did not produce exactly one non-weapon loadout") ? 0 : 1;
			}
			const GameAbility* dash = combatant.GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability1);
			if (!dash || dash->GetDefinition().abilityId != "Ability.Movement.Dash.Basic")
				return Fail("Ability-only profile did not bind its ability to Ability1") ? 0 : 1;
		}

		std::cout << "[TEST 9B]" << std::endl;
		// 9B. Two weapons and two active abilities share one generic loadout.
		{
			World world{ nullptr };
			TestCombatant combatant{ &world };
			EnemyRuntime enemyRuntime{ combatant.GetCombatRuntime() };
			EnemyCombatProfile mixedProfile{ "EnemyCombat.Test.MixedLoadout", {}, {}, EnemyPowerScalingPolicy::Allowed, false };
			mixedProfile.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			mixedProfile.weapons.push_back({ "Weapon.Projectile.EnemyTwinBladeScatter.Basic", sas::AbilitySlot::Ability2, 1 });
			mixedProfile.abilities.push_back({ "Ability.Movement.Dash.Basic", sas::AbilitySlot::Ability1, 1 });
			mixedProfile.abilities.push_back({ "Ability.Defense.Shield.Basic", sas::AbilitySlot::Ability3, 1 });

			std::string initError;
			if (!enemyRuntime.Initialize(mixedProfile, 1.f, &initError)) return Fail(initError.c_str()) ? 0 : 1;
			const auto& handles = enemyRuntime.GetOwnedLoadoutHandles();
			if (handles.size() != 4 || handles[0] == handles[1]) return Fail("Mixed loadout did not grant four distinct handles") ? 0 : 1;
			const GameAbility* firstWeapon = combatant.GetAbilitySystemComponent().GetAbility(handles[0]);
			const GameAbility* secondWeapon = combatant.GetAbilitySystemComponent().GetAbility(handles[1]);
			if (!firstWeapon || !secondWeapon || firstWeapon->GetRuntimeSlot() != sas::AbilitySlot::PrimaryFire ||
				secondWeapon->GetRuntimeSlot() != sas::AbilitySlot::Ability2 ||
				combatant.GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability1) == nullptr ||
				combatant.GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability3) == nullptr)
			{
				return Fail("Mixed loadout did not bind weapons and abilities to their requested slots") ? 0 : 1;
			}

			GameAbility* mutableFirstWeapon = combatant.GetAbilitySystemComponent().GetAbility(handles[0]);
			GameAbility* mutableSecondWeapon = combatant.GetAbilitySystemComponent().GetAbility(handles[1]);
			mutableFirstWeapon->GetPrimaryWeaponRuntime().fireIntervalRemaining = 3.5f;
			mutableSecondWeapon->GetPrimaryWeaponRuntime().fireIntervalRemaining = 7.25f;
			mutableFirstWeapon->GetPrimaryWeaponRuntime().fireIntervalRemaining = 1.25f;
			if (!NearlyEqual(mutableFirstWeapon->GetPrimaryWeaponRuntime().fireIntervalRemaining, 1.25f) ||
				!NearlyEqual(mutableSecondWeapon->GetPrimaryWeaponRuntime().fireIntervalRemaining, 7.25f))
			{
				return Fail("Two weapon runtime states are not independent") ? 0 : 1;
			}

			mutableSecondWeapon->GetPrimaryWeaponRuntime().fireIntervalRemaining = 0.f;
			const uint64_t primaryFireCount = mutableFirstWeapon->GetPrimaryWeaponRuntime().successfulFireCount;
			const uint64_t secondaryFireCount = mutableSecondWeapon->GetPrimaryWeaponRuntime().successfulFireCount;
			const size_t projectileCount = world.GetActorsByType<PrimaryWeaponProjectileActor>().size();
			combatant.GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability2, true);
			combatant.GetAbilitySystemComponent().Tick(0.01f);
			world.TickInternal(0.01f);
			combatant.GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability2, false);
			combatant.GetAbilitySystemComponent().Tick(0.01f);
			world.TickInternal(0.01f);
			if (world.GetActorsByType<PrimaryWeaponProjectileActor>().size() <= projectileCount ||
				mutableSecondWeapon->GetPrimaryWeaponRuntime().successfulFireCount <= secondaryFireCount ||
				mutableFirstWeapon->GetPrimaryWeaponRuntime().successfulFireCount != primaryFireCount ||
				mutableFirstWeapon->GetPrimaryWeaponRuntime().isFiring)
			{
				return Fail("Ability2 input did not execute only the secondary weapon") ? 0 : 1;
			}
		}

		std::cout << "[TEST 9C]" << std::endl;
		// 9C. Empty loadouts are valid only when contact-only combat is enabled.
		{
			World world{ nullptr };
			TestCombatant combatant{ &world };
			EnemyRuntime enemyRuntime{ combatant.GetCombatRuntime() };
			EnemyCombatProfile contactOnly{ "EnemyCombat.Test.ContactOnly", {}, {}, EnemyPowerScalingPolicy::Disabled, true };
			std::string initError;
			if (!enemyRuntime.Initialize(contactOnly, 1.f, &initError) || !enemyRuntime.IsReady() ||
				!enemyRuntime.GetOwnedLoadoutHandles().empty())
			{
				return Fail("Contact-only empty loadout was not accepted as an empty runtime") ? 0 : 1;
			}
			EnemyCombatProfile emptyCombat{ "EnemyCombat.Test.EmptyCombat", {}, {}, EnemyPowerScalingPolicy::Disabled, false };
			if (content::EnemyCombatProfileCatalog::ValidateProfile(emptyCombat))
				return Fail("Empty non-contact-only profile was incorrectly accepted") ? 0 : 1;
		}

		std::cout << "[TEST 9D]" << std::endl;
		// 9D. Missing IDs, invalid levels, and cross-family slot collisions fail preflight.
		{
			EnemyCombatProfile invalid{ "EnemyCombat.Test.InvalidBindings", {}, {}, EnemyPowerScalingPolicy::Allowed, false };
			invalid.weapons.push_back({ "Weapon.Projectile.DoesNotExist.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			if (content::EnemyCombatProfileCatalog::ValidateProfile(invalid)) return Fail("Missing weapon ID passed enemy loadout validation") ? 0 : 1;
			invalid.weapons.front() = { "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 999 };
			if (content::EnemyCombatProfileCatalog::ValidateProfile(invalid)) return Fail("Invalid weapon level passed enemy loadout validation") ? 0 : 1;
			invalid.weapons.front() = { "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::Ability1, 1 };
			invalid.abilities.push_back({ "Ability.Movement.Dash.Basic", sas::AbilitySlot::Ability1, 1 });
			if (content::EnemyCombatProfileCatalog::ValidateProfile(invalid)) return Fail("Weapon and ability slot collision passed enemy loadout validation") ? 0 : 1;
			EnemyCombatProfile duplicateWeapons{ "EnemyCombat.Test.DuplicateWeapons", {}, {}, EnemyPowerScalingPolicy::Allowed, false };
			duplicateWeapons.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			duplicateWeapons.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::Ability2, 1 });
			if (content::EnemyCombatProfileCatalog::ValidateProfile(duplicateWeapons)) return Fail("Duplicate weapon ID passed enemy loadout validation") ? 0 : 1;
		}

		std::cout << "[TEST 10]" << std::endl;
		// 10. Representative enemy fire spawns actual PrimaryWeaponProjectileActor
		{
			World world{ nullptr };
			std::shared_ptr<EnemyActor> shooter = content::SpawnEnemy(
				world,
				EnemyIds::ApproachGunnerBasic,
				{ 300.f, 300.f }
			).lock();
			world.TickInternal(0.01f);
			if (!shooter || shooter->GetIsPendingDestroy())
				return Fail("Generic enemy actor did not initialize") ? 0 : 1;

			const size_t beforeProjectiles = world.GetActorsByType<PrimaryWeaponProjectileActor>().size();
			shooter->GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
			shooter->GetAbilitySystemComponent().Tick(0.01f);
			world.TickInternal(0.01f);
			shooter->GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
			shooter->GetAbilitySystemComponent().Tick(0.01f);
			world.TickInternal(0.01f);
			const size_t afterProjectiles = world.GetActorsByType<PrimaryWeaponProjectileActor>().size();

			if (afterProjectiles <= beforeProjectiles)
			{
				return Fail("Representative enemy fire did not spawn any PrimaryWeaponProjectileActor") ? 0 : 1;
			}
		}

		std::cout << "[TEST Encounter Waves]" << std::endl;
		{
			World world{ nullptr };
			List<std::string> spawnedIds;
			EncounterWaveRuntime runtime;
			if (!SameSnapshot(runtime.BuildSnapshot(), EncounterWaveSnapshot{}))
				return Fail("Encounter snapshot did not start in a neutral idle state") ? 0 : 1;
			const List<EnemyWaveDefinition> definitions{
				{ { { sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 2, 0.5f } }, 0.25f },
				{ { { sas::ContentId{ EnemyIds::StrafeSkirmisherBasic }, 1, 0.f } }, 0.25f }
			};
			if (!runtime.Start(definitions,
				[&](const sas::ContentId& enemyId, size_t spawnIndex, const EnemySpawnContext& context)
				{
					spawnedIds.push_back(enemyId.ToString());
					return content::SpawnEnemy(world, enemyId.ToString(), { 600.f + 100.f * static_cast<float>(spawnIndex), 600.f }, 1.f, context);
				}))
				return Fail("Encounter wave runtime failed to start valid definitions") ? 0 : 1;
			const EncounterWaveSnapshot waveStartSnapshot = runtime.BuildSnapshot();
			if (waveStartSnapshot.state != EncounterWaveState::Spawning ||
				waveStartSnapshot.currentWaveNumber != 1 ||
				waveStartSnapshot.totalWaveCount != 2 ||
				waveStartSnapshot.plannedEnemyCount != 2 ||
				waveStartSnapshot.spawnedEnemyCount != 0 ||
				waveStartSnapshot.aliveEnemyCount != 0 ||
				waveStartSnapshot.remainingSpawnCount != 2 ||
				waveStartSnapshot.interWaveRemainingTime != 0.f ||
				waveStartSnapshot.enemyLevel != 1)
				return Fail("Encounter snapshot did not report a complete first wave contract") ? 0 : 1;
			runtime.Tick(0.f);
			if (spawnedIds.size() != 1 || spawnedIds.front() != EnemyIds::ApproachGunnerBasic)
				return Fail("Encounter wave did not spawn its first entry immediately") ? 0 : 1;
			const EncounterWaveSnapshot firstSpawnSnapshot = runtime.BuildSnapshot();
			if (firstSpawnSnapshot.spawnedEnemyCount != 1 ||
				firstSpawnSnapshot.remainingSpawnCount != 1 ||
				firstSpawnSnapshot.aliveEnemyCount != 1 ||
				firstSpawnSnapshot.spawnedEnemyCount + firstSpawnSnapshot.remainingSpawnCount !=
					firstSpawnSnapshot.plannedEnemyCount)
				return Fail("Encounter snapshot did not track an in-flight spawn interval") ? 0 : 1;
			runtime.Tick(0.49f);
			if (spawnedIds.size() != 1) return Fail("Encounter wave violated its spawn interval") ? 0 : 1;
			if (runtime.BuildSnapshot().spawnedEnemyCount != 1)
				return Fail("Encounter snapshot advanced before its spawn interval elapsed") ? 0 : 1;
			runtime.Tick(0.01f);
			if (spawnedIds.size() != 2 || runtime.GetState() != EncounterWaveState::WaitingForClear)
				return Fail("Encounter wave did not finish its ordered first entry") ? 0 : 1;
			const EncounterWaveSnapshot clearSnapshot = runtime.BuildSnapshot();
			if (clearSnapshot.spawnedEnemyCount != 2 ||
				clearSnapshot.remainingSpawnCount != 0 ||
				clearSnapshot.aliveEnemyCount != 2 ||
				clearSnapshot.plannedEnemyCount != 2)
				return Fail("Encounter snapshot did not report a fully spawned wave") ? 0 : 1;

			const shared_ptr<TestCombatant> foreignActor = world.SpawnActor<TestCombatant>().lock();
			for (const weak_ptr<EnemyActor>& enemy : runtime.GetOwnedEnemies())
				if (const shared_ptr<EnemyActor> actor = enemy.lock()) actor->Destroy();
			world.TickInternal(0.f);
			runtime.Tick(0.f);
			if (runtime.GetState() != EncounterWaveState::InterWaveDelay)
				return Fail("Encounter wave did not wait after its owned enemies died") ? 0 : 1;
			const EncounterWaveSnapshot interWaveSnapshot = runtime.BuildSnapshot();
			if (interWaveSnapshot.aliveEnemyCount != 0 ||
				interWaveSnapshot.currentWaveNumber != 1 ||
				interWaveSnapshot.remainingSpawnCount != 0 ||
				!NearlyEqual(interWaveSnapshot.interWaveRemainingTime, 0.25f))
				return Fail("Encounter snapshot did not report the inter-wave delay window") ? 0 : 1;
			runtime.Tick(0.25f);
			if (spawnedIds.size() != 3 || spawnedIds.back() != EnemyIds::StrafeSkirmisherBasic)
				return Fail("Encounter wave did not advance after the configured delay") ? 0 : 1;
			const EncounterWaveSnapshot secondWaveSnapshot = runtime.BuildSnapshot();
			if (secondWaveSnapshot.currentWaveNumber != 2 ||
				secondWaveSnapshot.spawnedEnemyCount != 1 ||
				secondWaveSnapshot.remainingSpawnCount != 0 ||
				secondWaveSnapshot.aliveEnemyCount != 1 ||
				secondWaveSnapshot.totalWaveCount != 2 ||
				secondWaveSnapshot.interWaveRemainingTime != 0.f)
				return Fail("Encounter snapshot did not advance to the second wave") ? 0 : 1;
			if (!foreignActor || foreignActor->GetIsPendingDestroy())
				return Fail("Encounter runtime incorrectly owned an external actor") ? 0 : 1;
			for (const weak_ptr<EnemyActor>& enemy : runtime.GetOwnedEnemies())
				if (const shared_ptr<EnemyActor> actor = enemy.lock()) actor->Destroy();
			world.TickInternal(0.f);
			runtime.Tick(0.f);
			if (!runtime.IsCompleted()) return Fail("Encounter runtime delayed completion after its final wave") ? 0 : 1;
			const EncounterWaveSnapshot completedSnapshot = runtime.BuildSnapshot();
			if (completedSnapshot.state != EncounterWaveState::Completed ||
				completedSnapshot.currentWaveNumber != completedSnapshot.totalWaveCount ||
				completedSnapshot.aliveEnemyCount != 0 ||
				completedSnapshot.remainingSpawnCount != 0)
				return Fail("Encounter snapshot did not report final completion counters") ? 0 : 1;
			runtime.Reset();
			if (runtime.GetState() != EncounterWaveState::Idle || !runtime.GetOwnedEnemies().empty())
				return Fail("Encounter runtime reset did not clear deterministic state") ? 0 : 1;
			if (!SameSnapshot(runtime.BuildSnapshot(), EncounterWaveSnapshot{}))
				return Fail("Encounter snapshot survived a runtime reset") ? 0 : 1;

			EncounterWaveRuntime failedRuntime;
			if (!failedRuntime.Start({ { { { sas::ContentId{ "Enemy.Unknown" }, 1, 0.f } }, 0.f } },
				[](const sas::ContentId&, size_t, const EnemySpawnContext&) { return weak_ptr<EnemyActor>{}; }))
				return Fail("Encounter runtime rejected a structurally valid failure fixture") ? 0 : 1;
			failedRuntime.Tick(0.f);
			if (!failedRuntime.HasFailed() || failedRuntime.GetFailureReason().empty())
				return Fail("Encounter runtime did not retain a failed spawn reason") ? 0 : 1;
			const EncounterWaveSnapshot failedSnapshot = failedRuntime.BuildSnapshot();
			if (failedSnapshot.state != EncounterWaveState::Failed ||
				failedSnapshot.currentWaveNumber != 1 ||
				failedSnapshot.plannedEnemyCount != 1 ||
				failedSnapshot.spawnedEnemyCount != 0 ||
				failedSnapshot.remainingSpawnCount != 1 ||
				failedSnapshot.aliveEnemyCount != 0)
				return Fail("Encounter snapshot was not readable after a spawn failure") ? 0 : 1;

			EncounterWaveRuntime largeDeltaRuntime;
			List<std::string> largeDeltaIds;
			if (!largeDeltaRuntime.Start({
				{ { { sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 3, 0.25f } }, 0.f }
			}, [&](const sas::ContentId& enemyId, size_t spawnIndex, const EnemySpawnContext& context)
			{
				largeDeltaIds.push_back(enemyId.ToString());
				return content::SpawnEnemy(world, enemyId.ToString(), { 900.f + 100.f * static_cast<float>(spawnIndex), 900.f }, 1.f, context);
			}))
				return Fail("Encounter runtime could not start the large-delta fixture") ? 0 : 1;
			largeDeltaRuntime.Tick(0.5f);
			if (largeDeltaIds.size() != 3 || largeDeltaRuntime.GetState() != EncounterWaveState::WaitingForClear)
				return Fail("Encounter runtime discarded elapsed spawn time during a large tick") ? 0 : 1;
			const EncounterWaveSnapshot largeDeltaSnapshot = largeDeltaRuntime.BuildSnapshot();
			if (largeDeltaSnapshot.plannedEnemyCount != 3 ||
				largeDeltaSnapshot.spawnedEnemyCount != static_cast<int>(largeDeltaIds.size()) ||
				largeDeltaSnapshot.remainingSpawnCount != 0 ||
				largeDeltaSnapshot.aliveEnemyCount != 3)
				return Fail("Encounter snapshot diverged from runtime state after a large tick") ? 0 : 1;

			EncounterWaveRuntime integrationRuntime;
			List<std::string> integrationIds;
			if (!integrationRuntime.Start({
				{ {
					{ sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 1, 0.f },
					{ sas::ContentId{ EnemyIds::StrafeSkirmisherBasic }, 1, 0.f },
					{ sas::ContentId{ EnemyIds::RangeKeeperBasic }, 1, 0.f }
				}, 0.f }
			}, [&](const sas::ContentId& enemyId, size_t spawnIndex, const EnemySpawnContext& context)
			{
				integrationIds.push_back(enemyId.ToString());
				return content::SpawnEnemy(world, enemyId.ToString(), { 1100.f + 100.f * static_cast<float>(spawnIndex), 1100.f }, 1.f, context);
			}))
				return Fail("Encounter runtime could not start the factory integration fixture") ? 0 : 1;
			integrationRuntime.Tick(0.f);
			if (integrationIds != List<std::string>{
				EnemyIds::ApproachGunnerBasic,
				EnemyIds::StrafeSkirmisherBasic,
				EnemyIds::RangeKeeperBasic
			}) return Fail("Encounter runtime did not spawn all shipped enemy types through the factory") ? 0 : 1;

			EncounterWaveRuntime progressionRuntime;
			List<EnemySpawnContext> progressionContexts;
			List<List<std::string>> progressionWaves;
			if (!progressionRuntime.Start({
				{ {
					{ sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 1, 0.f },
					{ sas::ContentId{ EnemyIds::StrafeSkirmisherBasic }, 1, 0.f },
					{ sas::ContentId{ EnemyIds::RangeKeeperBasic }, 1, 0.f }
				}, 0.f },
				{ { { sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 1, 0.f }, { sas::ContentId{ EnemyIds::StrafeSkirmisherBasic }, 1, 0.f }, { sas::ContentId{ EnemyIds::RangeKeeperBasic }, 1, 0.f } }, 0.f },
				{ { { sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 1, 0.f }, { sas::ContentId{ EnemyIds::StrafeSkirmisherBasic }, 1, 0.f }, { sas::ContentId{ EnemyIds::RangeKeeperBasic }, 1, 0.f } }, 0.f },
				{ { { sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 1, 0.f }, { sas::ContentId{ EnemyIds::StrafeSkirmisherBasic }, 1, 0.f }, { sas::ContentId{ EnemyIds::RangeKeeperBasic }, 1, 0.f } }, 0.f },
				{ { { sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 1, 0.f }, { sas::ContentId{ EnemyIds::StrafeSkirmisherBasic }, 1, 0.f }, { sas::ContentId{ EnemyIds::RangeKeeperBasic }, 1, 0.f } }, 0.f }
			}, [&](const sas::ContentId& enemyId, size_t spawnIndex, const EnemySpawnContext& context)
			{
				progressionContexts.push_back(context);
				progressionWaves.back().push_back(enemyId.ToString());
				const weak_ptr<EnemyActor> spawned = content::SpawnEnemy(world, enemyId.ToString(), { 1500.f + 100.f * static_cast<float>(spawnIndex), 1500.f }, 1.f, context);
				if (const shared_ptr<EnemyActor> actor = spawned.lock(); !actor || actor->GetEnemyLevel() != context.level) return weak_ptr<EnemyActor>{};
				return spawned;
			}, { 2, 2, 2, 3, 1337u }))
				return Fail("Encounter runtime could not start the progression fixture") ? 0 : 1;
			const List<size_t> expectedCounts{ 3, 3, 4, 4, 5 };
			const List<int> expectedLevels{ 1, 1, 2, 2, 3 };
			progressionWaves.push_back({});
			progressionRuntime.Tick(0.f);
			for (int wave = 0; wave < 5; ++wave)
			{
				const EncounterWaveSnapshot progressionSnapshot = progressionRuntime.BuildSnapshot();
				if (progressionSnapshot.currentWaveNumber != static_cast<size_t>(wave) + 1 ||
					progressionSnapshot.enemyLevel != expectedLevels[wave] ||
					progressionSnapshot.plannedEnemyCount != static_cast<int>(expectedCounts[wave]) ||
					progressionSnapshot.spawnedEnemyCount != progressionSnapshot.plannedEnemyCount ||
					progressionSnapshot.remainingSpawnCount != 0)
					return Fail("Encounter snapshot did not mirror authored progression per wave") ? 0 : 1;
				for (const weak_ptr<EnemyActor>& enemy : progressionRuntime.GetOwnedEnemies())
					if (const shared_ptr<EnemyActor> actor = enemy.lock()) actor->Destroy();
				if (wave + 1 < 5) progressionWaves.push_back({});
				progressionRuntime.Tick(0.f);
			}
			if (progressionWaves.size() != expectedCounts.size()) return Fail("Encounter progression did not visit every authored wave") ? 0 : 1;
			for (size_t index = 0; index < expectedCounts.size(); ++index)
				if (progressionWaves[index].size() != expectedCounts[index]) return Fail("Encounter progression count or cap is incorrect") ? 0 : 1;
			if (progressionWaves[2] != List<std::string>{ EnemyIds::ApproachGunnerBasic, EnemyIds::ApproachGunnerBasic, EnemyIds::StrafeSkirmisherBasic, EnemyIds::RangeKeeperBasic } ||
				progressionWaves[4] != List<std::string>{ EnemyIds::ApproachGunnerBasic, EnemyIds::ApproachGunnerBasic, EnemyIds::StrafeSkirmisherBasic, EnemyIds::StrafeSkirmisherBasic, EnemyIds::RangeKeeperBasic })
				return Fail("Encounter progression did not distribute additional enemies round-robin") ? 0 : 1;
			size_t contextOffset = 0;
			for (size_t wave = 0; wave < expectedCounts.size(); ++wave)
				for (size_t spawn = 0; spawn < expectedCounts[wave]; ++spawn)
					if (progressionContexts[contextOffset++].level != expectedLevels[wave]) return Fail("Encounter progression level or cap is incorrect") ? 0 : 1;
			if (progressionContexts[0].variationSeed == progressionContexts[1].variationSeed)
				return Fail("Encounter progression did not derive unique spawn seeds") ? 0 : 1;
			EncounterWaveRuntime sameSeedRuntime;
			List<EnemySpawnContext> sameSeedContexts;
			if (!sameSeedRuntime.Start({
				{ {
					{ sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 1, 0.f },
					{ sas::ContentId{ EnemyIds::StrafeSkirmisherBasic }, 1, 0.f },
					{ sas::ContentId{ EnemyIds::RangeKeeperBasic }, 1, 0.f }
				}, 0.f }
			}, [&](const sas::ContentId& enemyId, size_t spawnIndex, const EnemySpawnContext& context)
			{
				sameSeedContexts.push_back(context);
				return content::SpawnEnemy(world, enemyId.ToString(), { 2500.f + 100.f * static_cast<float>(spawnIndex), 1500.f }, 1.f, context);
			}, { 2, 2, 2, 3, 1337u }))
				return Fail("Encounter runtime could not start the same-seed fixture") ? 0 : 1;
			sameSeedRuntime.Tick(0.f);
			if (sameSeedContexts.size() != 3 || !std::equal(sameSeedContexts.begin(), sameSeedContexts.end(), progressionContexts.begin(), [](const EnemySpawnContext& left, const EnemySpawnContext& right)
			{
				return left.level == right.level && left.variationSeed == right.variationSeed;
			})) return Fail("Encounter progression seed resolution is not deterministic") ? 0 : 1;
			if (!content::SpawnEnemy(world, EnemyIds::ApproachGunnerBasic, { 0.f, 0.f }, 1.f, { -1, 123u }).expired())
				return Fail("Enemy factory accepted an invalid progression level") ? 0 : 1;
		}

		std::cout << "[TEST Encounter Endless Progression Curve]" << std::endl;
		{
			World world{ nullptr };
			EncounterWaveRuntime endlessRuntime;
			if (!endlessRuntime.Start({
				{ { { sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 2, 0.f } }, 0.f },
				{ { { sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 1, 0.f },
					{ sas::ContentId{ EnemyIds::StrafeSkirmisherBasic }, 1, 0.f } }, 0.f },
				{ { { sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 1, 0.f },
					{ sas::ContentId{ EnemyIds::StrafeSkirmisherBasic }, 1, 0.f },
					{ sas::ContentId{ EnemyIds::RangeKeeperBasic }, 1, 0.f } }, 0.f }
			}, [&](const sas::ContentId& enemyId, size_t spawnIndex, const EnemySpawnContext& context)
			{
				return content::SpawnEnemy(world, enemyId.ToString(), { 800.f + 20.f * static_cast<float>(spawnIndex), 800.f }, 1.f, context);
			}, { 1, 3, 0, 15, 1337u }, EncounterSequenceMode::EndlessCycle))
				return Fail("Endless encounter runtime failed to start") ? 0 : 1;

			// Open wave 1 before any assertion; every fixture template uses a zero
			// spawn interval so one tick spawns the whole wave.
			endlessRuntime.Tick(0.f);

			// Clearing the owned enemies advances exactly one wave because every fixture
			// template uses a zero next-wave delay.
			size_t currentWave = 1;
			const auto advanceTo = [&](size_t targetWave)
			{
				while (currentWave < targetWave)
				{
					for (const weak_ptr<EnemyActor>& enemy : endlessRuntime.GetOwnedEnemies())
						if (const shared_ptr<EnemyActor> actor = enemy.lock()) actor->Destroy();
					world.TickInternal(0.f);
					endlessRuntime.Tick(0.f);
					++currentWave;
				}
			};
			const auto waveMatches = [&](int expectedLevel, int expectedCount) -> bool
			{
				const EncounterWaveSnapshot snapshot = endlessRuntime.BuildSnapshot();
				return snapshot.sequenceMode == EncounterSequenceMode::EndlessCycle &&
					!snapshot.totalWaveCount.has_value() &&
					snapshot.state != EncounterWaveState::Completed &&
					snapshot.currentWaveNumber == currentWave &&
					snapshot.enemyLevel == expectedLevel &&
					snapshot.plannedEnemyCount == expectedCount &&
					snapshot.spawnedEnemyCount == expectedCount &&
					snapshot.remainingSpawnCount == 0;
			};

			// Authored shape: 3-4-5, then 4-5-6 after the level-up drop, then 5-6-7.
			const int expectedLevels12[]{ 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4 };
			const int expectedCounts12[]{ 3, 4, 5, 4, 5, 6, 5, 6, 7, 6, 7, 8 };
			for (size_t index = 0; index < 12; ++index)
			{
				if (!waveMatches(expectedLevels12[index], expectedCounts12[index]))
					return Fail("Endless encounter count/level curve diverged within the first twelve waves") ? 0 : 1;
				advanceTo(currentWave + 1);
			}

			// Level 15 starts at wave 43; the drop stops there, so the count rises by one per wave.
			const int expectedLevelsCap[]{ 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15 };
			const int expectedCountsCap[]{ 16, 17, 18, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26 };
			advanceTo(40);
			for (size_t index = 0; index < 13; ++index)
			{
				if (!waveMatches(expectedLevelsCap[index], expectedCountsCap[index]))
					return Fail("Endless encounter count/level curve diverged around the level cap") ? 0 : 1;
				if (index + 1 < 13) advanceTo(currentWave + 1);
			}
			advanceTo(60);
			if (!waveMatches(15, 34)) return Fail("Endless encounter count diverged at wave 60") ? 0 : 1;
			advanceTo(100);
			if (!waveMatches(15, 74)) return Fail("Endless encounter count diverged at wave 100") ? 0 : 1;
		}

		std::cout << "[TEST Encounter Endless Template Cycling]" << std::endl;
		{
			World world{ nullptr };
			EncounterWaveRuntime cyclingRuntime;
			List<EnemySpawnContext> cyclingContexts;
			List<List<std::string>> cyclingWaves;
			const List<EnemyWaveDefinition> templates{
				{ { { sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 2, 0.f } }, 0.f },
				{ { { sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 1, 0.f },
					{ sas::ContentId{ EnemyIds::StrafeSkirmisherBasic }, 1, 0.f } }, 0.f },
				{ { { sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 1, 0.f },
					{ sas::ContentId{ EnemyIds::StrafeSkirmisherBasic }, 1, 0.f },
					{ sas::ContentId{ EnemyIds::RangeKeeperBasic }, 1, 0.f } }, 0.f }
			};
			if (!cyclingRuntime.Start(templates,
				[&](const sas::ContentId& enemyId, size_t spawnIndex, const EnemySpawnContext& context)
				{
					cyclingContexts.push_back(context);
					cyclingWaves.back().push_back(enemyId.ToString());
					return content::SpawnEnemy(world, enemyId.ToString(), { 1500.f + 20.f * static_cast<float>(spawnIndex), 1500.f }, 1.f, context);
				}, { 1, 3, 0, 15, 1337u }, EncounterSequenceMode::EndlessCycle))
				return Fail("Endless cycling fixture failed to start") ? 0 : 1;

			cyclingWaves.push_back({});
			cyclingRuntime.Tick(0.f);
			const EncounterWaveSnapshot firstWave = cyclingRuntime.BuildSnapshot();
			if (firstWave.currentWaveNumber != 1 || firstWave.enemyLevel != 1 ||
				firstWave.plannedEnemyCount != 3 || firstWave.spawnedEnemyCount != 3)
				return Fail("Endless cycling fixture did not open on wave 1") ? 0 : 1;
			const uint32_t wave1FirstSeed = cyclingContexts.front().variationSeed;

			for (size_t wave = 1; wave < 6; ++wave)
			{
				for (const weak_ptr<EnemyActor>& enemy : cyclingRuntime.GetOwnedEnemies())
					if (const shared_ptr<EnemyActor> actor = enemy.lock()) actor->Destroy();
				world.TickInternal(0.f);
				cyclingWaves.push_back({});
				cyclingRuntime.Tick(0.f);
			}

			if (cyclingWaves.size() != 6) return Fail("Endless cycling fixture did not visit six waves") ? 0 : 1;
			if (cyclingWaves[0].size() != 3 || cyclingWaves[1].size() != 4 || cyclingWaves[2].size() != 5 ||
				cyclingWaves[3].size() != 4 || cyclingWaves[4].size() != 5 || cyclingWaves[5].size() != 6)
				return Fail("Endless cycling planned counts did not follow the agreed curve") ? 0 : 1;

			// Wave 4 reuses template 0 but must keep the ABSOLUTE wave number for level and seed.
			const EncounterWaveSnapshot wave4 = cyclingRuntime.BuildSnapshot();
			if (wave4.currentWaveNumber != 6 || wave4.enemyLevel != 2 || wave4.sequenceMode != EncounterSequenceMode::EndlessCycle ||
				wave4.totalWaveCount.has_value())
				return Fail("Endless cycling did not keep absolute wave numbering") ? 0 : 1;
			if (cyclingWaves[3] != List<std::string>{ EnemyIds::ApproachGunnerBasic, EnemyIds::ApproachGunnerBasic,
				EnemyIds::ApproachGunnerBasic, EnemyIds::ApproachGunnerBasic })
				return Fail("Endless wave 4 did not reuse template 0 composition") ? 0 : 1;
			if (cyclingWaves[4] != List<std::string>{ EnemyIds::ApproachGunnerBasic, EnemyIds::ApproachGunnerBasic,
				EnemyIds::ApproachGunnerBasic, EnemyIds::StrafeSkirmisherBasic, EnemyIds::StrafeSkirmisherBasic })
				return Fail("Endless wave 5 did not reuse template 1 composition") ? 0 : 1;
			if (cyclingWaves[5] != List<std::string>{ EnemyIds::ApproachGunnerBasic, EnemyIds::ApproachGunnerBasic,
				EnemyIds::StrafeSkirmisherBasic, EnemyIds::StrafeSkirmisherBasic,
				EnemyIds::RangeKeeperBasic, EnemyIds::RangeKeeperBasic })
				return Fail("Endless wave 6 did not reuse template 2 composition") ? 0 : 1;

			// Template 0 recurs at wave 4; if the wave component of the seed had wrapped, the
			// first spawn seed would repeat instead of changing.
			size_t wave4FirstContext = 3 + 4 + 5;
			if (cyclingContexts[wave4FirstContext].variationSeed == wave1FirstSeed)
				return Fail("Endless cycling wrapped the absolute wave component of the spawn seed") ? 0 : 1;
		}

		std::cout << "[TEST Encounter Concurrent Spawn Cap]" << std::endl;
		{
			World world{ nullptr };
			EncounterWaveRuntime cappedRuntime;
			if (!cappedRuntime.Start({ { { { sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 5, 0.f } }, 0.f } },
				[&](const sas::ContentId& enemyId, size_t spawnIndex, const EnemySpawnContext& context)
				{
					return content::SpawnEnemy(world, enemyId.ToString(), { 800.f + 100.f * static_cast<float>(spawnIndex), 800.f }, 1.f, context);
				}, { 2, 2, 0, 1, 1337u, 2 }))
				return Fail("Encounter runtime rejected a valid concurrent spawn cap") ? 0 : 1;
			cappedRuntime.Tick(0.f);
			EncounterWaveSnapshot snapshot = cappedRuntime.BuildSnapshot();
			if (snapshot.spawnedEnemyCount != 2 || snapshot.aliveEnemyCount != 2 || snapshot.remainingSpawnCount != 3)
				return Fail("Encounter runtime exceeded or ignored the concurrent spawn cap") ? 0 : 1;

			if (const shared_ptr<EnemyActor> firstEnemy = cappedRuntime.GetOwnedEnemies().front().lock()) firstEnemy->Destroy();
			world.TickInternal(0.f);
			cappedRuntime.Tick(0.f);
			snapshot = cappedRuntime.BuildSnapshot();
			if (snapshot.spawnedEnemyCount != 3 || snapshot.aliveEnemyCount != 2 || snapshot.remainingSpawnCount != 2)
				return Fail("Encounter runtime did not refill a freed concurrent spawn slot") ? 0 : 1;

			for (const weak_ptr<EnemyActor>& enemy : cappedRuntime.GetOwnedEnemies())
				if (const shared_ptr<EnemyActor> actor = enemy.lock()) actor->Destroy();
			world.TickInternal(0.f);
			cappedRuntime.Tick(0.f);
			snapshot = cappedRuntime.BuildSnapshot();
			if (snapshot.spawnedEnemyCount != 5 || snapshot.aliveEnemyCount != 2 || snapshot.remainingSpawnCount != 0)
				return Fail("Encounter runtime did not preserve planned count while batching spawns") ? 0 : 1;

			for (const weak_ptr<EnemyActor>& enemy : cappedRuntime.GetOwnedEnemies())
				if (const shared_ptr<EnemyActor> actor = enemy.lock()) actor->Destroy();
			world.TickInternal(0.f);
			cappedRuntime.Tick(0.f);
			if (!cappedRuntime.IsCompleted())
				return Fail("Encounter runtime did not complete after capped wave clear") ? 0 : 1;
		}

		std::cout << "[TEST Encounter HUD View Model]" << std::endl;
		{
			EncounterWaveSnapshot finiteSnapshot;
			finiteSnapshot.state = EncounterWaveState::Spawning;
			finiteSnapshot.sequenceMode = EncounterSequenceMode::Finite;
			finiteSnapshot.currentWaveNumber = 2;
			finiteSnapshot.totalWaveCount = 3;
			finiteSnapshot.enemyLevel = 1;
			const EncounterHUDViewModel finiteModel = BuildEncounterHUDViewModel(finiteSnapshot);
			if (!finiteModel.visible || finiteModel.title != "WAVE 2 / 3" ||
				finiteModel.detail.find("LEVEL 1") == std::string::npos)
				return Fail("Encounter HUD did not render the finite wave header") ? 0 : 1;

			EncounterWaveSnapshot endlessSnapshot;
			endlessSnapshot.state = EncounterWaveState::Spawning;
			endlessSnapshot.sequenceMode = EncounterSequenceMode::EndlessCycle;
			endlessSnapshot.currentWaveNumber = 27;
			endlessSnapshot.enemyLevel = 9;
			const EncounterHUDViewModel endlessModel = BuildEncounterHUDViewModel(endlessSnapshot);
			if (!endlessModel.visible || endlessModel.title != "WAVE 27 | ENDLESS" ||
				endlessModel.detail.find("LEVEL 9") == std::string::npos)
				return Fail("Encounter HUD did not render the endless wave header") ? 0 : 1;

			EncounterWaveSnapshot interWaveSnapshot;
			interWaveSnapshot.state = EncounterWaveState::InterWaveDelay;
			interWaveSnapshot.sequenceMode = EncounterSequenceMode::EndlessCycle;
			interWaveSnapshot.currentWaveNumber = 27;
			interWaveSnapshot.interWaveRemainingTime = 1.f;
			const EncounterHUDViewModel interWaveModel = BuildEncounterHUDViewModel(interWaveSnapshot);
			if (interWaveModel.title != "WAVE 27 CLEARED" ||
				interWaveModel.detail.find("NEXT WAVE IN") == std::string::npos)
				return Fail("Encounter HUD changed its inter-wave wording") ? 0 : 1;
		}

		std::cout << "[TEST Arena Encounter Lifecycle]" << std::endl;
		{
			PlayerManager::GetPlayerManager().Reset();
			const shared_ptr<ArenaLifecycleTestLevel> playerlessArena = std::make_shared<ArenaLifecycleTestLevel>(nullptr);
			if (playerlessArena->StartEncounter() || playerlessArena->GetEncounterState() != ArenaEncounterState::Failed)
				return Fail("Arena encounter started without an active player ship") ? 0 : 1;

			const shared_ptr<ArenaLifecycleTestLevel> arena = std::make_shared<ArenaLifecycleTestLevel>(nullptr);
			const shared_ptr<PlayerSpaceShip> encounterPlayer = arena->SpawnActor<PlayerSpaceShip>().lock();
			const shared_ptr<PlayerSpaceShip> externalPlayer = arena->SpawnActor<PlayerSpaceShip>().lock();
			if (!encounterPlayer || !externalPlayer)
				return Fail("Arena lifecycle fixture could not create player ships") ? 0 : 1;

			arena->SetEncounterPlayer(encounterPlayer);
			if (!arena->StartEncounter() || arena->GetEncounterState() != ArenaEncounterState::Running)
				return Fail("Arena encounter did not enter Running with a player ship") ? 0 : 1;
			const EncounterWaveSnapshot arenaSnapshot = arena->GetEncounterWaveSnapshot();
			if (arenaSnapshot.state != EncounterWaveState::Spawning ||
				arenaSnapshot.sequenceMode != EncounterSequenceMode::EndlessCycle ||
				arenaSnapshot.totalWaveCount.has_value() ||
				arenaSnapshot.currentWaveNumber != 1 ||
				arenaSnapshot.plannedEnemyCount != 3 ||
				arenaSnapshot.spawnedEnemyCount != 0 ||
				arenaSnapshot.enemyLevel != 1)
				return Fail("Arena encounter did not forward a read-only endless runtime wave snapshot") ? 0 : 1;
			arena->TickEncounter(0.f);
			encounterPlayer->Destroy();
			if (arena->GetEncounterState() != ArenaEncounterState::Failed)
				return Fail("Arena encounter did not fail when its player ship was destroyed") ? 0 : 1;
			if (externalPlayer->GetIsPendingDestroy())
				return Fail("Arena encounter failure destroyed an external actor") ? 0 : 1;

			arena->SetEncounterPlayer(externalPlayer);
			if (!arena->RestartEncounter() || arena->GetEncounterState() != ArenaEncounterState::Running)
				return Fail("Arena encounter restart did not reset to a fresh Running state") ? 0 : 1;
		}

		std::cout << "[TEST Arena Playtest Telemetry]" << std::endl;
		{
			PlayerManager::GetPlayerManager().Reset();
			const shared_ptr<ArenaLifecycleTestLevel> playerlessArena =
				std::make_shared<ArenaLifecycleTestLevel>(nullptr);
			if (playerlessArena->StartEncounter() ||
				playerlessArena->GetPlaytestMetrics().active ||
				playerlessArena->GetPlaytestMetrics().summaryEmitCount != 0)
			{
				return Fail("Playtest telemetry started without a valid encounter") ? 0 : 1;
			}

			const shared_ptr<ArenaLifecycleTestLevel> arena =
				std::make_shared<ArenaLifecycleTestLevel>(nullptr);
			const shared_ptr<PlayerSpaceShip> player = arena->SpawnActor<PlayerSpaceShip>().lock();
			if (!player)
			{
				return Fail("Playtest telemetry fixture could not create a player ship") ? 0 : 1;
			}
			arena->SetEncounterPlayer(player);
			// The unarmed telemetry fixture must survive long enough to measure
			// crafted hits, so its baseline health/shield are set explicitly before
			// any encounter frame runs.
			player->GetHealthComponent().SetInitialHealth(250.f, 250.f);
			player->GetShieldComponent().SetMaxShield(100.f);
			player->GetShieldComponent().ChangeShield(100.f);
			if (!arena->StartEncounter())
			{
				return Fail("Playtest telemetry fixture could not start the encounter") ? 0 : 1;
			}

			{
				const ArenaPlaytestMetrics& metrics = arena->GetPlaytestMetrics();
				if (!metrics.active || metrics.duration != 0.0 || metrics.totalSpawned != 0 ||
					metrics.totalKilled != 0 || metrics.summaryEmitCount != 0 || !metrics.waves.empty())
				{
					return Fail("Playtest telemetry did not start from a zeroed run state") ? 0 : 1;
				}
			}

			arena->TickEncounter(0.5f);
			{
				const ArenaPlaytestMetrics& metrics = arena->GetPlaytestMetrics();
				if (!NearlyEqual(static_cast<float>(metrics.duration), 0.5f) ||
					metrics.waves.size() != 1 || metrics.waves.front().waveNumber != 1 ||
					metrics.waves.front().startTime != 0.0 || !metrics.waves.front().started)
				{
					const std::string diagnostic =
						"Playtest telemetry did not start wave one exactly once (duration=" +
						std::to_string(metrics.duration) +
						" waves=" + std::to_string(metrics.waves.size()) + ")";
					return Fail(diagnostic.c_str()) ? 0 : 1;
				}
			}

			// Encounter teardown must never be counted as kills (checked after the
			// terminal state below, where teardown actually destroys enemies).
			arena->TickEncounter(0.6f);
			{
				const ArenaPlaytestMetrics& metrics = arena->GetPlaytestMetrics();
				if (metrics.waves.size() != 1)
				{
					return Fail("Playtest telemetry recorded the same wave start twice") ? 0 : 1;
				}
				if (metrics.totalSpawned <= 0 ||
					metrics.waves.front().spawned != metrics.totalSpawned)
				{
					return Fail("Playtest telemetry did not attribute spawns to the active wave") ? 0 : 1;
				}
				if (arena->GetEncounterState() == ArenaEncounterState::Running &&
					metrics.totalSpawned != arena->GetEncounterWaveSnapshot().spawnedEnemyCount)
				{
					return Fail("Playtest telemetry did not count only encounter-owned spawns") ? 0 : 1;
				}
				if (metrics.peakAlive < 0 || !(metrics.aliveIntegral >= 0.0) ||
					!(metrics.aliveSampledDuration > 0.0))
				{
					return Fail("Playtest telemetry produced invalid alive-enemy accumulation") ? 0 : 1;
				}
			}

			arena->TriggerBoundaryPenaltyForTest();
			arena->TriggerBoundaryPenaltyForTest();
			if (arena->GetPlaytestMetrics().boundaryPenaltyCount != 2)
			{
				return Fail("Playtest telemetry did not count boundary penalties per event") ? 0 : 1;
			}

			// A restart must not carry measurement state into the next run, and the
			// encounter teardown that a restart performs must never count as kills.
			const shared_ptr<PlayerSpaceShip> restartedPlayer = arena->SpawnActor<PlayerSpaceShip>().lock();
			if (!restartedPlayer)
			{
				return Fail("Playtest telemetry restart fixture could not create a player ship") ? 0 : 1;
			}
			arena->SetEncounterPlayer(restartedPlayer);
			if (!arena->RestartEncounter())
			{
				return Fail("Playtest telemetry restart fixture could not restart the encounter") ? 0 : 1;
			}
			{
				const ArenaPlaytestMetrics& metrics = arena->GetPlaytestMetrics();
				if (!metrics.active || metrics.duration != 0.0 || metrics.totalSpawned != 0 ||
					metrics.totalKilled != 0 || metrics.totalDamageTaken != 0.0 ||
					metrics.playerDeathCount != 0 || metrics.summaryEmitCount != 0 ||
					metrics.boundaryPenaltyCount != 0 || !metrics.waves.empty())
				{
					return Fail("Playtest telemetry carried state across a restart") ? 0 : 1;
				}
			}
		}

		std::cout << "[TEST Arena Playtest Telemetry Reload]" << std::endl;
		{
			std::string windowTitle = "LightYears GasLite Arena Telemetry";
			Application application{ { 320, 240 }, 32, windowTitle, sf::Style::None };
			PlayerManager::GetPlayerManager().Reset();
			const shared_ptr<ArenaLifecycleTestLevel> arena =
				std::make_shared<ArenaLifecycleTestLevel>(&application);
			arena->BeginPlayInternal();
			if (!arena->StartEncounter())
			{
				return Fail("Playtest telemetry reload fixture could not start the encounter") ? 0 : 1;
			}
			Player* player = PlayerManager::GetPlayerManager().GetPlayer();
			const shared_ptr<PlayerSpaceShip> playerShip =
				player ? player->GetCurrentSpaceShip().lock() : nullptr;
			GameAbility* primary = playerShip
				? playerShip->GetCombatRuntime().GetAbilitySystemComponent().GetAbility(
					sas::AbilitySlot::PrimaryFire)
				: nullptr;
			if (!playerShip || !primary)
			{
				return Fail("Playtest telemetry reload fixture has no primary weapon ability") ? 0 : 1;
			}
			if (playerShip->IsInvulnerable())
			{
				// Spawn protection is cleared by the level's own spawn wiring in
				// production; the same public API is used here so crafted hits travel
				// the real damage path.
				playerShip->SetInvulnerability(false);
			}

			// Positive kill/TTK accounting is blocked in tests: encounter enemies are
			// spawned into World::mPendingActors and only promoted to mActors by
			// World::TickInternal, which is driven by the Application's private tick
			// path (World::Tick is an empty virtual). No existing public/testable
			// route exposes an encounter-owned enemy to destroy, so the positive path
			// is reported as blocked instead of faked.

			primary->GetPrimaryWeaponRuntime().magazineState.reloadRemaining = 5.0;
			arena->TickEncounter(0.2f);
			if (arena->GetPlaytestMetrics().reloadCount != 1 ||
				!NearlyEqual(static_cast<float>(arena->GetPlaytestMetrics().totalReloadTime), 0.2f))
			{
				return Fail("Playtest telemetry did not detect the first reload window") ? 0 : 1;
			}
			arena->TickEncounter(0.2f);
			if (arena->GetPlaytestMetrics().reloadCount != 1 ||
				!NearlyEqual(static_cast<float>(arena->GetPlaytestMetrics().totalReloadTime), 0.4f))
			{
				return Fail("Playtest telemetry recounted an ongoing reload window") ? 0 : 1;
			}

			{
				DamageContext reloadHit;
				reloadHit.target = playerShip.get();
				reloadHit.originalDamage = 5.f;
				reloadHit.remainingDamage = 5.f;
				reloadHit.payload.shieldDamageMultiplier = 1.f;
				playerShip->ReceiveDamage(reloadHit);
				if (!NearlyEqual(
					static_cast<float>(arena->GetPlaytestMetrics().damageWhileReloading), 5.f))
				{
					return Fail("Playtest telemetry did not accumulate damage taken while reloading") ? 0 : 1;
				}
			}

			primary->GetPrimaryWeaponRuntime().magazineState.reloadRemaining = 0.0;
			arena->TickEncounter(0.2f);
			const double reloadTimeAfterFirstWindow = arena->GetPlaytestMetrics().totalReloadTime;
			{
				DamageContext idleHit;
				idleHit.target = playerShip.get();
				idleHit.originalDamage = 5.f;
				idleHit.remainingDamage = 5.f;
				idleHit.payload.shieldDamageMultiplier = 1.f;
				playerShip->ReceiveDamage(idleHit);
				if (!NearlyEqual(
					static_cast<float>(arena->GetPlaytestMetrics().damageWhileReloading), 5.f))
				{
					return Fail("Playtest telemetry counted damage while not reloading") ? 0 : 1;
				}
			}

			primary->GetPrimaryWeaponRuntime().magazineState.reloadRemaining = 5.0;
			arena->TickEncounter(0.2f);
			{
				const ArenaPlaytestMetrics& metrics = arena->GetPlaytestMetrics();
				if (metrics.reloadCount != 2 || !(metrics.totalReloadTime > reloadTimeAfterFirstWindow))
				{
					return Fail("Playtest telemetry did not detect the second reload window") ? 0 : 1;
				}
			}
		}

		std::cout << "[TEST Enemy Window Cull Policy]" << std::endl;
		{
			std::string windowTitle = "LightYears GasLite Enemy Cull Policy";
			Application application{ { 320, 240 }, 32, windowTitle, sf::Style::None };
			PlayerManager::GetPlayerManager().Reset();
			const shared_ptr<ArenaLifecycleTestLevel> arena =
				std::make_shared<ArenaLifecycleTestLevel>(&application);
			arena->BeginPlayInternal();

			// Far outside the 320x240 test window, so only the cull policy decides.
			const sf::Vector2f farLocation{ 5000.f, 2500.f };
			const std::string enemyId = "Enemy.StrafeSkirmisher.Basic";

			EnemySpawnContext defaultContext;
			defaultContext.level = 1;
			const shared_ptr<EnemyActor> defaultEnemy =
				content::SpawnEnemy(*arena, enemyId, farLocation, 1.f, defaultContext).lock();
			if (!defaultEnemy)
			{
				return Fail("Enemy cull policy fixture could not spawn a default enemy") ? 0 : 1;
			}
			defaultEnemy->Tick(0.1f);
			if (!defaultEnemy->GetIsPendingDestroy())
			{
				return Fail("Default enemies no longer follow the camera-view cull") ? 0 : 1;
			}

			EnemySpawnContext encounterContext;
			encounterContext.level = 1;
			encounterContext.windowCullEnabled = false;
			const shared_ptr<EnemyActor> encounterEnemy =
				content::SpawnEnemy(*arena, enemyId, farLocation, 1.f, encounterContext).lock();
			if (!encounterEnemy)
			{
				return Fail("Enemy cull policy fixture could not spawn an encounter enemy") ? 0 : 1;
			}
			encounterEnemy->Tick(0.1f);
			encounterEnemy->Tick(0.1f);
			if (encounterEnemy->GetIsPendingDestroy())
			{
				return Fail("Encounter spawn policy did not disable the camera-view cull") ? 0 : 1;
			}
			encounterEnemy->Destroy();
			if (!encounterEnemy->GetIsPendingDestroy())
			{
				return Fail("Encounter enemies can no longer be destroyed normally") ? 0 : 1;
			}
		}

		std::cout << "[TEST Arena Playtest Telemetry Damage]" << std::endl;
		{
			// Damage, kill/TTK and lethal ordering are only meaningful in a fixture
			// that runs the production BeginPlay lifecycle: PlayerSpaceShip spawns
			// invulnerable and is only released by the level's own spawn wiring, and
			// spawned enemies only become world actors once the world ticks.
			std::string windowTitle = "LightYears GasLite Arena Telemetry Damage";
			Application application{ { 320, 240 }, 32, windowTitle, sf::Style::None };
			PlayerManager::GetPlayerManager().Reset();
			const shared_ptr<ArenaLifecycleTestLevel> arena =
				std::make_shared<ArenaLifecycleTestLevel>(&application);
			arena->BeginPlayInternal();
			if (!arena->StartEncounter())
			{
				return Fail("Playtest telemetry damage fixture could not start the encounter") ? 0 : 1;
			}
			Player* player = PlayerManager::GetPlayerManager().GetPlayer();
			const shared_ptr<PlayerSpaceShip> playerShip =
				player ? player->GetCurrentSpaceShip().lock() : nullptr;
			if (!playerShip)
			{
				return Fail("Playtest telemetry damage fixture has no player ship") ? 0 : 1;
			}
			if (playerShip->IsInvulnerable())
			{
				// Spawn protection is cleared by the level's own spawn wiring in
				// production; the same public API is used here so crafted hits travel
				// the real damage path.
				playerShip->SetInvulnerability(false);
			}

			// The observer binds on the first active frame.
			arena->TickEncounter(0.6f);
			arena->TickEncounter(0.1f);

			// NOTE: the positive kill/TTK assertion is blocked in tests. Encounter
			// enemies are spawned into World::mPendingActors and are only promoted to
			// mActors by World::TickInternal, which is driven by the Application's
			// private tick path (World::Tick itself is an empty virtual), so no
			// existing public/testable route can expose an encounter-owned enemy to
			// destroy. Kill/TTK finalization therefore stays covered by the forced
			// cleanup assertions below, and the positive path is reported as blocked
			// rather than faked.

			// Case 1: fully shielded hit.
			{
				playerShip->GetShieldComponent().SetMaxShield(100.f);
				playerShip->GetShieldComponent().ChangeShield(100.f);
				playerShip->GetHealthComponent().SetInitialHealth(250.f, 250.f);
				const double shipShieldBefore = static_cast<double>(playerShip->GetShieldComponent().GetShield());
				const double totalBefore = arena->GetPlaytestMetrics().totalDamageTaken;
				const double shieldBefore = arena->GetPlaytestMetrics().shieldDamageTaken;
				const double hullBefore = arena->GetPlaytestMetrics().hullDamageTaken;
				DamageContext shieldedHit;
				shieldedHit.target = playerShip.get();
				shieldedHit.originalDamage = 10.f;
				shieldedHit.remainingDamage = 10.f;
				shieldedHit.payload.shieldDamageMultiplier = 1.f;
				playerShip->ReceiveDamage(shieldedHit);
				const double applied = arena->GetPlaytestMetrics().totalDamageTaken - totalBefore;
				const double shield = arena->GetPlaytestMetrics().shieldDamageTaken - shieldBefore;
				const double hull = arena->GetPlaytestMetrics().hullDamageTaken - hullBefore;
				if (!(static_cast<double>(playerShip->GetShieldComponent().GetShield()) < shipShieldBefore) ||
					!(applied > 0.0) ||
					!NearlyEqual(static_cast<float>(shield), static_cast<float>(applied)) ||
					!NearlyEqual(static_cast<float>(hull), 0.f))
				{
					return Fail("Playtest telemetry mis-split a fully shielded hit") ? 0 : 1;
				}
			}

			// Case 2: mixed shield + hull hit.
			{
				playerShip->GetShieldComponent().SetMaxShield(100.f);
				playerShip->GetShieldComponent().ChangeShield(100.f);
				playerShip->GetHealthComponent().SetInitialHealth(250.f, 250.f);
				const double totalBefore = arena->GetPlaytestMetrics().totalDamageTaken;
				const double shieldBefore = arena->GetPlaytestMetrics().shieldDamageTaken;
				const double hullBefore = arena->GetPlaytestMetrics().hullDamageTaken;
				DamageContext mixedHit;
				mixedHit.target = playerShip.get();
				mixedHit.originalDamage = 200.f;
				mixedHit.remainingDamage = 200.f;
				mixedHit.payload.shieldDamageMultiplier = 1.f;
				playerShip->ReceiveDamage(mixedHit);
				const double applied = arena->GetPlaytestMetrics().totalDamageTaken - totalBefore;
				const double shield = arena->GetPlaytestMetrics().shieldDamageTaken - shieldBefore;
				const double hull = arena->GetPlaytestMetrics().hullDamageTaken - hullBefore;
				if (!(shield > 0.0) || !(hull > 0.0) ||
					!NearlyEqual(static_cast<float>(applied), static_cast<float>(shield + hull)))
				{
					return Fail("Playtest telemetry mis-split a mixed shield/hull hit") ? 0 : 1;
				}
			}

			// Case 3: overkill is capped and the lethal hit must still be observed
			// before the terminal summary is emitted.
			{
				playerShip->GetShieldComponent().SetMaxShield(0.f);
				playerShip->GetHealthComponent().SetInitialHealth(150.f, 150.f);
				const double healthBefore = static_cast<double>(playerShip->GetHealthComponent().GetHealth());
				const double totalBefore = arena->GetPlaytestMetrics().totalDamageTaken;
				const double shieldBefore = arena->GetPlaytestMetrics().shieldDamageTaken;
				const double hullBefore = arena->GetPlaytestMetrics().hullDamageTaken;
				DamageContext overkillHit;
				overkillHit.target = playerShip.get();
				overkillHit.originalDamage = 10000.f;
				overkillHit.remainingDamage = 10000.f;
				overkillHit.payload.shieldDamageMultiplier = 1.f;
				playerShip->ReceiveDamage(overkillHit);
				const double applied = arena->GetPlaytestMetrics().totalDamageTaken - totalBefore;
				const double shield = arena->GetPlaytestMetrics().shieldDamageTaken - shieldBefore;
				const double hull = arena->GetPlaytestMetrics().hullDamageTaken - hullBefore;
				if (playerShip->GetHealthComponent().GetHealth() > 0.f || !(hull > 0.0) ||
					!(hull <= healthBefore) ||
					!NearlyEqual(static_cast<float>(applied), static_cast<float>(shield + hull)))
				{
					return Fail("Playtest telemetry did not cap overkill to the applied loss") ? 0 : 1;
				}
			}

			// The lethal damage has already been recorded by the resolved-damage
			// observer. The encounter death path is then driven through the normal
			// public actor lifecycle, exactly as the existing arena lifecycle test
			// does, because this harness does not run the world tick that completes
			// ship destruction.
			playerShip->Destroy();

			arena->TickEncounter(0.1f);
			{
				const ArenaPlaytestMetrics& metrics = arena->GetPlaytestMetrics();
				if (arena->GetEncounterState() != ArenaEncounterState::Failed ||
					metrics.playerDeathCount != 1 || metrics.playerDeathWave != 1 ||
					!(metrics.playerDeathTime > 0.0) || metrics.summaryEmitCount != 1)
				{
					const std::string diagnostic =
						"Playtest telemetry did not finalize a failed run exactly once (state=" +
						std::to_string(static_cast<int>(arena->GetEncounterState())) +
						" deaths=" + std::to_string(metrics.playerDeathCount) +
						" summaries=" + std::to_string(metrics.summaryEmitCount) + ")";
					return Fail(diagnostic.c_str()) ? 0 : 1;
				}
				if (!(metrics.totalDamageTaken > 0.0) || metrics.totalKilled != 0)
				{
					return Fail("Playtest telemetry lost measured data on failure") ? 0 : 1;
				}
			}
			arena->TickEncounter(0.1f);
			arena->TickEncounter(0.1f);
			if (arena->GetPlaytestMetrics().summaryEmitCount != 1)
			{
				return Fail("Playtest telemetry emitted its summary more than once") ? 0 : 1;
			}
			if (arena->GetPlaytestMetrics().totalKilled != 0)
			{
				return Fail("Playtest telemetry counted encounter teardown as kills") ? 0 : 1;
			}
		}

		std::cout << "[TEST Arena Level Restart Lifecycle]" << std::endl;
		shared_ptr<PlayerSpaceShip> retainedPlayerShip;
		{
			std::string windowTitle = "LightYears GasLite Arena Lifecycle";
			Application application{ { 320, 240 }, 32, windowTitle, sf::Style::None };
			PlayerManager::GetPlayerManager().Reset();
			const shared_ptr<ArenaLifecycleTestLevel> arena = std::make_shared<ArenaLifecycleTestLevel>(&application);
			arena->BeginPlayInternal();
			if (!arena->StartEncounter())
				return Fail("Arena level restart fixture could not start its encounter") ? 0 : 1;

			// One live frame drives the registered encounter HUD controller through the real arena provider.
			arena->TickEncounter(0.f);
			if (arena->GetEncounterWaveSnapshot().state != EncounterWaveState::Spawning)
				return Fail("Arena level HUD frame did not keep the encounter spawning") ? 0 : 1;

			Player* player = PlayerManager::GetPlayerManager().GetPlayer();
			const shared_ptr<PlayerSpaceShip> previousPlayerShip = player ? player->GetCurrentSpaceShip().lock() : nullptr;
			if (!previousPlayerShip || arena->GetEncounterState() != ArenaEncounterState::Running)
				return Fail("Arena level restart fixture did not start with a live player ship") ? 0 : 1;

			arena->RestartLevelForTest();
			if (!previousPlayerShip->GetIsPendingDestroy())
				return Fail("Arena level restart left the previous player ship alive") ? 0 : 1;

			player = PlayerManager::GetPlayerManager().GetPlayer();
			const shared_ptr<PlayerSpaceShip> restartedPlayerShip = player ? player->GetCurrentSpaceShip().lock() : nullptr;
			if (!restartedPlayerShip || restartedPlayerShip.get() == previousPlayerShip.get() ||
				restartedPlayerShip->GetIsPendingDestroy() || arena->GetEncounterState() != ArenaEncounterState::Running)
				return Fail("Arena level restart did not create a fresh running player encounter") ? 0 : 1;

			std::cout << "[TEST Encounter HUD Presentation]" << std::endl;
			{
				const shared_ptr<EncounterHUDTestHUD> encounterHUD = std::make_shared<EncounterHUDTestHUD>();
				encounterHUD->NativeInit(application.GetRenderWindow());
				const size_t baseWidgetCount = encounterHUD->GetWidgetCount();

				EncounterWaveSnapshot snapshot{};
				{
					EncounterHUDController controller{ encounterHUD, [&snapshot]() { return snapshot; } };
					controller.Tick(0.f);

					// Idle: the encounter widgets exist but present nothing.
					if (encounterHUD->GetWidgetCount() != baseWidgetCount + 2 || encounterHUD->GetVisibleWidgetCount() != 0)
						return Fail("Encounter HUD presented widgets while the encounter was idle") ? 0 : 1;

					snapshot.state = EncounterWaveState::Spawning;
					snapshot.currentWaveNumber = 2;
					snapshot.totalWaveCount = 3;
					snapshot.plannedEnemyCount = 4;
					snapshot.spawnedEnemyCount = 3;
					snapshot.aliveEnemyCount = 3;
					snapshot.remainingSpawnCount = 1;
					snapshot.enemyLevel = 2;
					const EncounterHUDViewModel spawningModel = BuildEncounterHUDViewModel(snapshot);
					if (!spawningModel.visible || spawningModel.title != "WAVE 2 / 3" ||
						spawningModel.detail != "ENEMIES 4  \xE2\x80\xA2  LEVEL 2")
						return Fail("Encounter HUD did not present the spawning wave and threat count") ? 0 : 1;
					controller.Tick(0.f);
					if (encounterHUD->GetVisibleWidgetCount() != 2)
						return Fail("Encounter HUD did not show both lines for an active wave") ? 0 : 1;

					// WaitingForClear: only spawned-and-alive enemies count toward the remaining threat.
					snapshot.state = EncounterWaveState::WaitingForClear;
					snapshot.aliveEnemyCount = 2;
					snapshot.remainingSpawnCount = 0;
					if (BuildEncounterHUDViewModel(snapshot).detail != "ENEMIES 2  \xE2\x80\xA2  LEVEL 2")
						return Fail("Encounter HUD counted enemies that had not spawned yet") ? 0 : 1;

					// Inter-wave: the cleared wave plus an upward-rounded countdown.
					snapshot.state = EncounterWaveState::InterWaveDelay;
					snapshot.currentWaveNumber = 1;
					snapshot.interWaveRemainingTime = 1.2f;
					const EncounterHUDViewModel interWaveModel = BuildEncounterHUDViewModel(snapshot);
					if (!interWaveModel.visible || interWaveModel.title != "WAVE 1 CLEARED" ||
						interWaveModel.detail != "NEXT WAVE IN 2")
						return Fail("Encounter HUD did not round the inter-wave countdown up") ? 0 : 1;
					snapshot.interWaveRemainingTime = 2.f;
					if (BuildEncounterHUDViewModel(snapshot).detail != "NEXT WAVE IN 2")
						return Fail("Encounter HUD rounded a whole-second countdown up") ? 0 : 1;

					// Completed: a single persistent line with no detail line.
					snapshot.state = EncounterWaveState::Completed;
					const EncounterHUDViewModel completedModel = BuildEncounterHUDViewModel(snapshot);
					if (!completedModel.visible || completedModel.title != "ENCOUNTER COMPLETE" || !completedModel.detail.empty())
						return Fail("Encounter HUD did not present the completed encounter") ? 0 : 1;
					controller.Tick(0.f);
					if (encounterHUD->GetVisibleWidgetCount() != 1)
						return Fail("Encounter HUD kept its detail line after completion") ? 0 : 1;

					// Failed: the game-over flow owns failure reporting, so the encounter HUD stays silent.
					snapshot.state = EncounterWaveState::Failed;
					if (BuildEncounterHUDViewModel(snapshot).visible)
						return Fail("Encounter HUD exposed a technical failure reason") ? 0 : 1;
					controller.Tick(0.f);
					if (encounterHUD->GetVisibleWidgetCount() != 0)
						return Fail("Encounter HUD presented a failed encounter") ? 0 : 1;

					// Repeated identical snapshots must not grow the widget list.
					snapshot.state = EncounterWaveState::Spawning;
					const size_t activeWidgetCount = encounterHUD->GetWidgetCount();
					for (int frame = 0; frame < 16; ++frame) controller.Tick(1.f / 60.f);
					if (encounterHUD->GetWidgetCount() != activeWidgetCount || encounterHUD->GetVisibleWidgetCount() != 2)
						return Fail("Encounter HUD created duplicate widgets for repeated snapshots") ? 0 : 1;
				}

				// Controller destruction must release the widgets it added.
				if (encounterHUD->GetWidgetCount() != baseWidgetCount)
					return Fail("Encounter HUD left dangling widgets behind after controller destruction") ? 0 : 1;

				// A provider bound to a weak level degrades to a neutral snapshot once that level is gone.
				weak_ptr<ArenaTestLevel> weakLevel;
				{
					const shared_ptr<ArenaLifecycleTestLevel> transientLevel =
						std::make_shared<ArenaLifecycleTestLevel>(nullptr);
					weakLevel = std::dynamic_pointer_cast<ArenaTestLevel>(transientLevel->GetWeakPtr().lock());
					if (weakLevel.expired())
						return Fail("Encounter HUD provider could not observe its arena level") ? 0 : 1;
				}
				if (!weakLevel.expired())
					return Fail("A weak encounter level outlived the level it observed") ? 0 : 1;

				EncounterHUDController detachedController{ encounterHUD, MakeWeakLevelSnapshotProvider(weakLevel) };
				detachedController.Tick(0.f);
				if (encounterHUD->GetVisibleWidgetCount() != 0)
					return Fail("Encounter HUD presented a neutral snapshot for a destroyed level") ? 0 : 1;
			}

			retainedPlayerShip = restartedPlayerShip;
		}
		if (retainedPlayerShip)
			retainedPlayerShip->onActorDestroyed.Broadcast(retainedPlayerShip.get());

		std::cout << "[TEST 19]" << std::endl;
		// 19. Each shipped enemy finds a real target and executes its authored attack through AI.
		{
			const char* enemyIds[] = {
				EnemyIds::ApproachGunnerBasic,
				EnemyIds::StrafeSkirmisherBasic,
				EnemyIds::RangeKeeperBasic
			};
			const float targetDistances[] = { 700.f, 400.f, 900.f };
			for (size_t index = 0; index < 3; ++index)
			{
				World world{ nullptr };
				const std::shared_ptr<EnemyActor> enemy = content::SpawnEnemy(
					world, enemyIds[index], { 1000.f, 1000.f }).lock();
				const std::shared_ptr<TestCombatant> target = world.SpawnActor<TestCombatant>().lock();
				if (!enemy || !target) return Fail("Enemy factory failed to spawn a test encounter") ? 0 : 1;
				target->SetCollisionLayer(CollisionLayer::Player);
				target->SetCollisionMask(CollisionLayer::Enemy | CollisionLayer::EnemyBullet);
				target->InitializeTestHealth(100000.f);
				const sf::Vector2f initialForward = enemy->GetActorForwardDirection();
				constexpr float TargetAngleRadians = 0.5235987756f;
				const sf::Vector2f angledDirection{
					initialForward.x * std::cos(TargetAngleRadians) - initialForward.y * std::sin(TargetAngleRadians),
					initialForward.x * std::sin(TargetAngleRadians) + initialForward.y * std::cos(TargetAngleRadians)
				};
				target->SetActorLocation(enemy->GetActorLocation() + angledDirection * targetDistances[index]);
				world.TickInternal(0.f);
				GameAbility* weapon = enemy->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::PrimaryFire);
				if (!weapon || !enemy->GetEnemyRuntime().IsReady() || !enemy->GetEnemyBehaviorRuntime().GetProfile())
					return Fail("A shipped enemy did not complete generic initialization") ? 0 : 1;
				const uint64_t fireCount = weapon->GetPrimaryWeaponRuntime().successfulFireCount;
				const float healthBefore = target->GetTestHealth();
				const float initialRotation = enemy->GetActorRotation();
				bool spawnedAttackActor = false;
				bool fired = false;
				bool damagedTarget = false;
				for (int frame = 0; frame < 240; ++frame)
				{
					world.TickInternal(1.f / 60.f);
					fired = weapon->GetPrimaryWeaponRuntime().successfulFireCount > fireCount;
					spawnedAttackActor = !world.GetActorsByType<PrimaryWeaponProjectileActor>().empty() ||
						!world.GetActorsByType<ExpandingWaveWeaponActor>().empty();
					damagedTarget = target->GetTestHealth() < healthBefore;
					if (fired && spawnedAttackActor && damagedTarget) break;
				}
				if (!fired || !spawnedAttackActor || !damagedTarget ||
					std::fabs(enemy->GetActorRotation() - initialRotation) <= 1.f)
				{
					return Fail("Enemy AI did not turn toward and damage a real target with its authored attack") ? 0 : 1;
				}
				enemy->ApplyDamage(std::numeric_limits<float>::max());
				if (!enemy->GetIsPendingDestroy() || !enemy->GetEnemyRuntime().IsEmpty())
					return Fail("A shipped enemy did not clear combat runtime on death") ? 0 : 1;
			}
		}

		std::cout << "[TEST 11]" << std::endl;
		// 11. Faz 1: First handle removal failure test (Adapter injection)
		{
			World world{ nullptr };
			TestCombatant combatant{ &world };
			MockEnemyAbilityOperations mockOps{ combatant.GetCombatRuntime() };
			EnemyRuntime enemyRuntime{ combatant.GetCombatRuntime(), &mockOps };

			EnemyCombatProfile profA;
			profA.profileId = "EnemyCombat.Test.ProfA";
			profA.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			profA.powerScalingPolicy = EnemyPowerScalingPolicy::Allowed;
			profA.abilities.push_back({ "Ability.Movement.Dash.Basic", sas::AbilitySlot::Ability1, 1 });
			std::string err;
			if (!enemyRuntime.Initialize(profA, 1.f, &err)) return Fail(err.c_str()) ? 0 : 1;

			// Fail first handle removal (Primary)
			mockOps.SetFailOnRemoveHandle(enemyRuntime.GetOwnedLoadoutHandles().front());

			EnemyCombatProfile profB;
			profB.profileId = "EnemyCombat.Test.ProfB";
			profB.weapons.push_back({ "Weapon.Projectile.EnemyTwinBladeScatter.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			profB.powerScalingPolicy = EnemyPowerScalingPolicy::Allowed;

			if (enemyRuntime.Initialize(profB, 1.f, &err))
			{
				return Fail("Expected Initialize to fail when first handle cannot be removed") ? 0 : 1;
			}

			// Old loadout remains intact and runtime is still Ready
			if (!enemyRuntime.IsReady() || enemyRuntime.GetCurrentProfile()->profileId != "EnemyCombat.Test.ProfA")
			{
				return Fail("Old profile was not preserved after first handle remove failure") ? 0 : 1;
			}
		}

		std::cout << "[TEST 12]" << std::endl;
		// 12. Faz 1: Middle and last handle removal failure triggering rollback
		{
			World world{ nullptr };
			TestCombatant combatant{ &world };
			MockEnemyAbilityOperations mockOps{ combatant.GetCombatRuntime() };
			EnemyRuntime enemyRuntime{ combatant.GetCombatRuntime(), &mockOps };

			EnemyCombatProfile profA;
			profA.profileId = "EnemyCombat.Test.ProfA3";
			profA.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			profA.powerScalingPolicy = EnemyPowerScalingPolicy::Allowed;
			profA.abilities.push_back({ "Ability.Movement.Dash.Basic", sas::AbilitySlot::Ability1, 1 });
			profA.abilities.push_back({ "Ability.Defense.Shield.Basic", sas::AbilitySlot::Ability2, 1 });
			std::string err;
			if (!enemyRuntime.Initialize(profA, 1.f, &err)) return Fail(err.c_str()) ? 0 : 1;

			// Handles order in oldSpecs: [Primary, Dash, Shield]
			// Middle handle is Dash (2nd remove call)
			mockOps.SetFailOnRemoveIndex(2);

			EnemyCombatProfile profB;
			profB.profileId = "EnemyCombat.Test.ProfB3";
			profB.weapons.push_back({ "Weapon.Projectile.EnemyTwinBladeScatter.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			profB.powerScalingPolicy = EnemyPowerScalingPolicy::Allowed;

			if (enemyRuntime.Initialize(profB, 1.f, &err))
			{
				return Fail("Expected Initialize to fail when middle handle removal fails") ? 0 : 1;
			}

			// Partial removal occurred and rollback restored Primary!
			if (!enemyRuntime.IsReady() || enemyRuntime.GetCurrentProfile()->profileId != "EnemyCombat.Test.ProfA3")
			{
				return Fail("Rollback did not restore old loadout after middle handle removal failure") ? 0 : 1;
			}
			if (enemyRuntime.GetOwnedLoadoutHandles().empty() || !enemyRuntime.GetOwnedLoadoutHandles().front().IsValid())
			{
				return Fail("Restored primary weapon handle is not valid") ? 0 : 1;
			}
		}

		std::cout << "[TEST 13]" << std::endl;
		// 13. Faz 1: Rollback failure transitions runtime to Faulted state
		{
			World world{ nullptr };
			TestCombatant combatant{ &world };
			MockEnemyAbilityOperations mockOps{ combatant.GetCombatRuntime() };
			EnemyRuntime enemyRuntime{ combatant.GetCombatRuntime(), &mockOps };

			EnemyCombatProfile profA;
			profA.profileId = "EnemyCombat.Test.RollbackFailA";
			profA.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			profA.powerScalingPolicy = EnemyPowerScalingPolicy::Allowed;
			profA.abilities.push_back({ "Ability.Movement.Dash.Basic", sas::AbilitySlot::Ability1, 1 });
			std::string err;
			if (!enemyRuntime.Initialize(profA, 1.f, &err)) return Fail(err.c_str()) ? 0 : 1;

			// Fail on 2nd removal (Dash), and also fail on subsequent Grant during restore
			mockOps.SetFailOnRemoveIndex(2);
			mockOps.SetFailOnGrant(true);

			EnemyCombatProfile profB;
			profB.profileId = "EnemyCombat.Test.RollbackFailB";
			profB.weapons.push_back({ "Weapon.Projectile.EnemyTwinBladeScatter.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			profB.powerScalingPolicy = EnemyPowerScalingPolicy::Allowed;

			if (enemyRuntime.Initialize(profB, 1.f, &err))
			{
				return Fail("Expected Initialize to fail when rollback fails") ? 0 : 1;
			}

			if (!enemyRuntime.IsFaulted())
			{
				return Fail("EnemyRuntime did not enter Faulted state after failed rollback") ? 0 : 1;
			}
			if (enemyRuntime.IsSafeEmpty())
			{
				return Fail("Faulted runtime must not be considered safe empty") ? 0 : 1;
			}
			if (enemyRuntime.GetUnremovedHandles().empty())
			{
				return Fail("Faulted runtime did not track unremoved handles") ? 0 : 1;
			}

			std::cout << "[TEST 14]" << std::endl;
		// 14. Faulted runtime rejects new profiles
			if (enemyRuntime.Initialize(profB, 1.f, &err))
			{
				return Fail("Faulted runtime must reject new profile initialization") ? 0 : 1;
			}
		}

		std::cout << "[TEST 15]" << std::endl;
		// 15. Faz 1: External abilities are preserved across all operations
		{
			World world{ nullptr };
			TestCombatant combatant{ &world };
			auto& asc = combatant.GetAbilitySystemComponent();

			// Grant external ability directly
			const GameAbilityDefinition* dashDef = content::AbilityContentCatalog::FindById("Ability.Movement.Dash.Basic");
			if (!dashDef) return Fail("Dash ability not found") ? 0 : 1;
			const sas::AbilityHandle externalHandle = asc.GrantAbility(*dashDef, sas::AbilitySlot::Ability4);
			if (!externalHandle.IsValid()) return Fail("Failed to grant external ability") ? 0 : 1;

			EnemyRuntime enemyRuntime{ combatant.GetCombatRuntime() };
			EnemyCombatProfile prof;
			prof.profileId = "EnemyCombat.Test.External";
			prof.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			prof.powerScalingPolicy = EnemyPowerScalingPolicy::Allowed;
			prof.abilities.push_back({ "Ability.Defense.Shield.Basic", sas::AbilitySlot::Ability1, 1 });

			std::string err;
			if (!enemyRuntime.Initialize(prof, 1.f, &err)) return Fail(err.c_str()) ? 0 : 1;

			if (asc.GetAbility(externalHandle) == nullptr)
			{
				return Fail("External ability was removed during EnemyRuntime Initialize") ? 0 : 1;
			}

			enemyRuntime.Clear();
			if (asc.GetAbility(externalHandle) == nullptr)
			{
				return Fail("External ability was removed during EnemyRuntime Clear") ? 0 : 1;
			}

			EnemyCombatProfile conflictingProfile;
			conflictingProfile.profileId = "EnemyCombat.Test.ExternalIdConflict";
			conflictingProfile.powerScalingPolicy = EnemyPowerScalingPolicy::Allowed;
			conflictingProfile.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			conflictingProfile.abilities.push_back({ "Ability.Movement.Dash.Basic", sas::AbilitySlot::Ability1, 1 });
			if (enemyRuntime.Initialize(conflictingProfile, 1.f, &err) ||
				asc.GetAbility(externalHandle) == nullptr || !enemyRuntime.IsSafeEmpty())
			{
				return Fail("External ability ID conflict was not rejected without adopting the external handle") ? 0 : 1;
			}
		}

		std::cout << "[TEST 15A]" << std::endl;
		// 15A. External ID conflicts are rejected before the old loadout is removed.
		{
			World world{ nullptr };
			TestCombatant combatant{ &world };
			auto& asc = combatant.GetAbilitySystemComponent();
			const GameAbilityDefinition* externalDefinition = content::AbilityContentCatalog::FindById("Ability.Movement.Dash.Basic");
			if (!externalDefinition) return Fail("External ID preflight ability was not found") ? 0 : 1;
			const sas::AbilityHandle externalHandle = asc.GrantAbility(*externalDefinition, sas::AbilitySlot::Ability4);
			if (!externalHandle.IsValid()) return Fail("Failed to grant external ID preflight ability") ? 0 : 1;

			EnemyRuntime enemyRuntime{ combatant.GetCombatRuntime() };
			EnemyCombatProfile oldProfile{ "EnemyCombat.Test.ExternalIdPreflightOld", {}, {}, EnemyPowerScalingPolicy::Allowed, false };
			oldProfile.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			std::string error;
			if (!enemyRuntime.Initialize(oldProfile, 2.f, &error)) return Fail(error.c_str()) ? 0 : 1;
			const sas::AbilityHandle oldHandle = enemyRuntime.GetOwnedLoadoutHandles().front();

			EnemyCombatProfile conflictingProfile{ "EnemyCombat.Test.ExternalIdPreflightNew", {}, {}, EnemyPowerScalingPolicy::Allowed, false };
			conflictingProfile.weapons.push_back({ "Weapon.Projectile.EnemyTwinBladeScatter.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			conflictingProfile.abilities.push_back({ "Ability.Movement.Dash.Basic", sas::AbilitySlot::Ability1, 1 });
			if (enemyRuntime.Initialize(conflictingProfile, 5.f, &error) ||
				enemyRuntime.GetCurrentProfile()->profileId != oldProfile.profileId ||
				asc.GetAbility(oldHandle) == nullptr ||
				asc.GetAbility(externalHandle) == nullptr)
			{
				return Fail("External ID conflict was not rejected before old loadout mutation") ? 0 : 1;
			}
		}

		std::cout << "[TEST 15B]" << std::endl;
		// 15B. A later grant failure removes all new handles and restores the old loadout.
		{
			World world{ nullptr };
			TestCombatant combatant{ &world };
			auto& asc = combatant.GetAbilitySystemComponent();
			const GameAbilityDefinition* externalDefinition = content::AbilityContentCatalog::FindById("Ability.Defense.Shield.Basic");
			if (!externalDefinition) return Fail("External ability definition was not found") ? 0 : 1;
			const sas::AbilityHandle externalHandle = asc.GrantAbility(*externalDefinition, sas::AbilitySlot::Ability4);
			if (!externalHandle.IsValid()) return Fail("Failed to grant external ability for rollback test") ? 0 : 1;
			combatant.GetCombatRuntime().SetRuntimeModifier("External.Test", CombatRuntimeModifier{ 1.5f });

			MockEnemyAbilityOperations mockOps{ combatant.GetCombatRuntime() };
			EnemyRuntime enemyRuntime{ combatant.GetCombatRuntime(), &mockOps };
			EnemyCombatProfile oldProfile{ "EnemyCombat.Test.GrantRollbackOld", {}, {}, EnemyPowerScalingPolicy::Allowed, false };
			oldProfile.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			oldProfile.abilities.push_back({ "Ability.Movement.Dash.Basic", sas::AbilitySlot::Ability1, 1 });
			std::string initError;
			if (!enemyRuntime.Initialize(oldProfile, 2.f, &initError)) return Fail(initError.c_str()) ? 0 : 1;

			EnemyCombatProfile newProfile{ "EnemyCombat.Test.GrantRollbackNew", {}, {}, EnemyPowerScalingPolicy::Allowed, false };
			newProfile.weapons.push_back({ "Weapon.Projectile.EnemyTwinBladeScatter.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			newProfile.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::Ability2, 1 });
			mockOps.SetFailOnGrantIndex(2);
			if (enemyRuntime.Initialize(newProfile, 5.f, &initError)) return Fail("Second grant failure unexpectedly committed a new loadout") ? 0 : 1;
			if (!enemyRuntime.IsReady() || !enemyRuntime.GetCurrentProfile() ||
				enemyRuntime.GetCurrentProfile()->profileId != oldProfile.profileId ||
				enemyRuntime.GetOwnedLoadoutHandles().size() != 2)
			{
				return Fail("Second grant failure did not restore the previous loadout") ? 0 : 1;
			}
			if (asc.GetAbility(externalHandle) == nullptr ||
				asc.GetAbility(sas::AbilitySlot::Ability2) != nullptr ||
				!NearlyEqual(combatant.GetCombatRuntime().GetOutgoingDamageMultiplier(), 3.f))
			{
				return Fail("Grant rollback did not preserve external state or remove new state") ? 0 : 1;
			}
		}

		std::cout << "[TEST 16]" << std::endl;
		// 16. Faz 3: Failed profile enemy destroys actor on BeginPlay
		{
			World world{ nullptr };
			const ShipDefinition* ship = content::ShipContentCatalog::FindById(
				"Ship.Enemy.ApproachGunner.Basic"
			);
			if (!ship) return Fail("Failed to find test enemy ship definition") ? 0 : 1;
			EnemyCombatProfile invalidProfile;
			invalidProfile.profileId = "EnemyCombat.DoesNotExist.Invalid";
			EnemyBehaviorProfile behaviorProfile;
			behaviorProfile.profileId = "EnemyBehavior.Test.Basic";
			behaviorProfile.targetSearchRange = 1000.f;
			behaviorProfile.targetRefreshInterval = 0.2f;
			behaviorProfile.desiredDistance = 300.f;
			behaviorProfile.strafeDirectionChangeInterval = 1.f;
			std::shared_ptr<EnemyActor> failedEnemy = world.SpawnActor<EnemyActor>(
				*ship,
				"Enemy.Test.Invalid",
				invalidProfile,
				behaviorProfile
				).lock();
			failedEnemy->BeginPlay();

			if (!failedEnemy->GetIsPendingDestroy())
			{
				return Fail("Failed enemy actor was not marked destroyed after failed BeginPlay initialization") ? 0 : 1;
			}
			if (failedEnemy->GetEnemyRuntime().IsReady())
			{
				return Fail("Failed enemy actor runtime must not be Ready") ? 0 : 1;
			}
		}

		std::cout << "[TEST 16B]" << std::endl;
		// 16B. Behavior and contact-only initialization failures roll back combat state.
		{
			World world{ nullptr };
			const ShipDefinition* ship = content::ShipContentCatalog::FindById("Ship.Enemy.ApproachGunner.Basic");
			const EnemyCombatProfile* combatProfile = content::EnemyCombatProfileCatalog::FindById("EnemyCombat.ApproachGunner.Basic");
			if (!ship || !combatProfile) return Fail("Failed to find rollback test enemy content") ? 0 : 1;
			EnemyBehaviorProfile invalidBehavior{ "EnemyBehavior.Test.Invalid", 1000.f, 0.2f, 300.f, 0.f, 1.f, EnemyMovementMode::Approach, 1.f, 1.f };
			const shared_ptr<EnemyActor> behaviorFailed = world.SpawnActor<EnemyActor>(*ship, "Enemy.Test.InvalidBehavior", *combatProfile, invalidBehavior).lock();
			behaviorFailed->BeginPlay();
			if (!behaviorFailed->GetIsPendingDestroy() || !behaviorFailed->GetEnemyRuntime().IsEmpty())
				return Fail("Behavior initialization failure did not roll back combat runtime") ? 0 : 1;

			ShipDefinition zeroCollisionShip = *ship;
			zeroCollisionShip.collisionDamage = 0.f;
			EnemyCombatProfile contactOnly{ "EnemyCombat.Test.ContactOnly", {}, {}, EnemyPowerScalingPolicy::Disabled, true };
			EnemyBehaviorProfile validBehavior{ "EnemyBehavior.Test.ContactOnly", 1000.f, 0.2f, 300.f, 0.f, 0.f, EnemyMovementMode::Approach, 1.f, 1.f };
			const shared_ptr<EnemyActor> contactFailed = world.SpawnActor<EnemyActor>(zeroCollisionShip, "Enemy.Test.ContactOnly", contactOnly, validBehavior).lock();
			contactFailed->BeginPlay();
			if (!contactFailed->GetIsPendingDestroy() || !contactFailed->GetEnemyRuntime().IsEmpty())
				return Fail("Contact-only validation failure did not roll back combat runtime") ? 0 : 1;
		}

		std::cout << "[TEST 17]" << std::endl;
		// 17. Faz 3: Unready enemy does not shoot or apply contact damage
		{
			World world{ nullptr };
			const ShipDefinition* ship = content::ShipContentCatalog::FindById(
				"Ship.Enemy.ApproachGunner.Basic"
			);
			if (!ship) return Fail("Failed to find test enemy ship definition") ? 0 : 1;
			EnemyCombatProfile invalidProfile;
			invalidProfile.profileId = "EnemyCombat.DoesNotExist.Invalid";
			EnemyBehaviorProfile behaviorProfile;
			behaviorProfile.profileId = "EnemyBehavior.Test.Basic";
			behaviorProfile.targetSearchRange = 1000.f;
			behaviorProfile.targetRefreshInterval = 0.2f;
			behaviorProfile.desiredDistance = 300.f;
			behaviorProfile.strafeDirectionChangeInterval = 1.f;
			std::shared_ptr<EnemyActor> failedEnemy = world.SpawnActor<EnemyActor>(
				*ship,
				"Enemy.Test.Invalid",
				invalidProfile,
				behaviorProfile
			).lock();
			failedEnemy->BeginPlay();

			// Overlap with player should not deal contact damage
			DummyEnemy playerShip{ &world, *ship };
			playerShip.SetCollisionLayer(CollisionLayer::Player);
			const float healthBefore = playerShip.GetHealthComponent().GetHealth();

			static_cast<Actor*>(failedEnemy.get())->OnActorBeginOverlap(&playerShip);
			if (playerShip.GetHealthComponent().GetHealth() != healthBefore)
			{
				return Fail("Failed enemy applied contact damage despite unready state") ? 0 : 1;
			}
		}

		std::cout << "[TEST 17A]" << std::endl;
		// Enemy spawn context owns progression; neutral variation preserves exact level math.
		{
			World world{ nullptr };
			const ShipDefinition* ship = content::ShipContentCatalog::FindById("Ship.Enemy.ApproachGunner.Basic");
			if (!ship) return Fail("Progression test ship was not found") ? 0 : 1;
			EnemyCombatProfile progressionProfile;
			progressionProfile.profileId = "EnemyCombat.Test.Progression";
			progressionProfile.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
			progressionProfile.progression.naturalGrowth = { { OwnerAttributeIds::MaxHealth, 12.f }, { OwnerAttributeIds::Armor, 1.f } };
			progressionProfile.progression.maxShieldPerLevel = 2.f;
			progressionProfile.progression.outgoingDamagePerLevel = 0.04f;
			EnemyBehaviorProfile behavior{ "EnemyBehavior.Test.Progression", 1000.f, 0.2f, 300.f, 0.f, 0.f, EnemyMovementMode::Approach, 1.f, 1.f };
			EnemyCombatProfile shieldVariationOnly = progressionProfile;
			shieldVariationOnly.progression.naturalGrowth.clear();
			shieldVariationOnly.progression.maxShieldPerLevel = 0.f;
			shieldVariationOnly.progression.maxShieldVariation = { 0.9f, 1.1f };
			TestCombatant loadoutOnlyCombatant{ &world };
			EnemyRuntime loadoutOnlyRuntime{ loadoutOnlyCombatant.GetCombatRuntime() };
			if (loadoutOnlyRuntime.Initialize(shieldVariationOnly, EnemySpawnContext{ 1, 42 }, 1.f))
				return Fail("ShipRuntime-less enemy accepted effective shield variation") ? 0 : 1;
			const shared_ptr<EnemyActor> enemy = world.SpawnActor<EnemyActor>(*ship, "Enemy.Test.Progression", progressionProfile, behavior, 1.5f, EnemySpawnContext{ 3, 0 }).lock();
			enemy->BeginPlay();
			const auto& attributes = enemy->GetCombatRuntime().GetAbilitySystemComponent().GetAttributes();
			if (!enemy->GetEnemyRuntime().IsReady() || !NearlyEqual(attributes.GetCurrentValue(OwnerAttributeIds::MaxHealth), 84.f) ||
				!NearlyEqual(attributes.GetCurrentValue(OwnerAttributeIds::Armor), 2.f) ||
				!NearlyEqual(enemy->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::MaxShield), 29.f) ||
				!NearlyEqual(enemy->GetCombatRuntime().GetOutgoingDamageMultiplier(), 1.62f))
			{
				return Fail("Enemy progression did not apply level-based owner, shield, and outgoing damage values") ? 0 : 1;
			}
			if (!enemy->GetEnemyRuntime().Clear()) return Fail("Enemy progression Clear reported a failure") ? 0 : 1;
			if (!enemy->GetEnemyRuntime().IsSafeEmpty() ||
				!NearlyEqual(attributes.GetCurrentValue(OwnerAttributeIds::MaxHealth), 60.f) ||
				!NearlyEqual(attributes.GetCurrentValue(OwnerAttributeIds::Armor), 0.f) ||
				!NearlyEqual(enemy->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::MaxShield), 25.f) ||
				!NearlyEqual(enemy->GetCombatRuntime().GetOutgoingDamageMultiplier(), 1.f))
			{
				return Fail("Enemy progression Clear did not remove only enemy-owned state") ? 0 : 1;
			}
		}

		std::cout << "[TEST 18]" << std::endl;
		// 18. Faz 4: CombatRuntimeModifiers numerical robustness tests
		{
			CombatRuntimeModifiers modifiers;

			// Big * Big overflow
			modifiers.Set("Huge1", 1e25f);
			modifiers.Set("Huge2", 1e25f);
			if (modifiers.GetOutgoingDamageMultiplier() != std::numeric_limits<float>::max())
			{
				return Fail("Big * Big did not clamp to float max") ? 0 : 1;
			}

			// Big * Small back to normal
			modifiers.Clear();
			modifiers.Set("Big", 1e25f);
			modifiers.Set("Small", 1e-20f);
			if (!NearlyEqual(modifiers.GetOutgoingDamageMultiplier(), 100000.0f, 100.0f))
			{
				return Fail("Big * Small did not return to normal range correctly") ? 0 : 1;
			}

			// Zero * Overflow = Zero (Zero dominance)
			modifiers.Clear();
			modifiers.Set("Huge", 1e30f);
			modifiers.Set("Zero", 0.0f);
			if (modifiers.GetOutgoingDamageMultiplier() != 0.0f)
			{
				return Fail("Zero dominance failed when combined with huge multiplier") ? 0 : 1;
			}

			// Permutations: Insertion order independence
			CombatRuntimeModifiers modA;
			modA.Set("A", 1.25f);
			modA.Set("B", 0.75f);
			modA.Set("C", 2.50f);

			CombatRuntimeModifiers modB;
			modB.Set("C", 2.50f);
			modB.Set("A", 1.25f);
			modB.Set("B", 0.75f);

			if (modA.GetOutgoingDamageMultiplier() != modB.GetOutgoingDamageMultiplier())
			{
				return Fail("CombatRuntimeModifiers result differed across insertion permutations") ? 0 : 1;
			}

			// Invalid update returns false and preserves old value
			if (modA.Set("A", -1.0f))
			{
				return Fail("Negative modifier update should have returned false") ? 0 : 1;
			}
			if (modA.GetOutgoingDamageMultiplier() != modB.GetOutgoingDamageMultiplier())
			{
				return Fail("Invalid modifier update corrupted previous valid modifier") ? 0 : 1;
			}
		}

		std::cout << "[PASS] All Enemy Combat Foundation runtime contract tests passed successfully!" << std::endl;
		return 0;
	}
}
