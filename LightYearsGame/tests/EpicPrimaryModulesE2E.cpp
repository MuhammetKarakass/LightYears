#include "framework/EngineContext.h"
#include "gameplay/weapon/wave/ExpandingWaveWeaponActor.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"

#include "gameplay/ability/GameAbilityActionExecutor.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
	using namespace uncommon_p6_e2e;
	constexpr const char* kTurretId = "Module.Primary.TurretMount.Epic";
	constexpr const char* kBroadsideId = "Module.Primary.BroadsideMount.Epic";
	constexpr const char* kExecutionPriorityId = "Module.SideFeature.TurretMount.ExecutionPriority";
	constexpr const char* kWeaponId = "Weapon.Projectile.FighterRapidLaser.Basic";
	// Variant indices: Turret has the two-cell domino (0) and the three-cell L (1, the only size that takes a side feature).
	// Broadside has one three-cell L (0).
	constexpr int kDomino = 0;
	constexpr int kTurretL = 1;
	constexpr int kBroadsideL = 0;

	bool Near(float left, float right, float tolerance = 0.01f) { return std::fabs(left - right) <= tolerance; }
	// Authored three-entry table, extended past level 3 by its last step as ModuleLevelTable does.
	float TableAt(float first, float second, float third, int level)
	{
		if (level <= 1) return first;
		if (level == 2) return second;
		if (level == 3) return third;
		return third + (third - second) * static_cast<float>(level - 3);
	}
	// Projectile forward is RotationToVector(rotation - 90): these are the unit axes the shots are checked against.
	bool Along(const sf::Vector2f& direction, const sf::Vector2f& axis) { return direction.x * axis.x + direction.y * axis.y > 0.999f; }
	const sf::Vector2f kNorth{ 0.f, -1.f };
	const sf::Vector2f kEast{ 1.f, 0.f };
	const sf::Vector2f kSouth{ 0.f, 1.f };
	const sf::Vector2f kWest{ -1.f, 0.f };

	// Level-resolved multiplier of the Primary-only damage rule this placement contributes; -1 when it has none.
	float PrimaryDamageMultiplier(Fixture& f, std::uint32_t id)
	{
		for (const ResolvedModuleDamageRule& rule : f.ship->GetModuleApplier().GetResolvedEffects().damageRules)
			if (rule.instanceId == id && rule.rule.condition == ModuleDamageCondition::PrimaryShotOnly) return rule.rule.damageMultiplier;
		return -1.f;
	}

	struct Volley
	{
		bool fired = false;
		List<shared_ptr<PrimaryWeaponProjectileActor>> projectiles;
	};

	// One real Primary volley through the same resolver and fire calls the fire action makes, with the given mount profile.
	Volley FireVolley(Fixture& f, const PrimaryWeaponMountProfile& mount)
	{
		Volley volley;
		const PrimaryWeaponDefinition* catalog = content::WeaponContentCatalog::FindById(kWeaponId);
		GameAbility* primary = f.ship->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::PrimaryFire);
		f.report.Check(catalog != nullptr && primary != nullptr, "Epic Primary fires the shipped Primary weapon");
		if (!catalog || !primary) return volley;
		const PrimaryWeaponDefinition weapon = *catalog;
		GameAbilityDefinition definition = primary->GetDefinition();
		AbilityExecutionContext resolve{ &f.ship->GetAbilitySystemComponent(), &definition, nullptr, nullptr };
		const sas::GameplayAttributeList attributes = AbilityActionAttributeResolver::ResolveAttributes(resolve, &weapon, weapon.attributes);
		PrimaryWeaponExecutionContext fire{ *f.ship, weapon, attributes };
		fire.damageTags = weapon.damageTags;
		fire.shouldContinue = [] { return true; };
		fire.mountProfile = mount;
		PrimaryWeaponRuntimeState runtime;
		PrimaryWeaponExecutionSystem::InitializeRuntime(weapon, runtime);
		PrimaryWeaponExecutionSystem::BeginFire(fire, runtime);
		List<const PrimaryWeaponProjectileActor*> before;
		for (const auto& weak : f.level->GetActorsByType<PrimaryWeaponProjectileActor>()) if (const auto actor = weak.lock()) before.push_back(actor.get());
		volley.fired = PrimaryWeaponExecutionSystem::FireOnce(fire, runtime);
		PrimaryWeaponExecutionSystem::EndFire(fire, runtime);
		f.level->TickInternal(0.f);
		for (const auto& weak : f.level->GetActorsByType<PrimaryWeaponProjectileActor>())
			if (const auto actor = weak.lock(); actor && std::find(before.begin(), before.end(), actor.get()) == before.end()) volley.projectiles.push_back(actor);
		return volley;
	}

	void FireOtherPrimary(Fixture& f, const char* weaponId, const PrimaryWeaponMountProfile& mount)
	{
		const auto* weapon = content::WeaponContentCatalog::FindById(weaponId);
		f.report.Check(weapon != nullptr, "mount E2E resolves shipped non-projectile Primary");
		PrimaryWeaponExecutionContext fire{ *f.ship, *weapon, weapon->attributes };
		fire.mountProfile = mount;
		fire.shotMetadata.criticalPolicy = DamageCriticalPolicy::Disabled;
		PrimaryWeaponRuntimeState state;
		PrimaryWeaponExecutionSystem::InitializeRuntime(*weapon, state);
		fire.runtime = &state;
		state.handler->BeginFire(fire, *state.typeState);
		if (state.handler->UsesIntervalFire()) f.report.Check(state.handler->FireOnce(fire, *state.typeState), "non-projectile Primary handler fires");
		else state.handler->TickFire(fire, *state.typeState, .25f);
		state.handler->EndFire(fire, *state.typeState);
		f.level->TickInternal(0.f);
		for (const auto& weak : f.level->GetActorsByType<ExpandingWaveWeaponActor>())
			if (const auto wave = weak.lock()) for (int step = 0; step < 45 && !wave->GetIsPendingDestroy(); ++step) wave->Tick(.02f);
	}

	void OtherPrimaryChecks(Report& report, Application& application)
	{
		for (const char* weapon : { "Weapon.Cone.RapidShotgun.Basic", "Weapon.Beam.ContinuousHeatLaser.Basic", "Weapon.Wave.CryoProjector.Basic", "Weapon.Arc.ElectricLauncher.Basic" })
		{
			float fullDamage = 0.f;
			for (int mode = 0; mode < 3; ++mode)
			{
				Fixture f{ report, application };
				f.ship->SetActorRotation(0.f);
				const auto east = Enemy(f, mode == 2 ? 450.f : 200.f);
				const auto other = Enemy(f, 450.f);
				other->SetActorLocation(f.ship->GetActorLocation() + (mode == 2 ? sf::Vector2f{ -450.f, 0.f } : sf::Vector2f{ 0.f, 400.f }));
				other->GetHealthComponent().ChangeHealth(-500.f);
				const auto north = Enemy(f, 450.f);
				north->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 0.f, -450.f });
				for (const auto& target : { east, other, north }) target->SetCollisionMask(GameCollision::PlayerBullet);
				PrimaryWeaponMountProfile mount;
				mount.turret = mode != 2;
				mount.executionPriority = mode == 1;
				if (mode == 2)
				{
					f.Install(kBroadsideId);
					mount = f.ship->GetAbilitySystemComponent().GetPrimaryMountProfile();
				}
				FireOtherPrimary(f, weapon, mount);
				const float eastDamage = 1000.f - east->GetHealthComponent().GetHealth();
				const float otherDamage = 500.f - other->GetHealthComponent().GetHealth();
				if (mode == 0)
				{
					fullDamage = eastDamage;
					report.Check(eastDamage > 0.f && otherDamage == 0.f, std::string{ weapon } + " Turret aims at nearest enemy despite north-facing ship");
				}
				else if (mode == 1) report.Check(otherDamage > 0.f && eastDamage == 0.f, std::string{ weapon } + " Execution Priority chooses the lower health fraction");
				else
				{
					report.Check(fullDamage > 0.f && Near(eastDamage, fullDamage * mount.broadsideDamageMultiplier, .02f) && Near(otherDamage, fullDamage * mount.broadsideDamageMultiplier, .02f), std::string{ weapon } + " Broadside fires both sides with its coefficient on each");
					report.Check(north->GetHealthComponent().GetHealth() == 1000.f, std::string{ weapon } + " Broadside replaces forward fire");
				}
			}
		}
	}

	void HeldBeamMountChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.ship->SetActorRotation(0.f);
		const auto east = Enemy(f, 200.f);
		const auto west = Enemy(f, 200.f);
		west->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ -200.f, 0.f });
		for (const auto& target : { east, west }) target->SetCollisionMask(GameCollision::PlayerBullet);
		const auto* weapon = content::WeaponContentCatalog::FindById("Weapon.Beam.ContinuousHeatLaser.Basic");
		PrimaryWeaponExecutionContext fire{ *f.ship, *weapon, weapon->attributes };
		fire.shotMetadata.criticalPolicy = DamageCriticalPolicy::Disabled;
		PrimaryWeaponRuntimeState state;
		PrimaryWeaponExecutionSystem::InitializeRuntime(*weapon, state);
		fire.runtime = &state;
		state.handler->BeginFire(fire, *state.typeState);
		fire.mountProfile.broadside = true;
		fire.mountProfile.broadsideDamageMultiplier = .55f;
		state.handler->TickFire(fire, *state.typeState, .25f);
		report.Check(east->GetHealthComponent().GetHealth() < 1000.f && west->GetHealthComponent().GetHealth() < 1000.f, "mounting Broadside while a beam is held creates both side beams");
		const float westHealth = west->GetHealthComponent().GetHealth();
		fire.mountProfile = {};
		state.handler->TickFire(fire, *state.typeState, .25f);
		report.Check(west->GetHealthComponent().GetHealth() == westHealth, "removing Broadside while held clears its extra side beam");
		state.handler->EndFire(fire, *state.typeState);
	}

	void ContentChecks(Report& report)
	{
		const ModuleDefinition* turret = content::ModuleContentCatalog::FindById(kTurretId);
		const ModuleDefinition* broadside = content::ModuleContentCatalog::FindById(kBroadsideId);
		report.Check(turret != nullptr && broadside != nullptr, "Epic Primary mounts load from shipped content");
		if (!turret || !broadside) return;
		report.Check(turret->rarity == ModuleRarity::Epic && broadside->rarity == ModuleRarity::Epic, "Turret and Broadside are Epic");
		report.Check(turret->sizeVariants.size() == 2 && turret->sizeVariants[0].shape.cells.size() == 2 && turret->sizeVariants[1].shape.cells.size() == 3,
			"Turret uses the two and three cell ladder");
		report.Check(broadside->sizeVariants.size() == 1 && broadside->sizeVariants[0].shape.cells.size() == 3, "Broadside is a fixed three-cell shape");
		report.Check(AllowsPrivateSideFeature(*turret, sas::ContentId{ kExecutionPriorityId }), "Turret privately allows Execution Priority");
		report.Check(!IsLargestCellCountVariant(*turret, 0) && IsLargestCellCountVariant(*turret, 1), "Execution Priority is usable only in the three-cell Turret");
		report.Check(std::find(turret->conflictsWith.begin(), turret->conflictsWith.end(), sas::ContentId{ kBroadsideId }) != turret->conflictsWith.end() &&
			std::find(broadside->conflictsWith.begin(), broadside->conflictsWith.end(), sas::ContentId{ kTurretId }) != broadside->conflictsWith.end(),
			"Turret and Broadside conflict in both directions");
	}

	void TurretChecks(Report& report, Application& application)
	{
		// Auto-aim ignores facing: the ship faces north, the only enemy is east, and the volley goes east, then south after a turn.
		{
			Fixture f{ report, application };
			f.Install(kTurretId, kTurretL, 1);
			f.ship->SetActorRotation(0.f);
			Enemy(f, 200.f);
			const PrimaryWeaponMountProfile profile = f.ship->GetAbilitySystemComponent().GetPrimaryMountProfile();
			report.Check(profile.turret && !profile.broadside && !profile.executionPriority, "installed Turret reaches the ability component");
			const Volley north = FireVolley(f, profile);
			report.Check(north.fired && !north.projectiles.empty(), "Turret volley spawns Primary projectiles");
			for (const auto& projectile : north.projectiles) report.Check(Along(projectile->GetActorForwardDirection(), kEast), "Turret volley aims east at the enemy while the ship faces north");
			f.ship->SetActorRotation(180.f);
			const Volley south = FireVolley(f, profile);
			for (const auto& projectile : south.projectiles) report.Check(Along(projectile->GetActorForwardDirection(), kEast), "Turret volley still aims east after the ship turns south");
		}
		// No enemy in range: the Turret falls back to the facing. An enemy beyond the 1600 weapon range is not a target.
		{
			Fixture f{ report, application };
			f.Install(kTurretId, kTurretL, 1);
			f.ship->SetActorRotation(0.f);
			Enemy(f, 2000.f);
			const Volley volley = FireVolley(f, f.ship->GetAbilitySystemComponent().GetPrimaryMountProfile());
			report.Check(!volley.projectiles.empty(), "Turret fires with no enemy in range");
			for (const auto& projectile : volley.projectiles) report.Check(Along(projectile->GetActorForwardDirection(), kNorth), "Turret falls back to the facing when the only enemy is out of range");
		}
		// Nearest by default; Execution Priority prefers the lowest health fraction even when that enemy is farther away.
		{
			Fixture f{ report, application };
			f.Install(kTurretId, kTurretL, 1);
			f.ship->SetActorRotation(0.f);
			Enemy(f, 200.f);
			const auto far = Enemy(f, 200.f);
			far->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 0.f, 400.f });
			far->GetHealthComponent().ChangeHealth(-700.f);
			const Volley volley = FireVolley(f, f.ship->GetAbilitySystemComponent().GetPrimaryMountProfile());
			report.Check(!volley.projectiles.empty(), "nearest-target volley fires");
			for (const auto& projectile : volley.projectiles) report.Check(Along(projectile->GetActorForwardDirection(), kEast), "without Execution Priority the nearest enemy wins");
		}
		{
			Fixture f{ report, application };
			f.Install(kTurretId, kTurretL, 1, kExecutionPriorityId);
			f.ship->SetActorRotation(0.f);
			Enemy(f, 200.f);
			const auto far = Enemy(f, 200.f);
			far->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 0.f, 400.f });
			far->GetHealthComponent().ChangeHealth(-700.f);
			report.Check(f.ship->GetAbilitySystemComponent().GetPrimaryMountProfile().executionPriority, "installed Execution Priority reaches the ability component");
			const Volley volley = FireVolley(f, f.ship->GetAbilitySystemComponent().GetPrimaryMountProfile());
			report.Check(!volley.projectiles.empty(), "Execution Priority volley fires");
			for (const auto& projectile : volley.projectiles) report.Check(Along(projectile->GetActorForwardDirection(), kSouth), "Execution Priority aims at the lowest-health enemy, not the nearest");
		}
		// Primary damage coefficient 0.80 / 0.83 / 0.86 at the placement level plus its zone bonus, on both sizes.
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto id = f.Install(kTurretId, kDomino, level);
			const int zone = ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), id);
			f.Equal("Turret domino damage multiplier follows its level table", TableAt(0.80f, 0.83f, 0.86f, level + zone), PrimaryDamageMultiplier(f, id));
		}
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto id = f.Install(kTurretId, kTurretL, level);
			const int zone = ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), id);
			f.Equal("Turret L damage multiplier follows its level table", TableAt(0.80f, 0.83f, 0.86f, level + zone), PrimaryDamageMultiplier(f, id));
		}
		// Uninstalling the Turret clears the mount profile.
		{
			Fixture f{ report, application };
			const auto id = f.Install(kTurretId, kTurretL, 1);
			report.Check(f.ship->GetAbilitySystemComponent().GetPrimaryMountProfile().turret, "Turret is active before removal");
			f.Remove(id);
			report.Check(!f.ship->GetAbilitySystemComponent().GetPrimaryMountProfile().turret, "uninstalling the Turret clears the mount profile");
		}
	}

	void BroadsideChecks(Report& report, Application& application)
	{
		// Baseline: a plain Primary volley fires forward and sets the undivided damage.
		Fixture plain{ report, application };
		plain.ship->SetActorRotation(0.f);
		const Volley forward = FireVolley(plain, PrimaryWeaponMountProfile{});
		report.Check(!forward.projectiles.empty(), "plain Primary volley spawns projectiles");
		for (const auto& projectile : forward.projectiles) report.Check(Along(projectile->GetActorForwardDirection(), kNorth), "without a mount the Primary fires forward");
		const float plainDamage = forward.projectiles.empty() ? 0.f : forward.projectiles.front()->GetDamage();
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			f.ship->SetActorRotation(0.f);
			const auto id = f.Install(kBroadsideId, kBroadsideL, level);
			const PrimaryWeaponMountProfile profile = f.ship->GetAbilitySystemComponent().GetPrimaryMountProfile();
			const int zone = ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), id);
			const float coefficient = TableAt(0.70f, 0.75f, 0.80f, level + zone);
			report.Check(profile.broadside && !profile.turret, "installed Broadside reaches the ability component");
			f.Equal("Broadside side coefficient follows its level table", coefficient, profile.broadsideDamageMultiplier);
			const Volley volley = FireVolley(f, profile);
			report.Check(volley.projectiles.size() == 2 * forward.projectiles.size(), "each volley fires once from each side of the ship");
			bool east = false;
			bool west = false;
			bool ahead = false;
			bool scaled = !volley.projectiles.empty();
			for (const auto& projectile : volley.projectiles)
			{
				const sf::Vector2f direction = projectile->GetActorForwardDirection();
				east = east || Along(direction, kEast);
				west = west || Along(direction, kWest);
				ahead = ahead || Along(direction, kNorth);
				scaled = scaled && Near(projectile->GetDamage(), plainDamage * coefficient, 0.05f);
			}
			report.Check(east && west && !ahead, "Broadside volleys go out east and west, never forward");
			report.Check(scaled, "each Broadside side volley deals its coefficient of the plain Primary damage");
		}
		{
			Fixture f{ report, application };
			const auto id = f.Install(kBroadsideId, kBroadsideL, 1);
			report.Check(f.ship->GetAbilitySystemComponent().GetPrimaryMountProfile().broadside, "Broadside is active before removal");
			f.Remove(id);
			report.Check(!f.ship->GetAbilitySystemComponent().GetPrimaryMountProfile().broadside, "uninstalling the Broadside clears the mount profile");
		}
	}
}

int main(int argc, char** argv)
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Epic Primary content registration failed" };
		std::string title = "Epic Primary E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		ContentChecks(report);
		TurretChecks(report, application);
		BroadsideChecks(report, application);
		OtherPrimaryChecks(report, application);
		HeldBeamMountChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "EpicPrimaryModulesE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
