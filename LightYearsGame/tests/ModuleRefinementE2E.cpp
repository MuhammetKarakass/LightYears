#include "framework/EngineContext.h"
#include "gameConfigs/CollisionLayers.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "gameplay/ability/infernoSpray/InfernoSprayActor.h"
#include "gameplay/ability/infernoSpray/InfernoSprayContracts.h"
#include "presentation/ability/infernoSpray/InfernoSprayPresentationProfile.h"

namespace
{
	using namespace uncommon_p6_e2e;
	std::uint32_t InstallInZone(Fixture& f, const char* id, const GameplayTag& zone)
	{
		const auto* definition = f.rules.lookup(id); const auto instance = f.loadout.AddToInventory(f.rules, *definition, 0, definition->rarity, 1);
		ModulePlacement candidate = *f.loadout.FindInventoryItem(instance);
		for (int rotation = 0; rotation < 4; ++rotation)
			for (int y = 0; y < f.rules.schematic.height; ++y)
				for (int x = 0; x < f.rules.schematic.width; ++x)
				{
					candidate.origin = { x, y }; candidate.rotationQuarterTurns = rotation;
					if (ModulePlacementRules::CanPlace(f.rules, f.loadout, candidate) != ModulePlacementFailure::None) continue;
					const auto cells = ModulePlacementRules::GetCoveredCells(*definition, candidate);
					if (!std::all_of(cells.begin(), cells.end(), [&](const auto& cell) { return f.rules.schematic.GetZoneAt(cell.x, cell.y) == zone; })) continue;
					f.report.Check(f.loadout.Install(f.rules, instance, candidate.origin, rotation) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "install shipped module entirely in its selected zone");
					return instance;
				}
		throw std::runtime_error{ "no legal placement in selected zone" };
	}
	void HuntChecks(Report& report, Application& application)
	{
		const char* families[] = { "Offense", "Defense", "Utility", "Movement" };
		const char* ids[] = { "Ability.Offense.Rocket.Basic", "Ability.Defense.DirectionalBarrier.Basic", "Ability.Utility.GravityAnomaly.Basic", "Ability.Movement.Dash.Basic" };
		const sas::AbilitySlot slots[] = { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 };
		for (int family = 0; family < 4; ++family)
			for (int level = 1; level <= 3; ++level)
			{
				Fixture f{ report, application }; auto& asc = f.ship->GetAbilitySystemComponent();
				GameAbility* abilities[4]{};
				for (const auto slot : slots) if (auto* previous = asc.GetAbility(slot)) report.Check(asc.RemoveAbility(previous->GetHandle()), "clear existing slots before granting family abilities");
				for (int index = 0; index < 4; ++index)
				{
					auto definition = *content::AbilityContentCatalog::FindById(ids[index]); definition.slot = slots[index];
					report.Check(asc.GrantAbility(definition).IsValid(), "grant a shipped ability of each family"); abilities[index] = asc.GetAbility(slots[index]);
					report.Check(abilities[index] != nullptr, "family ability is bound to its intended slot");
				}
				const auto clock = f.Install((std::string{ "Module." } + families[family] + ".HuntClock.Uncommon").c_str(), 1, level, "Module.SideFeature.HuntClock.FifthKill");
				for (auto* ability : abilities)
				{
					report.Check(ability->TryActivate(), "activate each real ability before cooldown refunds");
					if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
				}
				for (int kill = 1; kill <= 5; ++kill)
				{
					float expected[4]; for (int index = 0; index < 4; ++index) expected[index] = abilities[index]->GetCooldownRemaining();
					expected[family] *= 1.f - (.04f + .01f * level); if (kill == 5) expected[family] *= .8f;
					f.Kill();
					for (int index = 0; index < 4; ++index) f.Equal("Hunt Clock and Fifth Kill refund only their own family", expected[index], abilities[index]->GetCooldownRemaining());
				}
				f.Remove(clock);
			}
	}
	void ZoneChecks(Report& report, Application& application)
	{
		{
			Fixture f{ report, application };
			const auto placements = f.loadout.GetPlacements();
			for (const auto& placement : placements) if (f.rules.lookup(placement.moduleId)->systemCategory == GameplayTags::Module::System::Afterburner) f.Remove(placement.instanceId);
			const float base = f.Owner(OwnerAttributeIds::AttackSpeed);
			InstallInZone(f, "Module.Movement.MovementRhythm.Uncommon", GameplayTags::Module::Zone::Engine);
			Move(f, 20.f, .1f); f.Equal("ordinary Movement Rhythm step has its authored AS percentage", base + 3.5f, f.Owner(OwnerAttributeIds::AttackSpeed));
			const auto amplifier = InstallInZone(f, "Module.Utility.ZoneOverclocker.Rare", GameplayTags::Module::Zone::Engine);
			Move(f, 20.f, .1f); f.Equal("Overclocker reaches a live owner attribute rule", base + 3.5f * 1.25f, f.Owner(OwnerAttributeIds::AttackSpeed));
			f.Remove(amplifier); Move(f, 20.f, .1f); f.Equal("removing Overclocker restores the ordinary AS step", base + 3.5f, f.Owner(OwnerAttributeIds::AttackSpeed));
		}
		{
			Fixture f{ report, application };
			InstallInZone(f, "Module.Offense.HuntClock.Uncommon", GameplayTags::Module::Zone::Wings);
			InstallInZone(f, "Module.Utility.ZoneOverclocker.Rare", GameplayTags::Module::Zone::Wings);
			auto& ability = CastOffense(f); const float pending = ability.GetCooldownRemaining();
			f.Kill(); f.Equal("Overclocker reaches a real family kill refund", pending * (1.f - .05f * 1.25f), ability.GetCooldownRemaining());
		}
		Fixture f{ report, application }; const auto enemy = Enemy(f);
		const auto lens = f.Install("Module.Primary.PhotonicLens.Uncommon");
		const auto placed = FindInstalled(f.loadout, lens); const auto zone = f.rules.schematic.GetZoneAt(placed.origin.x, placed.origin.y);
		f.Equal("Lens level one grants fifty percent to Primary", 150.f, Hit(*enemy, f.ship.get(), 100.f, Plain(), {}, { GameplayTags::Ability::Primary }));
		const auto* definition = f.rules.lookup("Module.Utility.ZoneOverclocker.Rare");
		const auto amplifier = f.loadout.AddToInventory(f.rules, *definition, 0, definition->rarity, 1);
		ModulePlacement candidate = *f.loadout.FindInventoryItem(amplifier); bool installed = false;
		for (int y = 0; y < f.rules.schematic.height && !installed; ++y)
			for (int x = 0; x < f.rules.schematic.width && !installed; ++x)
			{
				candidate.origin = { x, y };
				if (f.rules.schematic.GetZoneAt(x, y) != zone || ModulePlacementRules::CanPlace(f.rules, f.loadout, candidate) != ModulePlacementFailure::None) continue;
				installed = f.loadout.Install(f.rules, amplifier, candidate.origin, 0) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules);
			}
		report.Check(installed, "install the real Overclocker in the Lens zone");
		f.Equal("Overclocker reaches a Primary impact rule", 162.5f, Hit(*enemy, f.ship.get(), 100.f, Plain(), {}, { GameplayTags::Ability::Primary }));
		float bonus = 0.f; for (const auto& value : f.ship->GetModuleApplier().GetResolvedEffects().zoneBonuses) if (value.instanceId == lens) bonus = value.bonus;
		f.Equal("runtime exposes the amplified zone bonus for the module screen", .5f, bonus);
		f.Remove(amplifier);
		f.Equal("removing Overclocker restores the ordinary Lens rule", 150.f, Hit(*enemy, f.ship.get(), 100.f, Plain(), {}, { GameplayTags::Ability::Primary }));
	}
	void CoreChecks(Report& report, Application& application)
	{
		Fixture f{ report, application }; const auto enemy = Enemy(f);
		const auto* authored = content::AbilityContentCatalog::FindById("Ability.Offense.InfernoSpray.Basic");
		enemy->SetCollisionMask(vega::GameCollision::PlayerBullet);
		auto definition = *authored; definition.slot = sas::AbilitySlot::Ability3;
		auto& asc = f.ship->GetAbilitySystemComponent();
		report.Check(asc.RemoveAbility(asc.GetAbility(sas::AbilitySlot::Ability3)->GetHandle()) && asc.GrantAbility(definition).IsValid(), "remaining ability is shipped Inferno Spray");
		f.Install("Module.Utility.TripleCore.Rare");
		enemy->SetActorLocation(f.ship->GetActorLocation() + f.ship->GetActorForwardDirection() * 200.f);
		asc.AddOwnedTag(GameplayTags::State::Ability::InfernoSpray::Active);
		const auto spray = [&]()
		{
			const float before = enemy->GetHealthComponent().GetHealth();
			const auto actor = f.level->SpawnActor<InfernoSprayActor>(f.ship.get(), InfernoSprayPresentationProfile{}).lock();
			actor->ConfigureFromAttributes({ { AbilityData::InfernoSpray::Actor::FlameCone::Range, 400.f, 0.f }, { AbilityData::InfernoSpray::Actor::FlameCone::ConeAngle, 36.f, 0.f }, { AbilityData::InfernoSpray::Actor::FlameCone::BaseDPS, 40.f, 0.f } });
			actor->SetSourceAbility(sas::ContentId{ definition.abilityId }, definition.abilityTags); actor->SetDamagePayload(Plain()); actor->Tick(.25f); actor->Destroy();
			return before - enemy->GetHealthComponent().GetHealth();
		};
		const float base = spray(); report.Check(base > 0.f, "real Inferno cone deals baseline damage");
		const auto placements = f.loadout.GetPlacements();
		for (const auto& placement : placements)
		{
			const auto slot = AbilitySlotForModuleSystem(f.rules.lookup(placement.moduleId)->systemCategory);
			if (slot != sas::AbilitySlot::None && slot != sas::AbilitySlot::Ability3) f.Remove(placement.instanceId);
		}
		f.Equal("Triple Core boosts the only remaining real Inferno ability", base * 1.25f, spray());
		f.Equal("Triple Core also boosts the remaining ability projectile boundary", 125.f, Hit(*enemy, f.ship.get(), 100.f, Plain(), sas::ContentId{ definition.abilityId }, definition.abilityTags, DamageDeliveryType::Projectile));
		f.Equal("Triple Core excludes Primary", 100.f, Hit(*enemy, f.ship.get(), 100.f, Plain(), {}, { GameplayTags::Ability::Primary }));
	}
	void ProjectileChecks(Report& report, Application& application)
	{
		Fixture f{ report, application }; const auto module = f.Install("Module.Primary.RicochetTip.Uncommon");
		const auto first = Enemy(f, 100.f); const auto second = Enemy(f, 500.f);
		PrimaryWeaponShotMetadata shot; shot.criticalPolicy = DamageCriticalPolicy::Disabled;
		const auto parent = PrimaryHit(f, *first, shot, 100.f, false);
		const auto bounces = Actors<ModuleRicochetProjectileActor>(f); report.Check(bounces.size() == 1, "Ricochet creates a travelling Primary projectile");
		const auto bounce = bounces.front();
		f.Equal("Ricochet preserves actual source projectile speed", parent->GetProjectileSpeed(), bounce->GetProjectileSpeed());
		report.Check(bounce->GetPresentationDefinition().texturePath == parent->GetPresentationDefinition().texturePath && bounce->GetPresentationDefinition().visualScale == parent->GetPresentationDefinition().visualScale, "Ricochet copies texture and visual scale from the source Primary");
		report.Check(Actors<ModuleGuidedProjectileActor>(f).empty(), "Ricochet does not use the Scatter sphere actor");
		f.Equal("nearest enemy is not damaged before the projectile travels", 1000.f, second->GetHealthComponent().GetHealth());
		for (int step = 0; step < 100 && !bounce->GetIsPendingDestroy(); ++step) bounce->Tick(.01f);
		f.Equal("travelling Ricochet reaches the enemy four hundred units away", 975.f, second->GetHealthComponent().GetHealth());
		f.Remove(module);
	}
	void CfgChecks(Report& report, Application& application)
	{
		const float rates[] = { .20f, .35f, .45f };
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; f.Install("Module.Defense.ContinuousFieldGenerator.Rare", 0, level);
			const float regen = f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen);
			ApplyCombatDamage(*f.ship, 20.f, nullptr, {}, Plain()); const float shield = f.ship->GetShieldComponent().GetShield();
			f.ship->Tick(.5f);
			report.CheckValue("CFG level " + std::to_string(level) + " regenerates at its revised rate during damage delay", regen * rates[level - 1] * .5f, f.ship->GetShieldComponent().GetShield() - shield, Json{ { "level", level }, { "rate", rates[level - 1] } }.dump());
		}
	}
	void ContentChecks(Report& report, Application& application)
	{
		const auto* omni = content::ModuleContentCatalog::FindById("Module.Movement.OmniThrusters.Uncommon");
		report.Check(omni && omni->rarity == ModuleRarity::Uncommon, "Omni is shipped as Uncommon");
		report.Check(!content::ModuleContentCatalog::FindById("Module.Movement.OmniThrusters.Rare"), "old Omni identity is removed");
		CfgChecks(report, application);
		for (const char* file : { "modules.json", "module_side_features.json" })
		{
			Json data; std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data" / file }; input >> data;
			for (const auto& record : data.at(std::string{ file } == "modules.json" ? "modules" : "sideFeatures"))
			{
				const std::string description = record.value("description", std::string{});
				if (description.find("Attack Speed") == std::string::npos || (description.find("{Owner.AttackSpeed}") == std::string::npos && description.find("{owner.0.amount}") == std::string::npos)) continue;
				report.Check(description.find('%') != std::string::npos, "Attack Speed module descriptions explicitly show percent", description);
			}
		}
	}
}
int main(int argc, char** argv)
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	if (argc != 2 && !(argc == 3 && std::string{ argv[2] } == "cfg")) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "module refinement content registration failed" };
		std::string title = "Module refinement E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		if (argc == 3) CfgChecks(report, application);
		else { HuntChecks(report, application); ZoneChecks(report, application); CoreChecks(report, application); ProjectileChecks(report, application); ContentChecks(report, application); }
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "ModuleRefinementE2E"; WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
