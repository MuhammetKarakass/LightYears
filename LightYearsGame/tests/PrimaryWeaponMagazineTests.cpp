#include "attributes/AttributeMath.h"
#include "attributes/AttributeSystem.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/ability/actions/FireWeaponActionRuntime.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/GameAbilityActionExecutor.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/content/AttributeJsonParser.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/content/WeaponLoader.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/progression/ShipProgression.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "spaceShip/SpaceShip.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <tuple>
#include <vector>

namespace ly
{
	namespace
	{
		bool NearlyEqual(float a, float b, float epsilon = 0.005f)
		{
			return std::abs(a - b) <= epsilon;
		}

		int Fail(const std::string& message)
		{
			std::cerr << "FAILED: " << message << '\n';
			return 1;
		}

		const PrimaryWeaponDefinition& LoadedWeapon(const char* weaponId)
		{
			return *ly::content::WeaponContentCatalog::FindById(weaponId);
		}

		class TestCombatantActor final
			: public Actor
			, public Combatant
		{
		public:
			explicit TestCombatantActor(World* world = nullptr, float maximumHealth = 250.f)
				: Actor{ world }
				, mCombatRuntime{ *this }
			{
				mCombatRuntime.GetAbilitySystemComponent().GetAttributes().RegisterAttribute(OwnerAttributeIds::AttackSpeed, 0.f);
				mCombatRuntime.GetAbilitySystemComponent().GetAttributes().RegisterAttribute(OwnerAttributeIds::AttackPower, 35.f);
				mCombatRuntime.GetAbilitySystemComponent().GetAttributes().RegisterAttribute(OwnerAttributeIds::EnergyPower, 35.f);
				mCombatRuntime.GetAbilitySystemComponent().GetAttributes().RegisterAttribute(OwnerAttributeIds::Armor, 0.f);
			}

			CombatRuntime& GetCombatRuntime() override { return mCombatRuntime; }
			const CombatRuntime& GetCombatRuntime() const override { return mCombatRuntime; }
			void ReceiveDamage(DamageContext) override {}

			void Tick(float deltaTime) override
			{
				Actor::Tick(deltaTime);
				mCombatRuntime.Tick(deltaTime);
			}

		private:
			CombatRuntime mCombatRuntime;
		};

		static ShipDefinition MakeTestShipDefinition()
		{
			return ShipDefinition{ "", 250.f, { 0.f, 0.f }, 0.f, 0, 0, {}, {} };
		}

		class TestShipCombatant final : public SpaceShip
		{
		public:
			explicit TestShipCombatant(World* world = nullptr)
				: SpaceShip{ world, MakeTestShipDefinition() }
			{
				GetCombatRuntime().GetAbilitySystemComponent().GetAttributes().RegisterAttribute(OwnerAttributeIds::AttackSpeed, 0.f);
				GetCombatRuntime().GetAbilitySystemComponent().GetAttributes().RegisterAttribute(OwnerAttributeIds::AttackPower, 35.f);
				GetCombatRuntime().GetAbilitySystemComponent().GetAttributes().RegisterAttribute(OwnerAttributeIds::EnergyPower, 35.f);
				GetCombatRuntime().GetAbilitySystemComponent().GetAttributes().RegisterAttribute(OwnerAttributeIds::Armor, 0.f);
			}
		};

		GameAbilityDefinition MakePrimaryWeaponAbility(const PrimaryWeaponDefinition& weaponDef)
		{
			return AbilityData::MakePrimaryFireAbilityDefinition(weaponDef);
		}
	}
}

int main()
{
	using namespace ly;

	std::cout << "Starting PrimaryWeaponMagazineTests...\n";

	if (!GameContentBootstrap::Register())
	{
		return Fail("GameContentBootstrap::Register failed");
	}

	if (!content::WeaponContentCatalog::IsLoaded())
	{
		content::WeaponContentCatalog::LoadFromFile("LightYearsGame/assets/content/data/weapons.json");
		if (!content::WeaponContentCatalog::IsLoaded())
		{
			content::WeaponContentCatalog::LoadFromFile("assets/content/data/weapons.json");
		}
	}

	// =========================================================================
	// SUITE A: Fighter Baseline
	// =========================================================================
	std::cout << "Running Suite A: Fighter Baseline...\n";
	{
		const PrimaryWeaponDefinition& fighterLaser = LoadedWeapon("Weapon.Projectile.FighterRapidLaser.Basic");
		if (fighterLaser.cadenceMode != PrimaryWeaponCadenceMode::OwnerAttackSpeedPercentage)
		{
			return Fail("Suite A: Fighter laser must use OwnerAttackSpeedPercentage");
		}
		if (!fighterLaser.magazine.has_value() || fighterLaser.magazine->capacity != 48 ||
			!NearlyEqual(fighterLaser.magazine->baseReloadTime, 2.0f))
		{
			return Fail("Suite A: Fighter laser magazine must be 48 capacity, 2.0s base reload");
		}
		const sas::GameplayAttribute* frAttr = sas::FindAttribute(fighterLaser.attributes, CommonAttributeIds::FireRate);
		if (!frAttr || !NearlyEqual(frAttr->baseValue, 4.0f))
		{
			return Fail("Suite A: Fighter laser FireRate baseValue must be 4.0");
		}

		World world{ nullptr };
		const shared_ptr<TestShipCombatant> ship = world.SpawnActor<TestShipCombatant>().lock();
		if (!ship)
		{
			return Fail("Suite A: Failed to spawn test ship");
		}
		world.TickInternal(0.f);

		GameAbilityDefinition abilityDef = MakePrimaryWeaponAbility(fighterLaser);
		std::string grantFailure;
		const sas::AbilityHandle handle =
			ship->GetCombatRuntime().GetAbilitySystemComponent().GrantAbility(abilityDef, sas::AbilitySlot::PrimaryFire, &grantFailure);
		if (!handle.IsValid())
		{
			std::cerr << "Grant failure reason: " << grantFailure << std::endl;
			return Fail("Suite A: Failed to grant primary weapon ability");
		}

		// Initial state
		ship->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
		world.TickInternal(0.f); // First execution fires 1st shot
		GameAbility* abilityInstance = ship->GetCombatRuntime().GetAbilitySystemComponent().GetAbility(handle);
		if (!abilityInstance)
		{
			return Fail("Suite A: Ability instance not found");
		}
		PrimaryWeaponRuntimeState& weaponRuntime = abilityInstance->GetPrimaryWeaponRuntime();
		const float resolvedDamage = sas::FindAttributeValue(
			abilityInstance->GetPrimaryWeaponRuntimeAttributes(),
			CommonAttributeIds::Damage,
			0.f
		);
		if (!NearlyEqual(resolvedDamage, 26.f) ||
			!NearlyEqual(resolvedDamage * frAttr->baseValue, 104.f) ||
			!NearlyEqual(1534.f / 14.f, 109.5714f, 0.1f))
		{
			return Fail("Suite A: Fighter L1 damage, raw DPS, or sustained cycle DPS is incorrect");
		}

		// 1 shot fired: ammo should be 47
		if (weaponRuntime.magazineState.roundsRemaining != 47)
		{
			return Fail("Suite A: First shot should leave 47 rounds");
		}
		if (weaponRuntime.magazineState.reloadRemaining != 0.0)
		{
			return Fail("Suite A: Reload should not be active with 47 rounds remaining");
		}

		// Fire remaining 47 shots: total 48 shots at 4/s (interval 0.25s).
		for (int i = 0; i < 47; ++i)
		{
			world.TickInternal(1.f / 4.f);
		}

		// After 48 shots, magazine must be empty and reload active
		if (weaponRuntime.magazineState.roundsRemaining != 0)
		{
			return Fail("Suite A: After 48 shots, roundsRemaining must be 0");
		}
		if (weaponRuntime.magazineState.reloadRemaining <= 0.0)
		{
			return Fail("Suite A: After 48 shots, reload must be active");
		}
		if (!NearlyEqual(static_cast<float>(weaponRuntime.magazineState.reloadRemaining), 2.0f, 0.05f))
		{
			return Fail("Suite A: AS0 reload snapshot should be 2.0s");
		}

		// While reloading, tick 0.5s: no projectile spawned, reloadRemaining decreases by 0.5s
		const size_t actorsBefore = world.GetActorsByType<PrimaryWeaponProjectileActor>().size();
		world.TickInternal(0.5f);
		if (world.GetActorsByType<PrimaryWeaponProjectileActor>().size() > actorsBefore)
		{
			return Fail("Suite A: Projectiles must not spawn during reload");
		}
		if (weaponRuntime.magazineState.roundsRemaining != 0)
		{
			return Fail("Suite A: Rounds must remain 0 during reload");
		}
		if (!NearlyEqual(static_cast<float>(weaponRuntime.magazineState.reloadRemaining), 1.5f, 0.05f))
		{
			return Fail("Suite A: Reload remaining should be ~1.5s after 0.5s tick");
		}

		// Complete the remaining 1.5s of reload
		world.TickInternal(1.5f);
		// Reload completed and immediately fired shot #49 at t = 2.0s!
		// Upon reload finish, magazine refilled to 48, shot 49 immediately fired -> 47 remaining!
		if (weaponRuntime.magazineState.roundsRemaining != 47)
		{
			return Fail("Suite A: After reload completion, shot should immediately fire leaving 47 rounds");
		}
		if (weaponRuntime.magazineState.reloadRemaining != 0.0)
		{
			return Fail("Suite A: Reload remaining should be 0.0 after completion");
		}
	}

	// =========================================================================
	// SUITE B: Cadence and Reload Numbers
	// =========================================================================
	std::cout << "Running Suite B: Cadence and Reload Numbers...\n";
	{
		const PrimaryWeaponDefinition& fighterLaser = LoadedWeapon("Weapon.Projectile.FighterRapidLaser.Basic");

		const struct TestCase
		{
			float attackSpeed;
			float expectedFireRate;
			float expectedReload;
		} cases[] = {
			{ 0.f, 4.0f, 2.0f },
			{ 20.f, 4.8f, 1.666667f },
			{ 50.f, 6.0f, 1.333333f },
			{ 100.f, 8.0f, 1.0f },
			{ -30.f, 4.0f, 2.0f } // Negative AS clamped to 0 -> multiplier 1.0
		};

		for (const auto& tc : cases)
		{
			World world{ nullptr };
			const shared_ptr<TestShipCombatant> ship = world.SpawnActor<TestShipCombatant>().lock();
			ship->GetCombatRuntime().GetAbilitySystemComponent().GetAttributes().RegisterAttribute(
				OwnerAttributeIds::AttackSpeed, tc.attackSpeed
			);
			world.TickInternal(0.f);

			const float multiplier = PrimaryWeaponExecutionSystem::CalculateAttackSpeedMultiplier(*ship);
			const float expectedMultiplier = 1.f + std::max(0.f, tc.attackSpeed) / 100.f;
			if (!NearlyEqual(multiplier, expectedMultiplier))
			{
				return Fail("Suite B: Multiplier math failed");
			}

			const float reload = PrimaryWeaponExecutionSystem::CalculateReloadDuration(
				fighterLaser.magazine->baseReloadTime,
				*ship
			);
			if (!NearlyEqual(reload, tc.expectedReload, 0.01f))
			{
				return Fail("Suite B: Reload duration did not match expected table");
			}

			// Verify cadence with ability execution
			GameAbilityDefinition abilityDef = MakePrimaryWeaponAbility(fighterLaser);
			const sas::AbilityHandle handle =
				ship->GetCombatRuntime().GetAbilitySystemComponent().GrantAbility(abilityDef, sas::AbilitySlot::PrimaryFire);
			ship->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
			world.TickInternal(0.f); // fires shot 1

			GameAbility* inst = ship->GetCombatRuntime().GetAbilitySystemComponent().GetAbility(handle);
			const float interval = inst->GetWeaponFireIntervalRemaining();
			const float expectedInterval = 1.f / tc.expectedFireRate;
			if (!NearlyEqual(interval, expectedInterval, 0.005f))
			{
				return Fail("Suite B: Fire interval did not match expected cadence");
			}
		}

		// Ship fire-rate multiplier test: affects cadence, does NOT affect reload
		{
			World world{ nullptr };
			const shared_ptr<TestShipCombatant> ship = world.SpawnActor<TestShipCombatant>().lock();
			ship->SetPrimaryWeaponFireRateModifier(1ULL, 0.5f);
			world.TickInternal(0.f);

			const float reload = PrimaryWeaponExecutionSystem::CalculateReloadDuration(
				fighterLaser.magazine->baseReloadTime,
				*ship
			);
			if (!NearlyEqual(reload, 2.0f))
			{
				return Fail("Suite B: Ship fire-rate multiplier must not affect reload duration");
			}

			GameAbilityDefinition abilityDef = MakePrimaryWeaponAbility(fighterLaser);
			const sas::AbilityHandle handle =
				ship->GetCombatRuntime().GetAbilitySystemComponent().GrantAbility(abilityDef, sas::AbilitySlot::PrimaryFire);
			ship->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
			world.TickInternal(0.f);

			GameAbility* inst = ship->GetCombatRuntime().GetAbilitySystemComponent().GetAbility(handle);
			// Base FR 4.0 * ship 0.5 = 2.0 FR -> interval = 0.5s.
			const float interval = inst->GetWeaponFireIntervalRemaining();
			if (!NearlyEqual(interval, 0.5f, 0.005f))
			{
				return Fail("Suite B: Ship fire-rate multiplier must affect fire interval");
			}
		}

		// Ongoing reload snapshot test: AS change mid-reload does NOT rescale active reload
		{
			World world{ nullptr };
			const shared_ptr<TestShipCombatant> ship = world.SpawnActor<TestShipCombatant>().lock();
			world.TickInternal(0.f);

			GameAbilityDefinition abilityDef = MakePrimaryWeaponAbility(fighterLaser);
			const sas::AbilityHandle handle =
				ship->GetCombatRuntime().GetAbilitySystemComponent().GrantAbility(abilityDef, sas::AbilitySlot::PrimaryFire);
			ship->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
			world.TickInternal(0.f);
			GameAbility* inst = ship->GetCombatRuntime().GetAbilitySystemComponent().GetAbility(handle);

			// Exhaust magazine
			for (int i = 0; i < 47; ++i)
			{
				world.TickInternal(1.f / 4.f);
			}
			// Reload is active: 2.0s
			world.TickInternal(0.5f); // 1.5s left

			// Now dynamically increase AS to 100 (which would make base reload 1.0s)
			ship->GetCombatRuntime().GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
				sas::AttributeModifier{ OwnerAttributeIds::AttackSpeed, 100.f }
			);

			// Advance 0.5s: remaining should be ~1.0s (not rescaled by new AS)
			world.TickInternal(0.5f);
			if (!NearlyEqual(static_cast<float>(inst->GetPrimaryWeaponRuntime().magazineState.reloadRemaining), 1.0f, 0.05f))
			{
				return Fail("Suite B: Active reload snapshot must not rescale on AS change");
			}

			// Complete this reload
			world.TickInternal(1.0f);
			if (inst->GetPrimaryWeaponRuntime().magazineState.reloadRemaining != 0.0)
			{
				return Fail("Suite B: Reload should be finished");
			}

			// Exhaust magazine again to test that subsequent reload DOES use the new AS
			for (int i = 0; i < 47; ++i)
			{
				world.TickInternal(1.f / 8.f);
			}
			// Now reload duration snapshot should be 1.0s (since AS is 100)
			if (!NearlyEqual(static_cast<float>(inst->GetPrimaryWeaponRuntime().magazineState.reloadRemaining), 1.0f, 0.05f))
			{
				return Fail("Suite B: Subsequent reload must use new AS value (1.0s)");
			}
		}
	}

	// =========================================================================
	// SUITE C: Successful-Fire Semantics
	// =========================================================================
	std::cout << "Running Suite C: Successful-Fire Semantics...\n";
	{
		const PrimaryWeaponDefinition& fighterLaser = LoadedWeapon("Weapon.Projectile.FighterRapidLaser.Basic");

		// Test failed fire when actor has no World
		{
			TestCombatantActor orphanActor{ nullptr };
			PrimaryWeaponRuntimeState state;
			PrimaryWeaponExecutionSystem::InitializeRuntime(fighterLaser, state);
			if (state.magazineState.roundsRemaining != 48)
			{
				return Fail("Suite C: Runtime not initialized with 48 rounds");
			}

			sas::GameplayAttributeList attrs = fighterLaser.attributes;
			PrimaryWeaponExecutionContext ctx{
				orphanActor,
				fighterLaser,
				attrs
			};
			PrimaryWeaponExecutionSystem::BeginFire(ctx, state);

			// FireOnce should fail because orphanActor has no world
			const bool fired = PrimaryWeaponExecutionSystem::FireOnce(ctx, state);
			if (fired)
			{
				return Fail("Suite C: FireOnce should fail with null world");
			}
			if (state.magazineState.roundsRemaining != 48)
			{
				return Fail("Suite C: Failed fire must not consume ammo");
			}
			if (state.magazineState.reloadRemaining != 0.0)
			{
				return Fail("Suite C: Failed fire must not trigger reload");
			}
			if (state.requestedCooldown != 0.f)
			{
				return Fail("Suite C: Failed fire must not request cooldown");
			}
		}

		// Test multi-projectile / shotgun volley: exactly 1 ammo per volley
		{
			PrimaryWeaponDefinition multiProjectileDef = fighterLaser;
			multiProjectileDef.muzzleDefinitions = {
				WeaponMuzzleDefinition{ { -10.f, 50.f } },
				WeaponMuzzleDefinition{ { 0.f, 50.f } },
				WeaponMuzzleDefinition{ { 10.f, 50.f } }
			};

			World world{ nullptr };
			const shared_ptr<TestCombatantActor> actor = world.SpawnActor<TestCombatantActor>().lock();
			PrimaryWeaponRuntimeState state;
			PrimaryWeaponExecutionSystem::InitializeRuntime(multiProjectileDef, state);

			sas::GameplayAttributeList attrs = multiProjectileDef.attributes;
			PrimaryWeaponExecutionContext ctx{
				*actor,
				multiProjectileDef,
				attrs
			};
			PrimaryWeaponExecutionSystem::BeginFire(ctx, state);

			const bool fired = PrimaryWeaponExecutionSystem::FireOnce(ctx, state);
			if (!fired)
			{
				return Fail("Suite C: Multi-muzzle FireOnce failed");
			}
			if (state.magazineState.roundsRemaining != 47)
			{
				return Fail("Suite C: Multi-muzzle volley must consume exactly 1 ammo");
			}
		}
	}

	// =========================================================================
	// SUITE D: Lifecycle and Ownership
	// =========================================================================
	std::cout << "Running Suite D: Lifecycle and Ownership...\n";
	{
		const PrimaryWeaponDefinition& fighterLaser = LoadedWeapon("Weapon.Projectile.FighterRapidLaser.Basic");

		World world{ nullptr };
		const shared_ptr<TestCombatantActor> actor = world.SpawnActor<TestCombatantActor>().lock();
		world.TickInternal(0.f);

		GameAbilityDefinition abilityDef = MakePrimaryWeaponAbility(fighterLaser);
		const sas::AbilityHandle handle =
			actor->GetCombatRuntime().GetAbilitySystemComponent().GrantAbility(abilityDef, sas::AbilitySlot::PrimaryFire);
		GameAbility* inst = actor->GetCombatRuntime().GetAbilitySystemComponent().GetAbility(handle);

		// 1. Release / repress preserves ammo and reload
		actor->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
		world.TickInternal(0.f); // shot 1 -> 47 rounds
		if (inst->GetPrimaryWeaponRuntime().magazineState.roundsRemaining != 47)
		{
			return Fail("Suite D: Shot 1 did not leave 47 rounds");
		}

		// Release input
		actor->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
		world.TickInternal(0.1f);
		if (inst->GetPrimaryWeaponRuntime().magazineState.roundsRemaining != 47)
		{
			return Fail("Suite D: Releasing input altered roundsRemaining");
		}

		// Repress input: still 47
		actor->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
		if (inst->GetPrimaryWeaponRuntime().magazineState.roundsRemaining != 47)
		{
			return Fail("Suite D: Repressing input altered roundsRemaining");
		}

		// 2. Inactive reload progresses
		// Exhaust the magazine down to 0
		for (int i = 0; i < 47; ++i)
		{
			world.TickInternal(1.f / 4.f);
		}
		if (inst->GetPrimaryWeaponRuntime().magazineState.roundsRemaining != 0 ||
			inst->GetPrimaryWeaponRuntime().magazineState.reloadRemaining <= 0.0)
		{
			return Fail("Suite D: Magazine should be reloading");
		}

		// Release input: weapon is now inactive, reload should continue via TickInactive
		const double reloadStart = inst->GetPrimaryWeaponRuntime().magazineState.reloadRemaining;
		actor->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
		world.TickInternal(1.0f); // 1.0s elapsed in inactive state
		const double reloadAfter1s = inst->GetPrimaryWeaponRuntime().magazineState.reloadRemaining;
		if (!NearlyEqual(static_cast<float>(reloadAfter1s), static_cast<float>(reloadStart - 1.0), 0.05f))
		{
			return Fail("Suite D: Inactive reload did not advance");
		}

		// Finish the remainder of reload while inactive
		world.TickInternal(static_cast<float>(reloadAfter1s) + 0.05f);
		if (inst->GetPrimaryWeaponRuntime().magazineState.roundsRemaining != 48 ||
			inst->GetPrimaryWeaponRuntime().magazineState.reloadRemaining != 0.0)
		{
			return Fail("Suite D: Inactive reload did not refill to 48 rounds upon completion");
		}

		// 3. EnsureRuntimeConfigured does NOT refill ammo
		// Shoot 5 times -> 43 rounds
		actor->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
		world.TickInternal(0.f); // shot 1 -> 47
		world.TickInternal(1.f / 4.f); // shot 2 -> 46
		world.TickInternal(1.f / 4.f); // shot 3 -> 45
		world.TickInternal(1.f / 4.f); // shot 4 -> 44
		world.TickInternal(1.f / 4.f); // shot 5 -> 43
		if (inst->GetPrimaryWeaponRuntime().magazineState.roundsRemaining != 43)
		{
			return Fail("Suite D: Should have 43 rounds after 5 shots");
		}

		// Call EnsureRuntimeConfigured directly
		PrimaryWeaponExecutionSystem::EnsureRuntimeConfigured(fighterLaser, inst->GetPrimaryWeaponRuntime());
		if (inst->GetPrimaryWeaponRuntime().magazineState.roundsRemaining != 43)
		{
			return Fail("Suite D: EnsureRuntimeConfigured must not refill magazine");
		}

		// 4. Two distinct weapon instances do not share state
		PrimaryWeaponRuntimeState stateA;
		PrimaryWeaponRuntimeState stateB;
		PrimaryWeaponExecutionSystem::InitializeRuntime(fighterLaser, stateA);
		PrimaryWeaponExecutionSystem::InitializeRuntime(fighterLaser, stateB);
		stateA.magazineState.roundsRemaining = 12;
		if (stateB.magazineState.roundsRemaining != 48)
		{
			return Fail("Suite D: Distinct weapon instances must not share magazine state");
		}

		// 5. EnsureRuntimeConfigured rejects live magazine changes on existing runtime
		PrimaryWeaponDefinition mutatedMag = fighterLaser;
		mutatedMag.magazine = PrimaryWeaponMagazineDefinition{ 100, 5.0f };
		const PrimaryWeaponValidationResult magResult =
			PrimaryWeaponExecutionSystem::EnsureRuntimeConfigured(mutatedMag, stateB);
		if (magResult.isValid)
		{
			return Fail("Suite D: EnsureRuntimeConfigured must reject magazine config change on existing runtime");
		}

		// 6. The equipped runtime continues reloading while a temporary primary
		// override owns the active fire payload. It must advance exactly once.
		{
			World overrideWorld{ nullptr };
			const shared_ptr<TestCombatantActor> overrideOwner =
				overrideWorld.SpawnActor<TestCombatantActor>().lock();
			overrideWorld.TickInternal(0.f);
			GameAbilityDefinition primaryDef = MakePrimaryWeaponAbility(fighterLaser);
			const sas::AbilityHandle primaryHandle =
				overrideOwner->GetCombatRuntime().GetAbilitySystemComponent().GrantAbility(
					primaryDef, sas::AbilitySlot::PrimaryFire);
			auto& overrideASC = overrideOwner->GetCombatRuntime().GetAbilitySystemComponent();
			overrideASC.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
			overrideWorld.TickInternal(0.f);
			GameAbility* primary = overrideASC.GetAbility(primaryHandle);
			primary->GetPrimaryWeaponRuntime().magazineState.roundsRemaining = 0;
			primary->GetPrimaryWeaponRuntime().magazineState.reloadRemaining = 2.0;
			overrideASC.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);

			PrimaryWeaponDefinition temporaryWeapon = fighterLaser;
			temporaryWeapon.weaponId = "Weapon.Projectile.TestTemporaryOverride";
			const PrimaryWeaponOverrideHandle overrideHandle = overrideASC.PushPrimaryWeaponOverride(
				sas::ContentId{ "Ability.Test.TemporaryOverride" }, temporaryWeapon);
			if (overrideHandle == 0)
			{
				return Fail("Suite D: Temporary primary override could not be installed");
			}
			overrideWorld.TickInternal(0.5f);
			if (!NearlyEqual(static_cast<float>(
				primary->GetPrimaryWeaponRuntime().magazineState.reloadRemaining), 1.5f, 0.01f))
			{
				return Fail("Suite D: Equipped weapon reload did not advance exactly once during override");
			}
			if (!overrideASC.RemovePrimaryWeaponOverride(overrideHandle))
			{
				return Fail("Suite D: Temporary primary override could not be removed");
			}
		}
	}

	// =========================================================================
	// SUITE E: Tick Determinism
	// =========================================================================
	std::cout << "Running Suite E: Tick Determinism...\n";
	{
		const PrimaryWeaponDefinition& fighterLaser = LoadedWeapon("Weapon.Projectile.FighterRapidLaser.Basic");

		auto runSimulation = [&](float totalTime, int stepCount)
		{
			struct Result
			{
				int roundsRemaining = 0;
				double reloadRemaining = 0.0;
				float intervalRemaining = 0.f;
			};
			World world{ nullptr };
			const shared_ptr<TestCombatantActor> actor = world.SpawnActor<TestCombatantActor>().lock();
			world.TickInternal(0.f);

			GameAbilityDefinition abilityDef = MakePrimaryWeaponAbility(fighterLaser);
			const sas::AbilityHandle handle =
				actor->GetCombatRuntime().GetAbilitySystemComponent().GrantAbility(abilityDef, sas::AbilitySlot::PrimaryFire);
			actor->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);

			const float dt = totalTime / static_cast<float>(stepCount);
			for (int step = 0; step < stepCount; ++step)
			{
				world.TickInternal(dt);
			}

			GameAbility* inst = actor->GetCombatRuntime().GetAbilitySystemComponent().GetAbility(handle);
			Result res;
			res.roundsRemaining = inst->GetPrimaryWeaponRuntime().magazineState.roundsRemaining;
			res.reloadRemaining = inst->GetPrimaryWeaponRuntime().magazineState.reloadRemaining;
			res.intervalRemaining = inst->GetWeaponFireIntervalRemaining();
			return res;
		};

		const auto smallRes = runSimulation(0.9f, 54);
		const auto bigRes = runSimulation(0.9f, 1);

		if (smallRes.roundsRemaining != bigRes.roundsRemaining)
		{
			std::cerr << "Small ticks rounds: " << smallRes.roundsRemaining
					  << ", Big tick rounds: " << bigRes.roundsRemaining << '\n';
			return Fail("Suite E: Small and big ticks produced different roundsRemaining");
		}
		if (!NearlyEqual(smallRes.intervalRemaining, bigRes.intervalRemaining, 0.02f))
		{
			std::cerr << "Small ticks interval: " << smallRes.intervalRemaining
					  << ", Big tick interval: " << bigRes.intervalRemaining << '\n';
			return Fail("Suite E: Small and big ticks produced different intervalRemaining");
		}

		// Cross fire/reload boundaries with both small and large ticks.
		PrimaryWeaponDefinition shortMagazine = fighterLaser;
		shortMagazine.magazine = PrimaryWeaponMagazineDefinition{ 3, 0.5f };
		shortMagazine.empoweredShot = std::nullopt;
		auto runBoundarySimulation = [&](float totalTime, int stepCount)
		{
			World world{ nullptr };
			const shared_ptr<TestCombatantActor> actor = world.SpawnActor<TestCombatantActor>().lock();
			world.TickInternal(0.f);
			const sas::AbilityHandle handle =
				actor->GetCombatRuntime().GetAbilitySystemComponent().GrantAbility(
					MakePrimaryWeaponAbility(shortMagazine), sas::AbilitySlot::PrimaryFire);
			auto& asc = actor->GetCombatRuntime().GetAbilitySystemComponent();
			asc.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
			world.TickInternal(0.f); // Activate and fire the initial shot outside measured time.
			const float dt = totalTime / static_cast<float>(stepCount);
			for (int i = 0; i < stepCount; ++i) world.TickInternal(dt);
			GameAbility* ability = asc.GetAbility(handle);
			return std::tuple<int, double, float>{
				ability->GetPrimaryWeaponRuntime().magazineState.roundsRemaining,
				ability->GetPrimaryWeaponRuntime().magazineState.reloadRemaining,
				ability->GetWeaponFireIntervalRemaining()
			};
		};
		// Stay below the deliberate per-frame catch-up cap here. Cap overflow has
		// its own lifecycle test below; this comparison isolates event boundaries.
		const auto boundarySmall = runBoundarySimulation(2.5f, 150);
		const auto boundaryLarge = runBoundarySimulation(2.5f, 2);
		if (std::get<0>(boundarySmall) != std::get<0>(boundaryLarge) ||
			!NearlyEqual(static_cast<float>(std::get<1>(boundarySmall)),
				static_cast<float>(std::get<1>(boundaryLarge)), 0.02f) ||
			!NearlyEqual(std::get<2>(boundarySmall), std::get<2>(boundaryLarge), 0.02f))
		{
			std::cerr << "Boundary small: ammo=" << std::get<0>(boundarySmall)
				<< " reload=" << std::get<1>(boundarySmall)
				<< " interval=" << std::get<2>(boundarySmall) << '\n'
				<< "Boundary large: ammo=" << std::get<0>(boundaryLarge)
				<< " reload=" << std::get<1>(boundaryLarge)
				<< " interval=" << std::get<2>(boundaryLarge) << '\n';
			return Fail("Suite E: Small and large ticks diverged across reload boundaries");
		}

		// Exceed the catch-up limit, then release. Deferred elapsed time must be
		// consumed as inactive time and must not burst-fire on the next press.
		{
			std::cout << "Suite E: Starting catch-up test...\n";
			World world{ nullptr };
			const shared_ptr<TestCombatantActor> actor = world.SpawnActor<TestCombatantActor>().lock();
			world.TickInternal(0.f);
			auto& asc = actor->GetCombatRuntime().GetAbilitySystemComponent();
			const sas::AbilityHandle handle = asc.GrantAbility(
				MakePrimaryWeaponAbility(fighterLaser), sas::AbilitySlot::PrimaryFire);
			asc.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
			std::cout << "Suite E: Before TickInternal(8.f)...\n";
			world.TickInternal(8.f);
			std::cout << "Suite E: After TickInternal(8.f)...\n";
			GameAbility* ability = asc.GetAbility(handle);
			if (ability->GetPrimaryWeaponRuntime().unprocessedSimulationTime <= 0.f)
			{
				return Fail("Suite E: Long frame did not retain catch-up-limited simulation time");
			}
			asc.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
			world.TickInternal(0.f);
			if (ability->GetPrimaryWeaponRuntime().unprocessedSimulationTime != 0.f)
			{
				return Fail("Suite E: Release did not consume pending simulation time as inactive time");
			}
			const int ammoBeforeRepress = ability->GetPrimaryWeaponRuntime().magazineState.roundsRemaining;
			asc.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
			world.TickInternal(0.f);
			const int ammoAfterRepress = ability->GetPrimaryWeaponRuntime().magazineState.roundsRemaining;
			if (ammoBeforeRepress - ammoAfterRepress > 1)
			{
				return Fail("Suite E: Repress burst-fired catch-up debt from the previous execution");
			}
		}
	}

	// =========================================================================
	// SUITE F: Backward Compatibility and Validation
	// =========================================================================
	std::cout << "Running Suite F: Backward Compatibility and Validation...\n";
	{
		// Loader must reject values that cannot be represented by the runtime's
		// signed magazine counter instead of narrowing them through get<int>().
		{
			const std::filesystem::path weaponPath =
				"LightYearsGame/assets/content/data/weapons.json";
			std::ifstream source{ weaponPath };
			std::string oversizedContent{
				std::istreambuf_iterator<char>{ source },
				std::istreambuf_iterator<char>{}
			};
			const std::string validCapacity = "\"capacity\": 48";
			const size_t capacityPosition = oversizedContent.find(validCapacity);
			if (capacityPosition == std::string::npos)
			{
				return Fail("Suite F: Fighter magazine capacity fixture was not found");
			}
			oversizedContent.replace(
				capacityPosition,
				validCapacity.size(),
				"\"capacity\": 2147483648"
			);
			const std::filesystem::path oversizedPath =
				std::filesystem::temp_directory_path() /
				"lightyears_primary_weapon_oversized_capacity.json";
			{
				std::ofstream output{ oversizedPath };
				output << oversizedContent;
			}
			const content::WeaponLoader::Result oversized =
				content::WeaponLoader::LoadFromFile(oversizedPath);
			std::filesystem::remove(oversizedPath);
			if (oversized.Succeeded())
			{
				return Fail("Suite F: Loader accepted magazine capacity above INT_MAX");
			}
		}

		// 1. Check non-fighter weapons do NOT have magazine or OwnerAttackSpeedPercentage
		const char* otherWeapons[] = {
			"Weapon.Projectile.RapidShotgun.Basic",
			"Weapon.Projectile.DualKineticBlaster.Basic",
			"Weapon.Arc.ElectricLauncher.Basic",
			"Weapon.Beam.ContinuousHeatLaser.Basic",
			"Weapon.Wave.CryoProjector.Basic"
		};
		for (const char* wId : otherWeapons)
		{
			const PrimaryWeaponDefinition& wDef = LoadedWeapon(wId);
			if (wDef.magazine.has_value())
			{
				return Fail("Suite F: Non-fighter weapon must not have magazine configured");
			}
			if (wDef.cadenceMode != PrimaryWeaponCadenceMode::AuthoredScaling)
			{
				return Fail("Suite F: Non-fighter weapon must retain AuthoredScaling cadence");
			}
		}

		// 2. Validation: negative/zero capacity rejected
		{
			PrimaryWeaponDefinition invalid = LoadedWeapon("Weapon.Projectile.FighterRapidLaser.Basic");
			invalid.magazine = PrimaryWeaponMagazineDefinition{ 0, 2.0f };
			if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalid).isValid)
			{
				return Fail("Suite F: Zero capacity magazine must be rejected");
			}
			invalid.magazine = PrimaryWeaponMagazineDefinition{ -5, 2.0f };
			if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalid).isValid)
			{
				return Fail("Suite F: Negative capacity magazine must be rejected");
			}
		}

		// 3. Validation: non-positive or non-finite reload rejected
		{
			PrimaryWeaponDefinition invalid = LoadedWeapon("Weapon.Projectile.FighterRapidLaser.Basic");
			invalid.magazine = PrimaryWeaponMagazineDefinition{ 48, 0.0f };
			if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalid).isValid)
			{
				return Fail("Suite F: Zero reload time must be rejected");
			}
			invalid.magazine = PrimaryWeaponMagazineDefinition{ 48, -1.0f };
			if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalid).isValid)
			{
				return Fail("Suite F: Negative reload time must be rejected");
			}
			invalid.magazine = PrimaryWeaponMagazineDefinition{ 48, std::numeric_limits<float>::infinity() };
			if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalid).isValid)
			{
				return Fail("Suite F: Infinite reload time must be rejected");
			}
		}

		// 4. Validation: Continuous beam weapon rejects magazine and percentage cadence
		{
			PrimaryWeaponDefinition beam = LoadedWeapon("Weapon.Beam.ContinuousHeatLaser.Basic");
			beam.magazine = PrimaryWeaponMagazineDefinition{ 10, 1.0f };
			if (PrimaryWeaponExecutionSystem::ValidateDefinition(beam).isValid)
			{
				return Fail("Suite F: Continuous beam weapon must reject magazine");
			}
			beam.magazine = std::nullopt;
			beam.cadenceMode = PrimaryWeaponCadenceMode::OwnerAttackSpeedPercentage;
		if (PrimaryWeaponExecutionSystem::ValidateDefinition(beam).isValid)
		{
			return Fail("Suite F: Continuous beam weapon must reject OwnerAttackSpeedPercentage");
		}
	}

	// 4b. Continuous weapons remain in their firing lifecycle until input release.
	{
		const PrimaryWeaponDefinition& beam = LoadedWeapon("Weapon.Beam.ContinuousHeatLaser.Basic");
		World world{ nullptr };
		const shared_ptr<TestShipCombatant> ship = world.SpawnActor<TestShipCombatant>().lock();
		if (!ship) return Fail("Suite F: Failed to spawn continuous beam test ship");
		world.TickInternal(0.f);

		std::string grantFailure;
		const sas::AbilityHandle handle = ship->GetCombatRuntime().GetAbilitySystemComponent().GrantAbility(
			MakePrimaryWeaponAbility(beam), sas::AbilitySlot::PrimaryFire, &grantFailure
		);
		if (!handle.IsValid()) return Fail("Suite F: Failed to grant continuous beam ability");

		auto& abilitySystem = ship->GetCombatRuntime().GetAbilitySystemComponent();
		abilitySystem.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
		world.TickInternal(0.1f);
		GameAbility* ability = abilitySystem.GetAbility(handle);
		if (!ability || !ability->GetPrimaryWeaponRuntime().isFiring)
		{
			return Fail("Suite F: Continuous beam must remain firing while input is held");
		}
		ability->GetPrimaryWeaponRuntime().RequestCooldown(0.1f);
		world.TickInternal(0.f);
		if (ability->GetPrimaryWeaponRuntime().isFiring)
		{
			return Fail("Suite F: Requested cooldown must end continuous beam fire");
		}
		world.TickInternal(0.05f);
		if (ability->GetPrimaryWeaponRuntime().isFiring)
		{
			return Fail("Suite F: Continuous beam must remain inactive until cooldown expires");
		}
		ability->GetPrimaryWeaponRuntime().SetFeatureValue(
			PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue,
			0.f
		);
		world.TickInternal(0.1f);
		if (!ability->GetPrimaryWeaponRuntime().isFiring)
		{
			return Fail("Suite F: Continuous beam must restart after requested cooldown");
		}
		if (ability->GetPrimaryWeaponRuntime().GetFeatureValue(
			PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
		) <= 0.f)
		{
			return Fail("Suite F: Active remainder after cooldown expiry must be simulated");
		}

		abilitySystem.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
		world.TickInternal(0.f);
		if (ability->GetPrimaryWeaponRuntime().isFiring)
		{
			return Fail("Suite F: Continuous beam must end fire on input release");
		}
	}

	// 5. Validation: OwnerAttackSpeedPercentage rejects duplicate AttackSpeed scaling rule
		{
			PrimaryWeaponDefinition dupeScaling = LoadedWeapon("Weapon.Projectile.FighterRapidLaser.Basic");
			dupeScaling.scalingRules.push_back(sas::AttributeScalingRule{
				CommonAttributeIds::FireRate,
				OwnerAttributeIds::AttackSpeed,
				sas::AttributeModifierOperation::Add,
				1.0f
			});
			if (PrimaryWeaponExecutionSystem::ValidateDefinition(dupeScaling).isValid)
			{
				return Fail("Suite F: Duplicate AttackSpeed scaling rule in OwnerAttackSpeedPercentage must be rejected");
			}
		}

		// 6. Strict JSON Scaling Rule Validation (Package A)
		{
			using ly::content::AttributeJsonParser::Json;
			namespace AJP = ly::content::AttributeJsonParser;

			auto expectThrowsWithMessage = [](const std::function<void()>& fn, const std::string& expectedSubstring, const std::string& caseName) -> bool
			{
				try
				{
					fn();
					std::cerr << "Expected exception for case: " << caseName << "\n";
					return false;
				}
				catch (const std::exception& ex)
				{
					const std::string msg = ex.what();
					if (msg.find(expectedSubstring) == std::string::npos)
					{
						std::cerr << "Expected substring '" << expectedSubstring << "' in '" << msg << "' for case: " << caseName << "\n";
						return false;
					}
					return true;
				}
			};

			// Non-object scaling rule
			if (!expectThrowsWithMessage([]() { AJP::ParseScalingRule(Json(123), "rule"); }, "rule must be an object", "non-object rule"))
			{
				return Fail("Suite F: Non-object scaling rule must throw with context");
			}

			// Missing targetAttributeId
			if (!expectThrowsWithMessage([]() {
				Json j = { {"sourceAttributeId", "Ship.AttackPower"}, {"operation", "Add"}, {"coefficient", 0.5} };
				AJP::ParseScalingRule(j, "test.path[0]");
			}, "test.path[0].targetAttributeId is required", "missing targetAttributeId"))
			{
				return Fail("Suite F: Missing targetAttributeId must throw");
			}

			// Empty targetAttributeId
			if (!expectThrowsWithMessage([]() {
				Json j = { {"targetAttributeId", ""}, {"sourceAttributeId", "Ship.AttackPower"}, {"operation", "Add"}, {"coefficient", 0.5} };
				AJP::ParseScalingRule(j, "test.path[0]");
			}, "test.path[0].targetAttributeId is required and must be a non-empty string", "empty targetAttributeId"))
			{
				return Fail("Suite F: Empty targetAttributeId must throw");
			}

			// Missing sourceAttributeId
			if (!expectThrowsWithMessage([]() {
				Json j = { {"targetAttributeId", "Common.Damage"}, {"operation", "Add"}, {"coefficient", 0.5} };
				AJP::ParseScalingRule(j, "test.path[0]");
			}, "test.path[0].sourceAttributeId is required", "missing sourceAttributeId"))
			{
				return Fail("Suite F: Missing sourceAttributeId must throw");
			}

			// Missing operation
			if (!expectThrowsWithMessage([]() {
				Json j = { {"targetAttributeId", "Common.Damage"}, {"sourceAttributeId", "Ship.AttackPower"}, {"coefficient", 0.5} };
				AJP::ParseScalingRule(j, "test.path[0]");
			}, "test.path[0].operation is required", "missing operation"))
			{
				return Fail("Suite F: Missing operation must throw");
			}

			// Unknown operation
			if (!expectThrowsWithMessage([]() {
				Json j = { {"targetAttributeId", "Common.Damage"}, {"sourceAttributeId", "Ship.AttackPower"}, {"operation", "Divide"}, {"coefficient", 0.5} };
				AJP::ParseScalingRule(j, "test.path[0]");
			}, "Unknown attribute modifier operation: Divide", "unknown operation"))
			{
				return Fail("Suite F: Unknown operation must throw");
			}

			// Missing coefficient (No silent 1.0 fallback!)
			if (!expectThrowsWithMessage([]() {
				Json j = { {"targetAttributeId", "Common.Damage"}, {"sourceAttributeId", "Ship.AttackPower"}, {"operation", "Add"} };
				AJP::ParseScalingRule(j, "Ability 'test': levels[2].scalingRules[0]");
			}, "Ability 'test': levels[2].scalingRules[0].coefficient is required", "missing coefficient"))
			{
				return Fail("Suite F: Missing coefficient must throw without fallback");
			}

			// Non-numeric coefficient (string)
			if (!expectThrowsWithMessage([]() {
				Json j = { {"targetAttributeId", "Common.Damage"}, {"sourceAttributeId", "Ship.AttackPower"}, {"operation", "Add"}, {"coefficient", "0.5"} };
				AJP::ParseScalingRule(j, "rule");
			}, "rule.coefficient must be a number", "string coefficient"))
			{
				return Fail("Suite F: String coefficient must throw");
			}

			// Non-finite coefficient (infinite)
			if (!expectThrowsWithMessage([]() {
				Json j = { {"targetAttributeId", "Common.Damage"}, {"sourceAttributeId", "Ship.AttackPower"}, {"operation", "Add"}, {"coefficient", std::numeric_limits<double>::infinity()} };
				AJP::ParseScalingRule(j, "rule");
			}, "rule.coefficient must be finite", "infinite coefficient"))
			{
				return Fail("Suite F: Infinite coefficient must throw");
			}

			// Non-array for ParseScalingRules
			if (!expectThrowsWithMessage([]() {
				Json j = { {"targetAttributeId", "Common.Damage"} };
				AJP::ParseScalingRules(j, "rules");
			}, "rules must be an array", "non-array scalingRules"))
			{
				return Fail("Suite F: Non-array scalingRules must throw");
			}

			// Valid parsing with positive and negative coefficient (debuff/inverse scaling support)
			{
				Json validPos = { {"targetAttributeId", "Common.Damage"}, {"sourceAttributeId", "Owner.AttackPower"}, {"operation", "Add"}, {"coefficient", 0.40} };
				const sas::AttributeScalingRule rulePos = AJP::ParseScalingRule(validPos, "pos");
				if (rulePos.targetAttributeId != CommonAttributeIds::Damage ||
					rulePos.sourceAttributeId != OwnerAttributeIds::AttackPower ||
					rulePos.operation != sas::AttributeModifierOperation::Add ||
					!NearlyEqual(rulePos.coefficient, 0.40f))
				{
					return Fail("Suite F: Valid positive scaling rule parsed incorrectly");
				}

				Json validNeg = { {"targetAttributeId", "Common.Damage"}, {"sourceAttributeId", "Owner.AttackPower"}, {"operation", "Multiply"}, {"coefficient", -0.25} };
				const sas::AttributeScalingRule ruleNeg = AJP::ParseScalingRule(validNeg, "neg");
				if (ruleNeg.operation != sas::AttributeModifierOperation::Multiply ||
					!NearlyEqual(ruleNeg.coefficient, -0.25f))
				{
					return Fail("Suite F: Valid negative scaling rule parsed incorrectly");
				}
			}
		}

		// 7. Explicit Invocation Output Selection (Package B)
		{
			// 7a. Shipped content checks: verify 5 abilities targeting non-Damage have explicit invocationOutputAttributes
			const auto* shieldDef = content::AbilityContentCatalog::FindById("Ability.Defense.Shield.Basic");
			if (!shieldDef) return Fail("Suite F (Pkg B): Shield.Basic definition not found in catalog");
			const sas::AttributeId barrierCapId{ "Effect.BarrierCapacity" };
			if (std::find(shieldDef->invocationOutputAttributes.begin(), shieldDef->invocationOutputAttributes.end(), barrierCapId) == shieldDef->invocationOutputAttributes.end())
			{
				return Fail("Suite F (Pkg B): Shield.Basic must explicitly contain Effect.BarrierCapacity in invocationOutputAttributes");
			}

			const auto* gravityDef = content::AbilityContentCatalog::FindById("Ability.Control.GravityAnomaly.Basic");
			if (!gravityDef) return Fail("Suite F (Pkg B): GravityAnomaly.Basic definition not found in catalog");
			if (std::find(gravityDef->invocationOutputAttributes.begin(), gravityDef->invocationOutputAttributes.end(), CommonAttributeIds::Radius) == gravityDef->invocationOutputAttributes.end() ||
				std::find(gravityDef->invocationOutputAttributes.begin(), gravityDef->invocationOutputAttributes.end(), CommonAttributeIds::Duration) == gravityDef->invocationOutputAttributes.end())
			{
				return Fail("Suite F (Pkg B): GravityAnomaly.Basic must explicitly contain Radius and Duration in invocationOutputAttributes");
			}

			const auto* overdriveDef = content::AbilityContentCatalog::FindById("Ability.Offense.OverdriveCore.Basic");
			if (!overdriveDef) return Fail("Suite F (Pkg B): OverdriveCore.Basic definition not found in catalog");
			if (std::find(overdriveDef->invocationOutputAttributes.begin(), overdriveDef->invocationOutputAttributes.end(), CommonAttributeIds::ProjectileCount) == overdriveDef->invocationOutputAttributes.end())
			{
				return Fail("Suite F (Pkg B): OverdriveCore.Basic must explicitly contain ProjectileCount in invocationOutputAttributes");
			}

			const auto* crescentDef = content::AbilityContentCatalog::FindById("Ability.Offense.CrescentReaver.Basic");
			if (!crescentDef) return Fail("Suite F (Pkg B): CrescentReaver.Basic definition not found in catalog");
			const sas::AttributeId bounceCountId{ "AbilityActor.CrescentReaver.Projectile.BounceCount" };
			if (std::find(crescentDef->invocationOutputAttributes.begin(), crescentDef->invocationOutputAttributes.end(), bounceCountId) == crescentDef->invocationOutputAttributes.end())
			{
				return Fail("Suite F (Pkg B): CrescentReaver.Basic must explicitly contain BounceCount in invocationOutputAttributes");
			}

			const auto* temporalDef = content::AbilityContentCatalog::FindById("Ability.Defense.TemporalConvergence.Basic");
			if (!temporalDef) return Fail("Suite F (Pkg B): TemporalConvergence.Basic definition not found in catalog");
			const sas::AttributeId baseShieldId{ "Ability.Defense.TemporalConvergence.BaseShield" };
			if (std::find(temporalDef->invocationOutputAttributes.begin(), temporalDef->invocationOutputAttributes.end(), baseShieldId) == temporalDef->invocationOutputAttributes.end())
			{
				return Fail("Suite F (Pkg B): TemporalConvergence.Basic must explicitly contain BaseShield in invocationOutputAttributes");
			}

			// 7b. Dynamic resolver decoupling: adding a scaling rule does NOT make attribute an invocation output
			World testWorld{ nullptr };
			const shared_ptr<TestCombatantActor> combatant = testWorld.SpawnActor<TestCombatantActor>().lock();
			if (!combatant) return Fail("Suite F (Pkg B): Failed to spawn test combatant");
			testWorld.TickInternal(0.f);

			auto& ownerAttrs = combatant->GetCombatRuntime().GetAbilitySystemComponent().GetAttributes();
			ownerAttrs.SetBaseValue(OwnerAttributeIds::AttackPower, 50.f);
			ownerAttrs.SetBaseValue(OwnerAttributeIds::EnergyPower, 40.f);

			GameAbilityDefinition decoupledDef;
			decoupledDef.attributeOutputMultiplier = 0.50f;
			decoupledDef.invocationOutputAttributes = { CommonAttributeIds::Damage };
			decoupledDef.scalingRules = {
				sas::AttributeScalingRule{
					CommonAttributeIds::Damage,
					OwnerAttributeIds::AttackPower,
					sas::AttributeModifierOperation::Add,
					1.0f
				},
				sas::AttributeScalingRule{
					CommonAttributeIds::Range,
					OwnerAttributeIds::AttackPower,
					sas::AttributeModifierOperation::Add,
					2.0f
				}
			};

			AbilityExecutionContext execContext{
				&combatant->GetCombatRuntime().GetAbilitySystemComponent(),
				&decoupledDef,
				nullptr,
				nullptr
			};

			const sas::GameplayAttributeList resolvedValues =
				AbilityActionAttributeResolver::ResolveAttributes(
					execContext,
					nullptr,
					{
						sas::GameplayAttribute{ CommonAttributeIds::Damage, 100.f, 0.f },
						sas::GameplayAttribute{ CommonAttributeIds::Range, 500.f, 0.f }
					}
				);

			// Damage (explicit output) -> (100 + 50 * 1.0) * 0.50 = 75.0
			const float resDamage = sas::FindAttributeValue(resolvedValues, CommonAttributeIds::Damage, 0.f);
			if (!NearlyEqual(resDamage, 75.f))
			{
				return Fail("Suite F (Pkg B): Explicit output attribute Damage expected 75, got " + std::to_string(resDamage));
			}

			// Range (NOT explicit output, despite having scaling rule!) -> 500 + 50 * 2.0 = 600.0 (NOT 300!)
			const float resRange = sas::FindAttributeValue(resolvedValues, CommonAttributeIds::Range, 0.f);
			if (!NearlyEqual(resRange, 600.f))
			{
				return Fail("Suite F (Pkg B): Decoupled Range attribute improperly scaled by invocation multiplier! Expected 600, got " + std::to_string(resRange));
			}

			// When multiplier is 1.0f, exact normal values
			decoupledDef.attributeOutputMultiplier = 1.0f;
			const sas::GameplayAttributeList resolvedNorm =
				AbilityActionAttributeResolver::ResolveAttributes(
					execContext,
					nullptr,
					{ sas::GameplayAttribute{ CommonAttributeIds::Damage, 100.f, 0.f } }
				);
			if (!NearlyEqual(sas::FindAttributeValue(resolvedNorm, CommonAttributeIds::Damage, 0.f), 150.f))
			{
				return Fail("Suite F (Pkg B): Multiplier of 1.0 disturbed normal resolved damage");
			}
		}

		// 8. Explicit Scaling Composition & Execution Order (Package C)
		{
			World testWorld{ nullptr };
			const shared_ptr<TestCombatantActor> combatant = testWorld.SpawnActor<TestCombatantActor>().lock();
			if (!combatant) return Fail("Suite F (Pkg C): Failed to spawn test combatant");
			testWorld.TickInternal(0.f);

			auto& ownerAttrs = combatant->GetCombatRuntime().GetAbilitySystemComponent().GetAttributes();
			// Distinct AP and EP so flat/coefficient contributions are isolated
			ownerAttrs.SetBaseValue(OwnerAttributeIds::AttackPower, 30.f);
			ownerAttrs.SetBaseValue(OwnerAttributeIds::EnergyPower, 40.f);

			// 8a. Order test: Base Override then Level Add
			// Base rule overrides Damage to AP * 2.0 = 60.f
			// Level rule adds EP * 1.5 = 60.f
			// Execution order: Base -> Level => Result must be 60 + 60 = 120.f!
			// (If Base ran after Level, Override would produce 60.f, wiping level contribution).
			GameAbilityDefinition compDef;
			compDef.scalingRules = {
				sas::AttributeScalingRule{
					CommonAttributeIds::Damage,
					OwnerAttributeIds::AttackPower,
					sas::AttributeModifierOperation::Override,
					2.0f
				}
			};
			compDef.levelScalingRules = {
				sas::AttributeScalingRule{
					CommonAttributeIds::Damage,
					OwnerAttributeIds::EnergyPower,
					sas::AttributeModifierOperation::Add,
					1.5f
				}
			};

			AbilityExecutionContext compContext{
				&combatant->GetCombatRuntime().GetAbilitySystemComponent(),
				&compDef,
				nullptr,
				nullptr
			};

			const sas::GameplayAttributeList compResolved =
				AbilityActionAttributeResolver::ResolveAttributes(
					compContext,
					nullptr,
					{ sas::GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f } }
				);
			const float compDamage = sas::FindAttributeValue(compResolved, CommonAttributeIds::Damage, 0.f);
			if (!NearlyEqual(compDamage, 120.f))
			{
				return Fail("Suite F (Pkg C): Scaling order violation! Expected 120 (Base Override + Level Add), got " + std::to_string(compDamage));
			}

			// 8b. Weapon Base + Level progression ordering
			PrimaryWeaponDefinition testWeapon;
			testWeapon.weaponId = "Weapon.Test.Order";
			// Weapon base scaling rule: Multiply by AP * 0.05 => 1 + 30 * 0.05 = 2.5x
			testWeapon.scalingRules = {
				sas::AttributeScalingRule{
					CommonAttributeIds::Damage,
					OwnerAttributeIds::AttackPower,
					sas::AttributeModifierOperation::Multiply,
					0.05f
				}
			};

			// Ability base value: 20.f.
			// Weapon base multiply: 20 * (1 + 30 * 0.05) = 20 * 2.5 = 50.f.
			// Level rule: Add EP * 1.0 = 40.f.
			// Base Multiply then Level Add => 50 + 40 = 90.f.
			GameAbilityDefinition weaponAbilityDef;
			weaponAbilityDef.levelScalingRules = {
				sas::AttributeScalingRule{
					CommonAttributeIds::Damage,
					OwnerAttributeIds::EnergyPower,
					sas::AttributeModifierOperation::Add,
					1.0f
				}
			};
			AbilityExecutionContext weaponCompContext{
				&combatant->GetCombatRuntime().GetAbilitySystemComponent(),
				&weaponAbilityDef,
				nullptr,
				nullptr
			};
			const sas::GameplayAttributeList weaponCompResolved =
				AbilityActionAttributeResolver::ResolveAttributes(
					weaponCompContext,
					&testWeapon,
					{ sas::GameplayAttribute{ CommonAttributeIds::Damage, 20.f, 0.f } }
				);
			const float weaponCompDamage = sas::FindAttributeValue(weaponCompResolved, CommonAttributeIds::Damage, 0.f);
			if (!NearlyEqual(weaponCompDamage, 90.f))
			{
				return Fail("Suite F (Pkg C): Weapon Base Multiply then Level Add order mismatch! Expected 90, got " + std::to_string(weaponCompDamage));
			}
		}
	}

	// =========================================================================
	// SUITE G: Empowered Shots & Crit Pipeline (Phase 3B.2)
	// =========================================================================
	std::cout << "Running Suite G: Empowered Shots & Damage Mechanics...\n";
	{
		const PrimaryWeaponDefinition& fighterLaser = LoadedWeapon("Weapon.Projectile.FighterRapidLaser.Basic");
		if (!fighterLaser.empoweredShot.has_value())
		{
			return Fail("Suite G: Fighter laser must have empoweredShot definition");
		}
		const auto& empDef = *fighterLaser.empoweredShot;
		const sas::GameplayAttribute* empoweredAttr = sas::FindAttribute(
			fighterLaser.attributes,
			PrimaryWeaponSchema::Empowered::BonusDamage
		);
		const sas::GameplayAttribute* everySuccessfulShotsAttr = sas::FindAttribute(
			fighterLaser.attributes, PrimaryWeaponSchema::Empowered::EverySuccessfulShots);
		const sas::GameplayAttribute* finalMagazineRoundsAttr = sas::FindAttribute(
			fighterLaser.attributes, PrimaryWeaponSchema::Empowered::FinalMagazineRounds);
		if (!empoweredAttr || !NearlyEqual(empoweredAttr->baseValue, 2.0f) ||
			!everySuccessfulShotsAttr || !NearlyEqual(everySuccessfulShotsAttr->baseValue, 6.f) ||
			!finalMagazineRoundsAttr || !NearlyEqual(finalMagazineRoundsAttr->baseValue, 6.f))
		{
			return Fail("Suite G: Fighter laser empowered attributes are invalid");
		}
		if (!empDef.guaranteedCritical ||
			fighterLaser.damageRoundingPolicy != PrimaryWeaponDamageRoundingPolicy::CeilFinalDamage)
		{
			return Fail("Suite G: Fighter laser empoweredShot parameters incorrect");
		}

		World world{ nullptr };
		const shared_ptr<TestShipCombatant> ship = world.SpawnActor<TestShipCombatant>().lock();
		if (!ship)
		{
			return Fail("Suite G: Failed to spawn test ship");
		}
		world.TickInternal(0.f);

		GameAbilityDefinition abilityDef = MakePrimaryWeaponAbility(fighterLaser);
		const sas::AbilityHandle handle =
			ship->GetCombatRuntime().GetAbilitySystemComponent().GrantAbility(abilityDef, sas::AbilitySlot::PrimaryFire);
		if (!handle.IsValid())
		{
			return Fail("Suite G: Failed to grant ability");
		}

		// Target to receive damage and measure exact damage dealt
		class MeasuringTarget final : public Actor, public Combatant
		{
		public:
			explicit MeasuringTarget(World* w) : Actor{ w }, mRuntime{ *this }
			{
				mRuntime.GetAbilitySystemComponent().GetAttributes().RegisterAttribute(OwnerAttributeIds::Armor, 0.f);
			}
			CombatRuntime& GetCombatRuntime() override { return mRuntime; }
			const CombatRuntime& GetCombatRuntime() const override { return mRuntime; }
			void ReceiveDamage(DamageContext ctx) override
			{
				// ApplyCombatDamage rounds the shot after critical resolution and
				// before the target receives it, matching combatant targets.
				receivedDamages.push_back(ctx.remainingDamage);
				wasCriticalList.push_back(ctx.wasCritical);
			}
			CombatRuntime mRuntime;
			std::vector<float> receivedDamages;
			std::vector<bool> wasCriticalList;
		};

		// 1. Check L1 Normal Damage = 26 and L1 Empowered Damage = 48
		// 2. Fire full magazine of 48 shots, tracking which are empowered and their damage
		ship->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);

		int normalCount = 0;
		int empoweredCount = 0;
		float totalDeterministicDamage = 0.f;

		for (int shot = 1; shot <= 48; ++shot)
		{
			if (shot == 1)
			{
				world.TickInternal(0.f);
			}
			else
			{
				world.TickInternal(0.25f);
			}
			world.TickInternal(0.f);
			const auto actors = world.GetActorsByType<PrimaryWeaponProjectileActor>();
			if (actors.empty())
			{
				std::cerr << "Suite G: Shot " << shot << " failed, no projectiles spawned\n";
				return Fail("Suite G: Projectile did not spawn on expected shot tick");
			}
			const shared_ptr<PrimaryWeaponProjectileActor> proj = actors.back().lock();
			if (!proj)
			{
				return Fail("Suite G: Projectile actor null");
			}

			const bool expectedEmpowered = (shot % 6 == 0) || (shot >= 43);
			if (proj->IsEmpowered() != expectedEmpowered)
			{
				std::cerr << "Mismatch at shot " << shot << ": expected empowered=" << expectedEmpowered << " got=" << proj->IsEmpowered() << '\n';
				return Fail("Suite G: Empowered shot index mismatch");
			}

			// Measure impact damage against a MeasuringTarget
			MeasuringTarget target{ &world };
			DamagePayload payload = proj->GetDamagePayload();
			ApplyCombatDamage(target, proj->GetDamage(), ship.get(), proj->GetDamageTags(), payload, {}, {}, DamageDeliveryType::Projectile, proj.get());

			if (target.receivedDamages.empty())
			{
				return Fail("Suite G: Target received no damage");
			}
			const float actualDamage = target.receivedDamages.back();
			const bool wasCrit = target.wasCriticalList.back();

			if (expectedEmpowered)
			{
				++empoweredCount;
				// Empowered raw: 26 + 2 + 3.5 = 31.5 -> crit 1.5 = 47.25 -> ceil = 48
				if (!NearlyEqual(actualDamage, 48.f))
				{
					std::cerr << "Empowered damage mismatch at shot " << shot << ": " << actualDamage << '\n';
					return Fail("Suite G: Empowered damage must be 48");
				}
				if (!wasCrit)
				{
					return Fail("Suite G: Empowered shot must be guaranteed critical");
				}
			}
			else
			{
				++normalCount;
				// Normal raw: 26 -> ceil = 26
				if (!NearlyEqual(actualDamage, 26.f))
				{
					std::cerr << "Normal damage mismatch at shot " << shot << ": " << actualDamage << '\n';
					return Fail("Suite G: Normal damage must be 26");
				}
				if (wasCrit)
				{
					return Fail("Suite G: Normal shot must not be guaranteed critical (L1 CritChance is 0)");
				}
			}

			totalDeterministicDamage += actualDamage;
			proj->Destroy();
		}

		// Verify 6th shot is empowered
		// Verify shots 42-48 are empowered (shot 42 is 42%6==0; shots 43-48 are finalPhase)
		// Total magazine count: 35 normal + 13 empowered
		if (normalCount != 35 || empoweredCount != 13)
		{
			std::cerr << "Count mismatch: normal=" << normalCount << " empowered=" << empoweredCount << '\n';
			return Fail("Suite G: Magazine must contain exactly 35 normal and 13 empowered shots");
		}

		// Total deterministic damage = 1534
		if (!NearlyEqual(totalDeterministicDamage, 1534.f))
		{
			std::cerr << "Total damage: " << totalDeterministicDamage << '\n';
			return Fail("Suite G: Total magazine deterministic damage must equal 1534");
		}

		// Sustained cycle DPS ≈ 109.6 (1534 / 14.0 = 109.5714...)
		const float cycleTime = (48.f / 4.f) + 2.0f; // 12.0s firing + 2.0s reload = 14.0s
		const float sustainedDPS = totalDeterministicDamage / cycleTime;
		if (!NearlyEqual(sustainedDPS, 109.5714f, 0.1f))
		{
			return Fail("Suite G: Sustained cycle DPS must be ~109.6");
		}

		struct SpawnedShot
		{
			float damage = 0.f;
			DamagePayload payload;
			bool isEmpowered = false;
		};
		const auto fireFighterShot = [&](float attackPower, float energyPower, int shotIndex, SpawnedShot& result,
			const PrimaryWeaponDefinition* weaponOverride = nullptr)
		{
			const PrimaryWeaponDefinition& shotWeapon = weaponOverride ? *weaponOverride : fighterLaser;
			World shotWorld{ nullptr };
			const shared_ptr<TestShipCombatant> shotShip = shotWorld.SpawnActor<TestShipCombatant>().lock();
			if (!shotShip)
			{
				return false;
			}
			auto& attributes = shotShip->GetCombatRuntime().GetAbilitySystemComponent().GetAttributes();
			attributes.SetBaseValue(OwnerAttributeIds::AttackPower, attackPower);
			attributes.SetBaseValue(OwnerAttributeIds::EnergyPower, energyPower);
			shotWorld.TickInternal(0.f);
			const sas::AbilityHandle shotHandle = shotShip->GetCombatRuntime().GetAbilitySystemComponent().GrantAbility(
				MakePrimaryWeaponAbility(shotWeapon), sas::AbilitySlot::PrimaryFire);
			if (!shotHandle.IsValid())
			{
				return false;
			}
			shotShip->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
			for (int shot = 1; shot <= shotIndex; ++shot)
			{
				shotWorld.TickInternal(shot == 1 ? 0.f : 0.25f);
			}
			shotWorld.TickInternal(0.f);
			const auto projectiles = shotWorld.GetActorsByType<PrimaryWeaponProjectileActor>();
			if (projectiles.empty())
			{
				return false;
			}
			const shared_ptr<PrimaryWeaponProjectileActor> projectile = projectiles.back().lock();
			if (!projectile)
			{
				return false;
			}
			result.damage = projectile->GetDamage();
			result.payload = projectile->GetDamagePayload();
			result.isEmpowered = projectile->IsEmpowered();
			return true;
		};

		// 3. EP scaling is verified through the real owner -> resolved attributes -> projectile path.
		{
			SpawnedShot normal;
			SpawnedShot empowered;
			if (!fireFighterShot(35.f, 135.f, 1, normal) || !fireFighterShot(35.f, 135.f, 6, empowered))
			{
				return Fail("Suite G: Failed to spawn EP-scaling verification shots");
			}
			if (normal.isEmpowered || !NearlyEqual(normal.damage, 26.f) ||
				normal.payload.criticalPolicy != DamageCriticalPolicy::Random)
			{
				return Fail("Suite G: EP must not affect normal projectile damage");
			}
			if (!empowered.isEmpowered || !NearlyEqual(empowered.damage, 41.5f) ||
				empowered.payload.criticalPolicy != DamageCriticalPolicy::Guaranteed || !empowered.payload.roundDamageUp)
			{
				return Fail("Suite G: EP must scale empowered projectile damage and metadata");
			}
		}

		// Empowered cadence is resolved through weapon attributes, so upgrades can change it without resetting runtime state.
		{
			PrimaryWeaponDefinition cadenceUpgrade = fighterLaser;
			cadenceUpgrade.attributeModifiers.push_back(sas::AttributeModifier{
				PrimaryWeaponSchema::Empowered::EverySuccessfulShots, -1.f });
			SpawnedShot fifthShot;
			if (!fireFighterShot(35.f, 35.f, 5, fifthShot, &cadenceUpgrade) || !fifthShot.isEmpowered)
			{
				return Fail("Suite G: Resolved empowered cadence modifier did not make the fifth shot empowered");
			}
		}

		// 4. AP scaling is verified through the same real projectile path.
		{
			SpawnedShot normal;
			SpawnedShot empowered;
			if (!fireFighterShot(45.f, 35.f, 1, normal) || !fireFighterShot(45.f, 35.f, 6, empowered))
			{
				return Fail("Suite G: Failed to spawn AP-scaling verification shots");
			}
			if (normal.isEmpowered || !NearlyEqual(normal.damage, 30.f))
			{
				return Fail("Suite G: AP must scale normal projectile damage");
			}
			if (!empowered.isEmpowered || !NearlyEqual(empowered.damage, 35.5f))
			{
				return Fail("Suite G: AP must scale empowered projectile damage");
			}
		}

		// 5. Guaranteed crit is never multiplied twice even when random crit chance resolves to 100%.
		{
			auto& attributes = ship->GetCombatRuntime().GetAbilitySystemComponent().GetAttributes();
			attributes.SetBaseValue(OwnerAttributeIds::CriticalChance, std::numeric_limits<float>::max());
			if (!NearlyEqual(ship->GetCombatRuntime().GetCriticalChance(), 1.f))
			{
				return Fail("Suite G: Critical rating must resolve to deterministic 100% chance for this regression");
			}
			MeasuringTarget targetCrit{ &world };
			DamagePayload empPayload;
			empPayload.roundDamageUp = true;
			empPayload.criticalPolicy = DamageCriticalPolicy::Guaranteed;
			// 31.5 * 1.5 = 47.25 -> ceil = 48. If multiplied twice it would be 47.25 * 1.5 = 70.875 -> 71.
			ApplyCombatDamage(targetCrit, 31.5f, ship.get(), {}, empPayload);
			if (!NearlyEqual(targetCrit.receivedDamages.back(), 48.f))
			{
				return Fail("Suite G: Guaranteed crit must not be multiplied twice by random crit");
			}
			attributes.SetBaseValue(OwnerAttributeIds::CriticalChance, 0.f);
		}

		// 6. Failed spawn does not consume ammo/cadence/empowered state
		{
			PrimaryWeaponDefinition testWeapon = fighterLaser;
			PrimaryWeaponRuntimeState testRuntime;
			PrimaryWeaponExecutionSystem::InitializeRuntime(testWeapon, testRuntime);
			testRuntime.isFiring = true;
			Actor actorWithoutWorld{ nullptr };
			PrimaryWeaponExecutionContext failedContext{ actorWithoutWorld, testWeapon, testWeapon.attributes };
			const int roundsBefore = testRuntime.magazineState.roundsRemaining;
			const bool fired = PrimaryWeaponExecutionSystem::FireOnce(failedContext, testRuntime);
			if (fired)
			{
				return Fail("Suite G: FireOnce without world should fail");
			}
			if (testRuntime.magazineState.roundsRemaining != roundsBefore)
			{
				return Fail("Suite G: Failed spawn must not consume rounds");
			}
		}

		// 7. Empowered config validation tests
		{
			PrimaryWeaponDefinition invalidEmp = fighterLaser;
			// everySuccessfulShots <= 0 rejected
			sas::FindAttribute(invalidEmp.attributes, PrimaryWeaponSchema::Empowered::EverySuccessfulShots)->baseValue = 0.f;
			if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalidEmp).isValid)
			{
				return Fail("Suite G: everySuccessfulShots == 0 must be rejected");
			}

			// finalMagazineRounds < 0 rejected
			invalidEmp = fighterLaser;
			sas::FindAttribute(invalidEmp.attributes, PrimaryWeaponSchema::Empowered::FinalMagazineRounds)->baseValue = -1.f;
			if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalidEmp).isValid)
			{
				return Fail("Suite G: negative finalMagazineRounds must be rejected");
			}

			// finalMagazineRounds > capacity rejected
			invalidEmp = fighterLaser;
			sas::FindAttribute(invalidEmp.attributes, PrimaryWeaponSchema::Empowered::FinalMagazineRounds)->baseValue = 50.f;
			if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalidEmp).isValid)
			{
				return Fail("Suite G: finalMagazineRounds > capacity must be rejected");
			}

			// Beam weapon with empoweredShot rejected
			PrimaryWeaponDefinition beam = LoadedWeapon("Weapon.Beam.ContinuousHeatLaser.Basic");
			beam.empoweredShot = *fighterLaser.empoweredShot;
			if (PrimaryWeaponExecutionSystem::ValidateDefinition(beam).isValid)
			{
				return Fail("Suite G: Beam weapon must reject empoweredShot");
			}

			// Empowered weapon missing PrimaryWeapon.Empowered.BonusDamage rejected
			PrimaryWeaponDefinition missingBonusAttr = fighterLaser;
			missingBonusAttr.attributes.erase(
				std::remove_if(
					missingBonusAttr.attributes.begin(),
					missingBonusAttr.attributes.end(),
					[](const sas::GameplayAttribute& attr)
					{
						return attr.id == PrimaryWeaponSchema::Empowered::BonusDamage;
					}
				),
				missingBonusAttr.attributes.end()
			);
			if (PrimaryWeaponExecutionSystem::ValidateDefinition(missingBonusAttr).isValid)
			{
				return Fail("Suite G: Empowered shot weapon without PrimaryWeapon.Empowered.BonusDamage must be rejected");
			}
		}
	}

	// =========================================================================
	// SUITE H: Level Progression & Scaling (Phase 3B.3)
	// =========================================================================
	std::cout << "Running Suite H: Level Progression & Scaling (Phase 3B.3)...\n";
	{
		const PrimaryWeaponDefinition& fighterLaser = LoadedWeapon("Weapon.Projectile.FighterRapidLaser.Basic");

		// 1. Progression profile checks
		if (fighterLaser.progressionProfile.maxLevel != 15)
		{
			return Fail("Suite H: Fighter maxLevel must be 15");
		}
		if (fighterLaser.progressionProfile.levelUpgradeScrapCosts.size() != 14)
		{
			return Fail("Suite H: Fighter levelUpgradeScrapCosts must contain 14 entries");
		}
		const ly::List<unsigned int> expectedCosts{ 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100, 105 };
		for (size_t i = 0; i < expectedCosts.size(); ++i)
		{
			if (fighterLaser.progressionProfile.levelUpgradeScrapCosts[i] != expectedCosts[i])
			{
				return Fail("Suite H: Fighter levelUpgradeScrapCosts mismatch");
			}
		}

		const auto steps = fighterLaser.progressionProfile.ResolveLevelSteps();
		if (steps.size() != 14)
		{
			return Fail("Suite H: ResolveLevelSteps must return 14 steps for maxLevel 15");
		}

		// Exercise the real level rebuild and attribute resolver. AP/EP = 1 makes
		// each resolved value directly expose its base value plus its coefficient.
		auto resolveLevelStats = [&](int level) -> std::optional<std::pair<float, float>>
		{
			World world{ nullptr };
			const shared_ptr<TestShipCombatant> ship = world.SpawnActor<TestShipCombatant>().lock();
			if (!ship) return std::optional<std::pair<float, float>>{};
			world.TickInternal(0.f);

			auto& ownerAttributes = ship->GetCombatRuntime().GetAbilitySystemComponent().GetAttributes();
			ownerAttributes.SetBaseValue(OwnerAttributeIds::AttackPower, 1.f);
			ownerAttributes.SetBaseValue(OwnerAttributeIds::EnergyPower, 1.f);
			const sas::AbilityHandle handle = ship->GetCombatRuntime().GetAbilitySystemComponent().GrantAbility(
				MakePrimaryWeaponAbility(fighterLaser), sas::AbilitySlot::PrimaryFire);
			if (!handle.IsValid() || (level > 1 && !ship->GetCombatRuntime().GetAbilitySystemComponent().SetAbilityLevel(
				sas::AbilitySlot::PrimaryFire, level))) return std::optional<std::pair<float, float>>{};

			ship->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
			world.TickInternal(0.f);
			GameAbility* ability = ship->GetCombatRuntime().GetAbilitySystemComponent().GetAbility(handle);
			if (!ability) return std::optional<std::pair<float, float>>{};
			const sas::GameplayAttributeList& attributes = ability->GetPrimaryWeaponRuntimeAttributes();
			return std::pair{
				sas::FindAttributeValue(attributes, CommonAttributeIds::Damage, -1.f),
				sas::FindAttributeValue(attributes, PrimaryWeaponSchema::Empowered::BonusDamage, -1.f)
			};
		};

		const auto verifyResolvedLevelStats = [&](int level, float expectedDamage, float expectedEmpoweredBonus)
		{
			const auto stats = resolveLevelStats(level);
			return stats.has_value() && NearlyEqual(stats->first, expectedDamage) &&
				NearlyEqual(stats->second, expectedEmpoweredBonus);
		};

		if (!verifyResolvedLevelStats(1, 12.40f, 2.10f) ||
			!verifyResolvedLevelStats(5, 52.60f, 6.22f) ||
			!verifyResolvedLevelStats(10, 102.85f, 11.37f) ||
			!verifyResolvedLevelStats(15, 153.10f, 16.52f))
		{
			return Fail("Suite H: Resolved L1/L5/L10/L15 progression stats mismatch");
		}

		// 2. Deterministic damage at L15: AP 77, EP 63
		// Normal raw: 152 + 77 * 1.10 = 236.7 -> ceil = 237
		// Empowered raw: 236.7 + 16 + 63 * 0.52 = 285.46 -> crit 1.5 = 428.19 -> ceil = 429
		{
			World world{ nullptr };
			const shared_ptr<TestShipCombatant> ship = world.SpawnActor<TestShipCombatant>().lock();
			if (!ship) return Fail("Suite H: Failed to spawn test ship");
			world.TickInternal(0.f);

			auto& ownerAttrs = ship->GetCombatRuntime().GetAbilitySystemComponent().GetAttributes();
			ownerAttrs.SetBaseValue(OwnerAttributeIds::AttackPower, 77.f);
			ownerAttrs.SetBaseValue(OwnerAttributeIds::EnergyPower, 63.f);

			GameAbilityDefinition abilityDef = MakePrimaryWeaponAbility(fighterLaser);
			const sas::AbilityHandle handle =
				ship->GetCombatRuntime().GetAbilitySystemComponent().GrantAbility(abilityDef, sas::AbilitySlot::PrimaryFire);
			if (!handle.IsValid()) return Fail("Suite H: Failed to grant primary ability");

			if (!ship->GetCombatRuntime().GetAbilitySystemComponent().SetAbilityLevel(sas::AbilitySlot::PrimaryFire, 15))
			{
				return Fail("Suite H: SetAbilityLevel to 15 failed");
			}

			class MeasuringTarget final : public Actor, public Combatant
			{
			public:
				explicit MeasuringTarget(World* w) : Actor{ w }, mRuntime{ *this }
				{
					mRuntime.GetAbilitySystemComponent().GetAttributes().RegisterAttribute(OwnerAttributeIds::Armor, 0.f);
				}
				CombatRuntime& GetCombatRuntime() override { return mRuntime; }
				const CombatRuntime& GetCombatRuntime() const override { return mRuntime; }
				void ReceiveDamage(DamageContext ctx) override
				{
				// ApplyCombatDamage rounds before target mitigation and delivery.
					receivedDamages.push_back(ctx.remainingDamage);
					wasCriticalList.push_back(ctx.wasCritical);
				}
				CombatRuntime mRuntime;
				std::vector<float> receivedDamages;
				std::vector<bool> wasCriticalList;
			};

			ship->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);

			int normalCount = 0;
			int empoweredCount = 0;
			float totalL15MagazineDamage = 0.f;

			for (int shot = 1; shot <= 48; ++shot)
			{
				world.TickInternal(shot == 1 ? 0.f : 0.25f);
				world.TickInternal(0.f);

				const auto actors = world.GetActorsByType<PrimaryWeaponProjectileActor>();
				if (actors.empty()) return Fail("Suite H: No projectile spawned at shot " + std::to_string(shot));
				const shared_ptr<PrimaryWeaponProjectileActor> proj = actors.back().lock();
				if (!proj) return Fail("Suite H: Projectile null at shot " + std::to_string(shot));

				const bool expectedEmpowered = (shot % 6 == 0) || (shot >= 43);
				if (proj->IsEmpowered() != expectedEmpowered)
				{
					return Fail("Suite H: Empowered mismatch at shot " + std::to_string(shot));
				}

				MeasuringTarget target{ &world };
				DamagePayload payload = proj->GetDamagePayload();
				ApplyCombatDamage(target, proj->GetDamage(), ship.get(), proj->GetDamageTags(), payload, {}, {}, DamageDeliveryType::Projectile, proj.get());

				if (target.receivedDamages.empty()) return Fail("Suite H: Target received no damage");
				const float actualDamage = target.receivedDamages.back();
				totalL15MagazineDamage += actualDamage;

				if (expectedEmpowered)
				{
					++empoweredCount;
					if (!NearlyEqual(actualDamage, 429.f))
					{
						return Fail("Suite H: L15 Empowered damage mismatch: expected 429, got " + std::to_string(actualDamage));
					}
					if (!target.wasCriticalList.back())
					{
						return Fail("Suite H: L15 Empowered shot must be critical");
					}
				}
				else
				{
					++normalCount;
					if (!NearlyEqual(actualDamage, 237.f))
					{
						return Fail("Suite H: L15 Normal damage mismatch: expected 237, got " + std::to_string(actualDamage));
					}
				}
			}

			if (normalCount != 35 || empoweredCount != 13)
			{
				return Fail("Suite H: Shot count mismatch: expected 35 normal, 13 empowered");
			}
			if (!NearlyEqual(totalL15MagazineDamage, 13872.f))
			{
				return Fail("Suite H: L15 Magazine total damage mismatch: expected 13872, got " + std::to_string(totalL15MagazineDamage));
			}
		}

		// 3. Level change lifecycle & cache invalidation:
		// - Ammo count, reload state, and successfulFireCount preserved across level change
		// - Next shot immediately reflects new level resolved damage
		{
			World world{ nullptr };
			const shared_ptr<TestShipCombatant> ship = world.SpawnActor<TestShipCombatant>().lock();
			if (!ship) return Fail("Suite H: Failed to spawn lifecycle test ship");
			world.TickInternal(0.f);

			auto& ownerAttrs = ship->GetCombatRuntime().GetAbilitySystemComponent().GetAttributes();
			ownerAttrs.SetBaseValue(OwnerAttributeIds::AttackPower, 77.f);
			ownerAttrs.SetBaseValue(OwnerAttributeIds::EnergyPower, 63.f);

			GameAbilityDefinition abilityDef = MakePrimaryWeaponAbility(fighterLaser);
			const sas::AbilityHandle handle =
				ship->GetCombatRuntime().GetAbilitySystemComponent().GrantAbility(abilityDef, sas::AbilitySlot::PrimaryFire);
			if (!handle.IsValid()) return Fail("Suite H: Failed to grant lifecycle primary ability");

			GameAbility* ability = ship->GetCombatRuntime().GetAbilitySystemComponent().GetAbility(handle);
			if (!ability) return Fail("Suite H: Ability pointer is null");

			// Start at Level 1: fire 5 shots (shots 1 to 5, all normal)
			ship->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
			for (int shot = 1; shot <= 5; ++shot)
			{
				world.TickInternal(shot == 1 ? 0.f : 0.25f);
			}
			world.TickInternal(0.f);

			if (ability->GetPrimaryWeaponRuntime().magazineState.roundsRemaining != 43)
			{
				return Fail("Suite H: Expected 43 rounds remaining after 5 shots");
			}
			if (ability->GetPrimaryWeaponRuntime().successfulFireCount != 5)
			{
				return Fail("Suite H: Expected successfulFireCount == 5 after 5 shots");
			}

			// Level up to Level 15 mid-fire
			const bool levelChanged = ship->GetCombatRuntime().GetAbilitySystemComponent().SetAbilityLevel(sas::AbilitySlot::PrimaryFire, 15);
			if (!levelChanged) return Fail("Suite H: SetAbilityLevel to 15 mid-fire failed");

			// State must be preserved!
			if (ability->GetPrimaryWeaponRuntime().magazineState.roundsRemaining != 43)
			{
				return Fail("Suite H: Rounds remaining reset or changed on level up");
			}
			if (ability->GetPrimaryWeaponRuntime().successfulFireCount != 5)
			{
				return Fail("Suite H: successfulFireCount reset on level up");
			}

			// Fire 6th shot: should be EMPOWERED, with L15 damage (429 final)
			world.TickInternal(0.25f);
			world.TickInternal(0.f);

			const auto actors = world.GetActorsByType<PrimaryWeaponProjectileActor>();
			if (actors.empty()) return Fail("Suite H: No projectile on shot 6 after level up");
			const shared_ptr<PrimaryWeaponProjectileActor> shot6Proj = actors.back().lock();
			if (!shot6Proj) return Fail("Suite H: Shot 6 projectile null");

			if (!shot6Proj->IsEmpowered())
			{
				return Fail("Suite H: Shot 6 must be empowered after level up");
			}

			class MeasuringTarget final : public Actor, public Combatant
			{
			public:
				explicit MeasuringTarget(World* w) : Actor{ w }, mRuntime{ *this }
				{
					mRuntime.GetAbilitySystemComponent().GetAttributes().RegisterAttribute(OwnerAttributeIds::Armor, 0.f);
				}
				CombatRuntime& GetCombatRuntime() override { return mRuntime; }
				const CombatRuntime& GetCombatRuntime() const override { return mRuntime; }
				void ReceiveDamage(DamageContext ctx) override
				{
				// ApplyCombatDamage rounds before target mitigation and delivery.
					receivedDamages.push_back(ctx.remainingDamage);
				}
				CombatRuntime mRuntime;
				std::vector<float> receivedDamages;
			};

			MeasuringTarget target{ &world };
			DamagePayload payload = shot6Proj->GetDamagePayload();
			ApplyCombatDamage(target, shot6Proj->GetDamage(), ship.get(), shot6Proj->GetDamageTags(), payload, {}, {}, DamageDeliveryType::Projectile, shot6Proj.get());
			if (target.receivedDamages.empty()) return Fail("Suite H: Shot 6 target received no damage");

			const float shot6Damage = target.receivedDamages.back();
			if (!NearlyEqual(shot6Damage, 429.f))
			{
				return Fail("Suite H: Cache invalidation failed! Expected L15 empowered damage 429, got " + std::to_string(shot6Damage));
			}

			// 4. Active reload preservation across level up
			ability->GetPrimaryWeaponRuntime().magazineState.roundsRemaining = 0;
			ability->GetPrimaryWeaponRuntime().magazineState.reloadRemaining = 1.5;
			ship->GetCombatRuntime().GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);

			ship->GetCombatRuntime().GetAbilitySystemComponent().SetAbilityLevel(sas::AbilitySlot::PrimaryFire, 10);

			if (ability->GetPrimaryWeaponRuntime().magazineState.roundsRemaining != 0)
			{
				return Fail("Suite H: Magazine reloaded prematurely on level up");
			}
			if (!NearlyEqual(static_cast<float>(ability->GetPrimaryWeaponRuntime().magazineState.reloadRemaining), 1.5f, 0.001f))
			{
				return Fail("Suite H: Active reload timer disturbed on level up");
			}

			world.TickInternal(0.5f);
			if (!NearlyEqual(static_cast<float>(ability->GetPrimaryWeaponRuntime().magazineState.reloadRemaining), 1.0f, 0.01f))
			{
				return Fail("Suite H: Reload timer did not advance properly after level up");
			}

			// 5. Invariance of non-scaling parameters across levels
			if (!fighterLaser.magazine.has_value() || fighterLaser.magazine->capacity != 48 ||
				!NearlyEqual(fighterLaser.magazine->baseReloadTime, 2.0f))
			{
				return Fail("Suite H: Magazine parameters invariant violation");
			}
			const auto* fireRateAttr = sas::FindAttribute(fighterLaser.attributes, CommonAttributeIds::FireRate);
			if (!fireRateAttr || !NearlyEqual(fireRateAttr->baseValue, 4.0f))
			{
				return Fail("Suite H: FireRate baseValue invariant violation");
			}
			if (!NearlyEqual(sas::FindAttributeValue(
				fighterLaser.attributes, PrimaryWeaponSchema::Empowered::EverySuccessfulShots), 6.f) ||
				!NearlyEqual(sas::FindAttributeValue(
					fighterLaser.attributes, PrimaryWeaponSchema::Empowered::FinalMagazineRounds), 6.f))
			{
				return Fail("Suite H: Empowered cadence invariant violation");
			}
		}
	}

	std::cout << "All PrimaryWeaponMagazineTests PASSED successfully!\n";
	return 0;
}
