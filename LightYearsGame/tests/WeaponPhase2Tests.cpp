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

			// Sprite-less test ships still need a physics shape to be found by
			// targeting queries.
			float GetPhysicsCollisionRadius() const override { return 20.f; }
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

		void TestDualKineticLevelOne()
		{
			// Design (Interceptor L1: AP 41, EP 28, AttackSpeed +1%):
			// normal 3 + 41 * 0.20 = 11.2; empowered + 5 + 41 * 0.08 + 28 * 0.25 = 26.48;
			// fire rate 7 * 1.01 = 7.07/s; one hit per shot; Kinetic stack only when empowered.
			WeaponRig rig;
			const PrimaryWeaponDefinition dual = CopyWeapon("Weapon.Projectile.DualKineticBlaster.Basic");
			Check(rig.Setup(dual, 41.f, 28.f), "dual: rig with the shipped Dual Kinetic Blaster");
			if (!rig.ship) return;
			rig.Abilities().GetAttributes().SetBaseValue(OwnerAttributeIds::AttackSpeed, 1.f);
			Check(dual.magazine.has_value() && dual.magazine->capacity == 64 &&
				NearlyEqual(dual.magazine->baseReloadTime, 1.5f), "dual: 64-round magazine, 1.5 s reload");
			rig.Hold(true);
			rig.world.TickInternal(0.f);
			const auto first = rig.NewProjectiles();
			PrimaryWeaponRuntimeState* runtime = rig.Runtime();
			Check(first.size() == 1, "dual: one projectile per shot");
			Check(runtime && NearlyEqual(runtime->fireIntervalRemaining, 1.f / 7.07f, 0.001f),
				"dual: AttackSpeed +1% fires at 7.07/s");
			if (first.size() != 1) return;
			Check(NearlyEqual(first[0]->GetDamage(), 11.2f) && first[0]->GetDamagePayload().kineticStacks == 0,
				"dual: normal shot 11.2 without a Kinetic stack");
			first[0]->GetShotMetadata().ReportHit();

			float previousX = first[0]->GetActorLocation().x;
			bool alternates = true;
			shared_ptr<PrimaryWeaponProjectileActor> ninth;
			for (int shot = 2; shot <= 9; ++shot)
			{
				rig.world.TickInternal(1.f / 7.07f + 0.0001f);
				const auto fresh = rig.NewProjectiles();
				if (fresh.size() != 1) { alternates = false; break; }
				alternates = alternates && (fresh[0]->GetActorLocation().x < 0.f) != (previousX < 0.f);
				previousX = fresh[0]->GetActorLocation().x;
				if (shot < 9) fresh[0]->GetShotMetadata().ReportHit();
				else ninth = fresh[0];
			}
			Check(alternates, "dual: muzzles alternate left and right");
			Check(ninth && ninth->IsEmpowered() && NearlyEqual(ninth->GetDamage(), 26.48f) &&
				ninth->GetDamagePayload().kineticStacks == 1,
				"dual: after 8 hits the next shot is 26.48 with one Kinetic stack");
		}

		int ElectricStacks(SpaceShip& ship)
		{
			const sas::ActiveGameplayEffect* effect = ship.GetCombatRuntime().GetAbilitySystemComponent()
				.FindGameplayEffectById("Effect.Status.Damage.Electric");
			return effect ? effect->stackCount : 0;
		}

		struct ElectricShotResult
		{
			float damage = 0.f;
			int electricStacks = 0;
		};

		// Fires the shipped Electric Launcher at one enemy and reports each shot's
		// hull damage and the target's Electric stacks afterwards.
		struct ElectricRange
		{
			WeaponRig rig;
			shared_ptr<WeaponTestShip> target;

			bool Setup(float attackPower, float energyPower)
			{
				if (!rig.Setup(CopyWeapon("Weapon.Arc.ElectricLauncher.Basic"), attackPower, energyPower)) return false;
				rig.ship->SetCollisionLayer(CollisionLayer::Player);
				target = rig.world.SpawnActor<WeaponTestShip>().lock();
				if (!target) return false;
				target->SetActorLocation({ 0.f, -300.f });
				target->SetCollisionLayer(CollisionLayer::Enemy);
				target->SetCollisionMask(CollisionLayer::PlayerBullet);
				rig.world.TickInternal(0.f);
				return true;
			}

			float Health() const { return target->GetHealthComponent().GetHealth(); }
		};

		void TestElectricLauncherLevelOne()
		{
			ElectricRange range;
			Check(range.Setup(27.f, 41.f), "electric: Conductor L1 range");
			if (!range.target) return;
			WeaponRig& rig = range.rig;
			const GameAbilityDefinition* shield = content::AbilityContentCatalog::FindById("Ability.Defense.Shield.Basic");
			if (shield) rig.Abilities().GrantAbility(*shield, sas::AbilitySlot::Ability1);

			rig.Hold(true);
			float before = range.Health();
			rig.world.TickInternal(0.f);
			Check(NearlyEqual(before - range.Health(), 39.1f), "electric: Conductor L1 normal hit 39.1 (EP dominant)");
			Check(ElectricStacks(*range.target) == 0, "electric: a normal hit applies no Electric stack");

			rig.Abilities().SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			rig.world.TickInternal(0.f);
			rig.Abilities().SetAbilitySlotInput(sas::AbilitySlot::Ability1, false);
			before = range.Health();
			rig.world.TickInternal(0.5f);
			Check(NearlyEqual(before - range.Health(), 45.38f), "electric: first empowered hit 45.38");
			Check(ElectricStacks(*range.target) == 1, "electric: an empowered hit applies one Electric stack");
			before = range.Health();
			rig.world.TickInternal(0.25f);
			Check(NearlyEqual(before - range.Health(), 45.38f * 1.03f, 0.02f),
				"electric: second empowered hit 0.25 s later, amplified by the first stack");
			Check(ElectricStacks(*range.target) == 2, "electric: two empowered hits stack twice");

			ElectricRange apDominant;
			Check(apDominant.Setup(50.f, 10.f), "electric: AP-dominant range");
			if (!apDominant.target) return;
			apDominant.rig.Hold(true);
			before = apDominant.Health();
			apDominant.rig.world.TickInternal(0.f);
			Check(NearlyEqual(before - apDominant.Health(), 20.f + 50.f * 0.4f + 10.f * 0.1f),
				"electric: AP becomes the dominant stat when it is higher (41)");
		}


		bool HasIgnite(SpaceShip& ship)
		{
			return ship.GetCombatRuntime().GetAbilitySystemComponent()
				.FindGameplayEffectById("Effect.Status.Damage.Ignite") != nullptr;
		}

		void TestConeShotgunLevelOne()
		{
			// Design (Breacher L1: AP 45, EP 34, HP 290): every target in the cone
			// takes 38 + 45 * 0.60 = 65; every 3rd trigger adds 34 * 0.15 + 290 * 0.03.
			WeaponRig rig;
			Check(rig.Setup(CopyWeapon("Weapon.Cone.RapidShotgun.Basic"), 45.f, 34.f),
				"cone: rig with the shipped Rapid Shotgun");
			if (!rig.ship) return;
			rig.Abilities().GetAttributes().SetBaseValue(OwnerAttributeIds::MaxHealth, 290.f);
			rig.ship->SetCollisionLayer(CollisionLayer::Player);
			const auto spawnTarget = [&](sf::Vector2f location)
			{
				const shared_ptr<WeaponTestShip> target = rig.world.SpawnActor<WeaponTestShip>().lock();
				target->SetActorLocation(location);
				target->SetCollisionLayer(CollisionLayer::Enemy);
				target->SetCollisionMask(CollisionLayer::PlayerBullet);
				return target;
			};
			// The ship faces up; the muzzle sits 50 units ahead.
			const auto ahead = spawnTarget({ 0.f, -300.f });
			const auto offAxis = spawnTarget({ 60.f, -250.f });
			const auto tooFar = spawnTarget({ 0.f, -520.f });
			const auto wide = spawnTarget({ 250.f, -200.f });
			const auto behind = spawnTarget({ 0.f, 200.f });
			rig.world.TickInternal(0.f);
			const auto health = [](const shared_ptr<WeaponTestShip>& ship) { return ship->GetHealthComponent().GetHealth(); };

			rig.Hold(true);
			rig.world.TickInternal(0.f);
			Check(NearlyEqual(250.f - health(ahead), 65.f) && NearlyEqual(250.f - health(offAxis), 65.f),
				"cone: both targets inside the cone take 65 each");
			Check(NearlyEqual(health(tooFar), 250.f) && NearlyEqual(health(wide), 250.f) && NearlyEqual(health(behind), 250.f),
				"cone: targets beyond range, outside the angle or behind take nothing");
			Check(!HasIgnite(*ahead), "cone: a normal blast does not ignite");

			const float interval = 1.f / 1.75f + 0.0001f;
			rig.world.TickInternal(interval);
			Check(!HasIgnite(*ahead), "cone: the second blast does not ignite");
			// Stop just before the third blast, then step onto it so the Ignite it
			// applies has no time left in the frame to tick.
			rig.world.TickInternal(interval - 0.01f);
			const float beforeThird = health(offAxis);
			rig.world.TickInternal(0.0101f);
			Check(NearlyEqual(beforeThird - health(offAxis), 78.8f, 0.05f),
				"cone: the third blast is empowered for 78.8");
			Check(HasIgnite(*ahead) && HasIgnite(*offAxis), "cone: the empowered blast ignites every target it hits");
		}

		void TestHeatLaserLevelOne()
		{
			// Design (Aegis L1: EP 43): tick damage 13 + 43 * 0.19 = 21.17 every
			// 0.25 s, x1.15 from 35% heat and x1.40 (Energy) from 75%.
			const auto measureTick = [](float heat) -> float
			{
				WeaponRig rig;
				if (!rig.Setup(CopyWeapon("Weapon.Beam.ContinuousHeatLaser.Basic"), 0.f, 43.f)) return -1.f;
				rig.ship->SetCollisionLayer(CollisionLayer::Player);
				const shared_ptr<WeaponTestShip> target = rig.world.SpawnActor<WeaponTestShip>().lock();
				target->SetActorLocation({ 0.f, -300.f });
				target->SetCollisionLayer(CollisionLayer::Enemy);
				target->SetCollisionMask(CollisionLayer::PlayerBullet);
				rig.world.TickInternal(0.f);
				rig.Hold(true);
				rig.world.TickInternal(0.f);
				if (PrimaryWeaponRuntimeState* runtime = rig.Runtime())
				{
					runtime->SetFeatureValue(PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue, heat);
				}
				const float before = target->GetHealthComponent().GetHealth();
				rig.world.TickInternal(0.25f);
				return before - target->GetHealthComponent().GetHealth();
			};
			Check(NearlyEqual(measureTick(10.f), 21.17f, 0.05f), "heat: low heat tick 21.17");
			Check(NearlyEqual(measureTick(50.f), 21.17f * 1.15f, 0.05f), "heat: mid heat tick x1.15 (24.35)");
			Check(NearlyEqual(measureTick(85.f), 21.17f * 1.4f, 0.05f), "heat: high heat tick x1.40 (29.64)");
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
	run("dual", &TestDualKineticLevelOne);
	run("electric", &TestElectricLauncherLevelOne);
	run("cone", &TestConeShotgunLevelOne);
	run("heat", &TestHeatLaserLevelOne);

	std::cout << (gFailures == 0 ? "[PASS] All weapon phase-2 checks passed\n" : "[FAIL] Weapon phase-2 checks failed\n");
	return gFailures == 0 ? 0 : 1;
}
