#include "attributes/AttributeSystem.h"
#include "framework/AssetManager.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <cmath>
#ifdef _WIN32
#include <crtdbg.h>
#include <cstdlib>
#endif
#include <filesystem>
#include <functional>
#include <iostream>
#include <set>
#include <string>

// Phase-2 primary weapon behavior (docs/WEAPON_PHASE2_PLAN.md). Each step adds
// its scenarios here; values come from the design document's level-one tables.
namespace ly
{
	namespace
	{
		int gFailures = 0;

		void Check(bool condition, const std::string& name)
		{
			std::cout << (condition ? "[PASS] " : "[FAIL] ") << name << '\n';
			if (!condition) ++gFailures;
		}

		bool NearlyEqual(float a, float b, float epsilon = 0.01f)
		{
			return std::abs(a - b) <= epsilon;
		}

		PrimaryWeaponDefinition CopyWeapon(const char* weaponId)
		{
			const PrimaryWeaponDefinition* weapon = content::WeaponContentCatalog::FindById(weaponId);
			return weapon ? *weapon : PrimaryWeaponDefinition{};
		}

		void SetAttribute(PrimaryWeaponDefinition& weapon, const sas::AttributeId& id, float value)
		{
			if (sas::GameplayAttribute* attribute = sas::FindAttribute(weapon.attributes, id))
			{
				attribute->baseValue = value;
				attribute->currentValue = value;
				return;
			}
			weapon.attributes.emplace_back(id, value, 0.f);
		}

		void RemoveAttribute(PrimaryWeaponDefinition& weapon, const sas::AttributeId& id)
		{
			weapon.attributes.erase(
				std::remove_if(weapon.attributes.begin(), weapon.attributes.end(),
					[&](const sas::GameplayAttribute& attribute) { return attribute.id == id; }),
				weapon.attributes.end());
		}

		ShipDefinition MakeTestShipDefinition()
		{
			return ShipDefinition{ "", 250.f, { 0.f, 0.f }, 0.f, 0, 0, {}, {} };
		}

		class WeaponTestShip final : public SpaceShip
		{
		public:
			explicit WeaponTestShip(World* world)
				: SpaceShip{ world, MakeTestShipDefinition() }
			{
				auto& attributes = GetCombatRuntime().GetAbilitySystemComponent().GetAttributes();
				attributes.RegisterAttribute(OwnerAttributeIds::AttackSpeed, 0.f);
				attributes.RegisterAttribute(OwnerAttributeIds::AttackPower, 0.f);
				attributes.RegisterAttribute(OwnerAttributeIds::EnergyPower, 0.f);
				attributes.RegisterAttribute(OwnerAttributeIds::Armor, 0.f);
			}
		};

		// A headless world with one ship holding the weapon under test.
		struct WeaponRig
		{
			World world{ nullptr };
			shared_ptr<WeaponTestShip> ship;
			sas::AbilityHandle primary;
			std::set<unsigned int> seenProjectiles;

			bool Setup(const PrimaryWeaponDefinition& weapon, float attackPower = 0.f, float energyPower = 0.f)
			{
				ship = world.SpawnActor<WeaponTestShip>().lock();
				if (!ship) return false;
				auto& attributes = Abilities().GetAttributes();
				attributes.SetBaseValue(OwnerAttributeIds::AttackPower, attackPower);
				attributes.SetBaseValue(OwnerAttributeIds::EnergyPower, energyPower);
				world.TickInternal(0.f);
				std::string failure;
				primary = Abilities().GrantAbility(
					AbilityData::MakePrimaryFireAbilityDefinition(weapon), sas::AbilitySlot::PrimaryFire, &failure);
				if (!primary.IsValid())
				{
					std::cout << "  grant failed: " << failure << '\n';
					return false;
				}
				return true;
			}

			LightYearsAbilitySystemComponent& Abilities()
			{
				return ship->GetCombatRuntime().GetAbilitySystemComponent();
			}

			PrimaryWeaponRuntimeState* Runtime()
			{
				GameAbility* ability = Abilities().GetAbility(primary);
				return ability ? &ability->GetPrimaryWeaponRuntime() : nullptr;
			}

			void Hold(bool held) { Abilities().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, held); }

			// Projectiles spawned since the last call, oldest first.
			List<shared_ptr<PrimaryWeaponProjectileActor>> NewProjectiles()
			{
				world.TickInternal(0.f);
				List<shared_ptr<PrimaryWeaponProjectileActor>> fresh;
				for (const auto& weak : world.GetActorsByType<PrimaryWeaponProjectileActor>())
				{
					const auto projectile = weak.lock();
					if (projectile && seenProjectiles.insert(projectile->GetUniqueID()).second)
					{
						fresh.push_back(projectile);
					}
				}
				return fresh;
			}
		};

		void TestEmpoweredOnHits()
		{
			PrimaryWeaponDefinition laser = CopyWeapon("Weapon.Projectile.FighterRapidLaser.Basic");
			laser.empoweredShot->trigger = PrimaryWeaponEmpoweredTrigger::EverySuccessfulHits;
			SetAttribute(laser, PrimaryWeaponSchema::Empowered::EverySuccessfulHits, 2.f);
			RemoveAttribute(laser, PrimaryWeaponSchema::Empowered::EverySuccessfulShots);

			WeaponRig rig;
			Check(rig.Setup(laser), "hits: rig with a hit-triggered laser");
			if (!rig.ship) return;
			rig.Hold(true);
			const auto fireNext = [&](float deltaTime) -> shared_ptr<PrimaryWeaponProjectileActor>
			{
				rig.world.TickInternal(deltaTime);
				const auto fresh = rig.NewProjectiles();
				return fresh.empty() ? nullptr : fresh.back();
			};

			const auto first = fireNext(0.f);
			Check(first && !first->IsEmpowered(), "hits: first shot is normal");
			if (first) first->GetShotMetadata().ReportHit();
			const auto second = fireNext(0.25f);
			Check(second && !second->IsEmpowered(), "hits: one hit is not enough");
			if (second)
			{
				second->GetShotMetadata().ReportHit();
				// A shot that hits again (pierce) must not count twice; the actor
				// guards this, the ledger itself counts every report.
			}
			const auto third = fireNext(0.25f);
			Check(third && third->IsEmpowered(), "hits: two hits empower the next shot");
			const auto fourth = fireNext(0.25f);
			Check(fourth && !fourth->IsEmpowered(), "hits: the counter restarts after an empowered shot");
			const auto fifth = fireNext(0.25f);
			Check(fifth && !fifth->IsEmpowered(), "hits: misses never empower");
		}

		void TestStatusStacksOnlyWhenEmpowered()
		{
			PrimaryWeaponDefinition dual = CopyWeapon("Weapon.Projectile.DualKineticBlaster.Basic");
			PrimaryWeaponEmpoweredShotDefinition empowered;
			empowered.trigger = PrimaryWeaponEmpoweredTrigger::EverySuccessfulShots;
			empowered.statusStacksOnlyWhenEmpowered = true;
			dual.empoweredShot = empowered;
			SetAttribute(dual, PrimaryWeaponSchema::Empowered::EverySuccessfulShots, 2.f);
			SetAttribute(dual, PrimaryWeaponSchema::Empowered::FinalMagazineRounds, 0.f);
			SetAttribute(dual, PrimaryWeaponSchema::Empowered::BonusDamage, 5.f);
			dual.muzzleDefinitions = { WeaponMuzzleDefinition{} };

			WeaponRig rig;
			Check(rig.Setup(dual), "stacks: rig with an empowered Dual copy");
			if (!rig.ship) return;
			rig.Hold(true);
			rig.world.TickInternal(0.f);
			const auto first = rig.NewProjectiles();
			const float interval = 1.f / sas::FindAttributeValue(dual.attributes, CommonAttributeIds::FireRate, 1.f);
			rig.world.TickInternal(interval);
			const auto second = rig.NewProjectiles();
			Check(first.size() == 1 && second.size() == 1, "stacks: one projectile per shot");
			if (first.size() != 1 || second.size() != 1) return;
			Check(!first[0]->IsEmpowered() && first[0]->GetDamagePayload().kineticStacks == 0,
				"stacks: a normal shot carries no Kinetic stack");
			Check(second[0]->IsEmpowered() && second[0]->GetDamagePayload().kineticStacks == 1,
				"stacks: the empowered shot carries its Kinetic stack");
			Check(NearlyEqual(second[0]->GetDamage() - first[0]->GetDamage(), 5.f),
				"stacks: the empowered shot adds BonusDamage");
		}

		void TestAbilityCastCharges()
		{
			PrimaryWeaponDefinition electric = CopyWeapon("Weapon.Arc.ElectricLauncher.Basic");
			PrimaryWeaponEmpoweredShotDefinition empowered;
			empowered.trigger = PrimaryWeaponEmpoweredTrigger::AbilityCastCharges;
			empowered.chargesPerCast = 2;
			empowered.chargedFireRate = 4.f;
			electric.empoweredShot = empowered;
			SetAttribute(electric, PrimaryWeaponSchema::Empowered::BonusDamage, 0.f);
			SetAttribute(electric, CommonAttributeIds::FireRate, 2.f);
			electric.scalingRules.clear();

			WeaponRig rig;
			Check(rig.Setup(electric), "charges: rig with a cast-charged Electric copy");
			if (!rig.ship) return;
			const GameAbilityDefinition* shield = content::AbilityContentCatalog::FindById("Ability.Defense.Shield.Basic");
			std::string failure;
			const sas::AbilityHandle shieldHandle = shield
				? rig.Abilities().GrantAbility(*shield, sas::AbilitySlot::Ability1, &failure)
				: sas::AbilityHandle{};
			Check(shieldHandle.IsValid(), "charges: a loadout ability is granted (" + failure + ")");

			rig.Hold(true);
			rig.world.TickInternal(0.f);
			PrimaryWeaponRuntimeState* runtime = rig.Runtime();
			Check(runtime && runtime->successfulFireCount == 1 && runtime->empoweredCharges == 0,
				"charges: firing alone grants no charge");
			Check(rig.Abilities().GetLoadoutAbilityActivationCount() == 0,
				"charges: primary fire is not a loadout activation");

			rig.Abilities().SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			rig.world.TickInternal(0.f);
			rig.Abilities().SetAbilitySlotInput(sas::AbilitySlot::Ability1, false);
			Check(rig.Abilities().GetLoadoutAbilityActivationCount() == 1, "charges: the cast is counted");

			rig.world.TickInternal(0.5f);
			runtime = rig.Runtime();
			Check(runtime && runtime->successfulFireCount == 2 && runtime->empoweredCharges == 1 &&
				NearlyEqual(runtime->nextFireIntervalScale, 0.5f),
				"charges: the first charged shot leaves one charge at 4/s");
			rig.world.TickInternal(0.25f);
			runtime = rig.Runtime();
			Check(runtime && runtime->successfulFireCount == 3 && runtime->empoweredCharges == 0 &&
				NearlyEqual(runtime->nextFireIntervalScale, 1.f),
				"charges: the second charged shot follows 0.25 s later and returns to 2/s");
			rig.world.TickInternal(0.25f);
			runtime = rig.Runtime();
			Check(runtime && runtime->successfulFireCount == 3, "charges: normal cadence resumes");
			rig.world.TickInternal(0.25f);
			runtime = rig.Runtime();
			Check(runtime && runtime->successfulFireCount == 4, "charges: the next normal shot after 0.5 s");
		}

		void TestRapidLaserLevelOne()
		{
			// Design: 10 + 5 (L - 1) + AP * 0.41; Fighter L1 AP 39 -> 25.99.
			WeaponRig rig;
			Check(rig.Setup(CopyWeapon("Weapon.Projectile.FighterRapidLaser.Basic"), 39.f, 35.f),
				"laser: rig with the shipped Rapid Laser");
			if (!rig.ship) return;
			rig.Hold(true);
			rig.world.TickInternal(0.f);
			const auto shots = rig.NewProjectiles();
			Check(shots.size() == 1 && NearlyEqual(shots[0]->GetDamage(), 25.99f),
				"laser: Fighter L1 normal shot is 25.99 before rounding");
			Check(shots.size() == 1 && shots[0]->GetDamagePayload().roundDamageUp,
				"laser: Rapid Laser still rounds final damage up");
		}

		int ElectricStacks(SpaceShip& ship)
		{
			const sas::ActiveGameplayEffect* effect = ship.GetCombatRuntime().GetAbilitySystemComponent()
				.FindGameplayEffectById("Effect.Status.Damage.Electric");
			return effect ? effect->stackCount : 0;
		}

		void TestSourceSustainedStatusLifetime()
		{
			World world{ nullptr };
			const shared_ptr<WeaponTestShip> source = world.SpawnActor<WeaponTestShip>().lock();
			const shared_ptr<WeaponTestShip> stranger = world.SpawnActor<WeaponTestShip>().lock();
			const shared_ptr<WeaponTestShip> target = world.SpawnActor<WeaponTestShip>().lock();
			world.TickInternal(0.f);
			if (!source || !stranger || !target)
			{
				Check(false, "lifetime: actors spawned");
				return;
			}
			const List<GameplayTag> electric{ DamageTypeSchema::Electric };
			DamagePayload stacking;
			stacking.electricStacks = 2;
			DamagePayload plain;
			plain.electricStacks = 0;

			ApplyCombatDamage(*target, 1.f, source.get(), electric, stacking);
			Check(ElectricStacks(*target) == 2, "lifetime: two Electric stacks applied");
			world.TickInternal(5.5f);
			Check(ElectricStacks(*target) == 2, "lifetime: stacks do not decay one by one");
			ApplyCombatDamage(*target, 1.f, source.get(), electric, plain);
			world.TickInternal(5.5f);
			Check(ElectricStacks(*target) == 2, "lifetime: a stackless hit from the source keeps them");
			ApplyCombatDamage(*target, 1.f, stranger.get(), electric, plain);
			world.TickInternal(1.f);
			Check(ElectricStacks(*target) == 0, "lifetime: 6 s without source damage clears all stacks at once");
		}
	}
}

int main()
{
	using namespace ly;
#ifdef _WIN32
	// Report debug-CRT assertions on stderr instead of a dialog that would hang
	// a headless test run until its timeout.
	_set_error_mode(_OUT_TO_STDERR);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif
	AssetManager::GetAssetManager().SetAssetRootDirectory(
		(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"
	);
	if (!GameContentBootstrap::Register())
	{
		std::cerr << "GameContentBootstrap::Register failed\n";
		return 1;
	}

	const auto run = [](const char* name, void (*test)())
	{
		try
		{
			test();
		}
		catch (const std::exception& exception)
		{
			Check(false, std::string{ name } + " threw: " + exception.what());
		}
	};
	run("hits", &TestEmpoweredOnHits);
	run("stacks", &TestStatusStacksOnlyWhenEmpowered);
	run("charges", &TestAbilityCastCharges);
	run("lifetime", &TestSourceSustainedStatusLifetime);
	run("laser", &TestRapidLaserLevelOne);

	std::cout << (gFailures == 0 ? "[PASS] All weapon phase-2 checks passed\n" : "[FAIL] Weapon phase-2 checks failed\n");
	return gFailures == 0 ? 0 : 1;
}
