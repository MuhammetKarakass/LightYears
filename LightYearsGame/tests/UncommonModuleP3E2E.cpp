#include "CommonModuleE2ETestSupport.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"
#include "gameplay/module/ModuleReactiveRuntime.h"

namespace
{
	using namespace ly;
	using namespace common_module_e2e;
	void ContentChecks(Report& report)
	{
		const auto rejects = [&report](const char* label, const Json& json)
		{
			bool rejected = false;
			try { content::ModuleLoader::ParseRulePackage(json, "P3 invalid fixture", 3, false); }
			catch (const std::exception&) { rejected = true; }
			report.Check(rejected, label, json);
		};
		const Json validAction = { { "trigger", "KillConfirmed" }, { "kind", "RestoreShield" }, { "amount", { .02f, .025f, .03f } } };
		Json entry = validAction;
		entry["kind"] = "Unknown";
		rejects("loader rejects unknown action kind", Json{ { "eventActions", Json::array({ entry }) } });
		entry = validAction; entry["amount"] = { .02f, .025f };
		rejects("loader rejects short action level tables", Json{ { "eventActions", Json::array({ entry }) } });
		entry = validAction; entry["amount"] = -1;
		rejects("loader rejects negative recovery", Json{ { "eventActions", Json::array({ entry }) } });
		entry = validAction; entry["everyCount"] = 0;
		rejects("loader rejects zero event count", Json{ { "eventActions", Json::array({ entry }) } });
		entry = validAction; entry["trigger"] = "AbilityActivated";
		rejects("loader rejects incompatible action trigger", Json{ { "eventActions", Json::array({ entry }) } });
		entry = validAction; entry["chancePerLuck"] = .05f;
		rejects("loader rejects Luck field on recovery", Json{ { "eventActions", Json::array({ entry }) } });
		entry = { { "trigger", "AbilityActivated" }, { "kind", "RefundUsedCooldown" }, { "amount", .5f } };
		rejects("refund requires a Luck coefficient", Json{ { "eventActions", Json::array({ entry }) } });
		entry["chancePerLuck"] = { .05f, -.06f, .07f };
		rejects("loader validates every Luck table entry", Json{ { "eventActions", Json::array({ entry }) } });
		entry = { { "source", "HigherPower" }, { "attributeId", "Owner.AttackPower" }, { "amount", { 35, 35 } }, { "healthAtMost", .35f } };
		rejects("loader rejects short conditional attribute tables", Json{ { "ownerAttributeRules", Json::array({ entry }) } });
		entry["amount"] = 35; entry["healthAtMost"] = 2;
		rejects("loader rejects invalid health threshold", Json{ { "ownerAttributeRules", Json::array({ entry }) } });
		entry = { { "incoming", true }, { "condition", "PrimaryShotOnly" }, { "damageMultiplier", .8f } };
		rejects("incoming rule cannot use an outgoing condition", Json{ { "damageRules", Json::array({ entry }) } });
		entry = { { "incoming", true }, { "condition", "OwnerHealthAtMost" }, { "conditionValue", .35f }, { "damageMultiplier", .8f }, { "bonusDamageFraction", .1f }, { "bonusDamageType", "Damage.Type.Energy" } };
		rejects("incoming rule cannot generate outgoing bonus hits", Json{ { "damageRules", Json::array({ entry }) } });
		const auto package = content::ModuleLoader::ParseRulePackage(Json{ { "eventActions", Json::array({ validAction }) } }, "P3 valid fixture", 3, false);
		report.Check(package.eventActions.size() == 1 && HasModuleRulePackageLevelCoverage(package, 3) && !HasModuleRulePackageLevelCoverage(package, 4), "resolver coverage includes new action tables");
	}
	struct Fixture
	{
		Report& report;
		shared_ptr<HeadlessModuleLevel> level;
		shared_ptr<PlayerSpaceShip> ship;
		ModuleRulesContext rules;
		ModuleLoadout loadout;
		Fixture(Report& result, Application& application) : report{ result }, level{ std::make_shared<HeadlessModuleLevel>(&application) }, rules{ *content::ShipContentCatalog::GetPlayerFighterDefinition().schematic, [](const sas::ContentId& id) { return content::ModuleContentCatalog::FindById(id); }, content::ModuleContentCatalog::GetZoneTable(), [](const sas::ContentId& id) { return content::ModuleSideFeatureCatalog::FindById(id); } }
		{
			level->BeginPlayInternal();
			const auto& definition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
			ship = level->SpawnActor<PlayerSpaceShip>(definition).lock();
			report.Check(ship != nullptr, "P3 spawns a real player ship");
			level->TickInternal(0.f);
			ship->SetInvulnerability(false);
			ship->SetUseScreenClamp(false);
			ship->SetActorLocation({ 10000.f, 10000.f });
			report.Check(loadout.SeedDefaults(rules) && ship->GetModuleApplier().Apply(loadout, rules), "P3 uses canonical default modules and runtime applier");
		}
		std::uint32_t Install(const char* id, int variant = 0, int levelValue = 1, const char* side = "")
		{
			const auto* definition = rules.lookup(id);
			report.Check(definition != nullptr, std::string{ "P3 shipped module: " } + id);
			const auto instance = loadout.AddToInventory(rules, *definition, variant, definition->rarity, levelValue, sas::ContentId{ side });
			report.Check(instance != 0, "P3 inventory validates variant, level and feature");
			ModulePlacement candidate = *loadout.FindInventoryItem(instance);
			for (int rotation = 0; rotation < 4; ++rotation)
				for (int y = 0; y < rules.schematic.height; ++y)
					for (int x = 0; x < rules.schematic.width; ++x)
					{
						candidate.origin = { x, y };
						candidate.rotationQuarterTurns = rotation;
						if (ModulePlacementRules::CanPlace(rules, loadout, candidate) != ModulePlacementFailure::None) continue;
						report.Check(loadout.Install(rules, instance, candidate.origin, rotation) == ModulePlacementFailure::None && ship->GetModuleApplier().Apply(loadout, rules), "P3 installs through real placement and applier");
						return instance;
					}
			throw std::runtime_error{ "No legal P3 placement" };
		}
		void Remove(std::uint32_t id)
		{
			report.Check(loadout.Uninstall(id) && ship->GetModuleApplier().Apply(loadout, rules), "P3 removes module through the real applier");
		}
		shared_ptr<SpaceShip> Kill(Actor* source = nullptr, bool generated = false)
		{
			const auto& definition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
			const auto enemy = level->SpawnActor<SpaceShip>(definition).lock();
			report.Check(enemy != nullptr, "P3 creates a real enemy combatant");
			level->TickInternal(0.f);
			enemy->SetCollisionLayer(CollisionLayer::Enemy);
			enemy->SetActorLocation({ 20000.f, 20000.f });
			DamagePayload payload;
			payload.criticalPolicy = DamageCriticalPolicy::Disabled;
			payload.kineticStacks = 0;
			payload.skipModuleDamageRules = generated;
			ApplyCombatDamage(*enemy, 100000.f, source ? source : ship.get(), {}, payload);
			report.Check(enemy->GetHealthComponent().GetHealth() == 0.f, "P3 confirms kill through actual damage and health resolution");
			return enemy;
		}
		float Owner(const sas::AttributeId& id) const { return ship->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(id); }
		void Equal(const char* name, float expected, float actual) { report.CheckValue(name, expected, actual, "P3 live ship, canonical loadout and content"); }
	};
	void HealthChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& health = f.ship->GetHealthComponent();
		health.ChangeHealth(50.f - health.GetHealth());
		const float beforeMax = health.GetMaxHealth();
		const auto id = f.Install("Module.Defense.GritYourTeeth.Uncommon", 1, 1, "Module.SideFeature.Shared.D1");
		f.Equal("adding MaxHealth preserves the current health number", 50.f, health.GetHealth());
		report.Check(health.GetMaxHealth() > beforeMax, "D1 really raises MaxHealth");
		f.Remove(id);
		f.Equal("removing MaxHealth preserves health below the new cap", 50.f, health.GetHealth());
		const auto glass = f.Install("Module.Offense.GlassTopCore.Uncommon", 1);
		f.Equal("Glass Cannon penalty does not scale current health", 50.f, health.GetHealth());
		f.Remove(glass);
		f.Equal("removing Glass Cannon does not heal", 50.f, health.GetHealth());
		const auto again = f.Install("Module.Defense.GritYourTeeth.Uncommon", 1, 1, "Module.SideFeature.Shared.D1");
		health.Regenerate(10000.f);
		f.Remove(again);
		f.Equal("removing MaxHealth clamps health to the reduced cap", health.GetMaxHealth(), health.GetHealth());
		auto fixture = std::make_shared<ModuleDefinition>(*f.rules.lookup("Module.Offense.GlassTopCore.Uncommon"));
		fixture->sizeVariants[1].attributeModifiers[1].magnitudeByLevel.values = { .94f, .84f, .74f };
		f.rules.lookup = [fixture](const sas::ContentId& id) { return id == fixture->moduleId ? fixture.get() : content::ModuleContentCatalog::FindById(id); };
		health.ChangeHealth(beforeMax * .90f - health.GetHealth());
		const auto batch = f.Install("Module.Offense.GlassTopCore.Uncommon", 1);
		report.Check(f.loadout.SetLevel(f.rules, batch, 2) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "fixture changes MaxHealth multiplier through a real loadout level mutation");
		f.Equal("modifier replacement clips only to the final MaxHealth, not the transient product", beforeMax * .84f, health.GetHealth());
		report.Check(f.loadout.SetLevel(f.rules, batch, 1) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "fixture raises MaxHealth through the same level mutation");
		f.Equal("raising MaxHealth during replacement grants no free healing", beforeMax * .84f, health.GetHealth());
		f.Remove(batch);
	}
	void KillWardChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto* module = f.rules.lookup("Module.Defense.LootShield.Uncommon");
			report.Check(module && module->sideFeatures.size() == 2 && module->sharedPools.front().allowedFeatureIds.size() == 3, "Loot Shield restores its two-private three-shared choice contract");
			const auto damage = [&f]()
			{
				auto& health = f.ship->GetHealthComponent();
				health.ChangeHealth(health.GetMaxHealth() - health.GetHealth());
				f.ship->GetShieldComponent().ChangeShield(-10000.f);
				DamagePayload payload; payload.criticalPolicy = DamageCriticalPolicy::Disabled; payload.kineticStacks = 0;
				const float before = health.GetHealth();
				ApplyCombatDamage(*f.ship, 20.f, nullptr, {}, payload);
				return before - health.GetHealth();
			};
			const float baseline = damage();
			for (int variant : { 0, 1 })
			{
				const auto locked = f.Install("Module.Defense.LootShield.Uncommon", variant, level, "Module.SideFeature.LootShield.KillWard");
				f.Kill(); f.Equal("Kill Ward remains locked below maximum size", baseline, damage()); f.Remove(locked);
			}
			const auto ward = f.Install("Module.Defense.LootShield.Uncommon", 2, level, "Module.SideFeature.LootShield.KillWard");
			f.Equal("equipping Kill Ward does not start its window", baseline, damage());
			const auto other = f.level->SpawnActor<PlayerSpaceShip>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock();
			f.Kill(other.get()); f.Equal("another owner's kill does not activate Kill Ward", baseline, damage());
			f.Kill(); f.Equal("owner kill reduces incoming hull damage ten percent", baseline * .9f, damage());
			f.ship->GetCombatRuntime().Tick(1.5f);
			f.Kill(); f.Equal("another kill refreshes rather than stacks Kill Ward", baseline * .9f, damage());
			f.ship->GetCombatRuntime().Tick(.6f);
			f.Equal("refreshed Kill Ward survives the original expiry", baseline * .9f, damage());
			f.ship->GetCombatRuntime().Tick(1.41f);
			f.Equal("Kill Ward expires two seconds after the latest kill", baseline, damage());
			f.Kill(); f.Remove(ward); f.Equal("removing active Kill Ward clears its damage reduction", baseline, damage());
		}
	}
	void KillChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& shield = f.ship->GetShieldComponent();
		const std::size_t baseline = f.ship->GetModuleApplier().GetOwnedHandleCount();
		const float regen = f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen);
		const auto lockedRegen = f.Install("Module.Defense.LootShield.Uncommon", 0, 1, "Module.SideFeature.Shared.D4");
		f.Equal("D4 is locked on smaller Loot Shield", regen, f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen));
		f.Remove(lockedRegen);
		const auto bigRegen = f.Install("Module.Defense.LootShield.Uncommon", 2, 1, "Module.SideFeature.Shared.D4");
		f.Equal("D4 uses the existing ship modifier path", regen * 1.15f, f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen));
		f.Remove(bigRegen);
		const float armor = f.Owner(OwnerAttributeIds::Armor);
		const auto guard = f.Install("Module.Defense.LootShield.Uncommon", 2, 1, "Module.SideFeature.Shared.D2");
		DamagePayload hit;
		hit.criticalPolicy = DamageCriticalPolicy::Disabled;
		hit.kineticStacks = 0;
		ApplyCombatDamage(*f.ship, 1.f, nullptr, {}, hit);
		f.Equal("D2 reuses DamageTaken instead of introducing another behavior", armor + 12.f, f.Owner(OwnerAttributeIds::Armor));
		f.Remove(guard);
		f.Equal("removing D2 clears its active armor buff", armor, f.Owner(OwnerAttributeIds::Armor));
		for (int variant = 0; variant < 3; ++variant)
			for (int level = 1; level <= 3; ++level)
			{
				const auto id = f.Install("Module.Defense.LootShield.Uncommon", variant, level);
				shield.ChangeShield(-10000.f);
				f.Kill();
				const float fraction = variant == 0 ? .02f + .005f * (level - 1) : .03f + .01f * (level - 1);
				f.Equal("Loot Shield reads size and level table on owner kill", shield.GetMaxShield() * fraction, shield.GetShield());
				f.Remove(id);
			}
		auto& energy = f.ship->GetEnergyComponent();
		energy.Consume(10000.f);
		const auto noFuel = f.Install("Module.Defense.LootShield.Uncommon", 2, 1);
		f.Kill();
		f.Equal("Loot Shield kills no longer restore Afterburner", 0.f, energy.GetEnergy());
		f.Remove(noFuel);
		const auto overflow = f.Install("Module.Defense.LootShield.Uncommon", 2, 1, "Module.SideFeature.LootShield.OverflowRecovery");
		shield.ChangeShield(10000.f);
		auto& health = f.ship->GetHealthComponent();
		health.ChangeHealth(50.f - health.GetHealth());
		f.Kill();
		f.Equal("full shield converts only the actual overflow to health", 50.f + shield.GetMaxShield() * .03f * .5f, health.GetHealth());
		shield.ChangeShield(-10000.f);
		health.ChangeHealth(50.f - health.GetHealth());
		f.Kill();
		f.Equal("overflow feature does not grant shield twice", shield.GetMaxShield() * .03f, shield.GetShield());
		f.Equal("no overflow means no healing", 50.f, health.GetHealth());
		const auto other = f.level->SpawnActor<PlayerSpaceShip>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock();
		const float before = shield.GetShield();
		f.Kill(other.get());
		f.Equal("another actor's kill does not replenish owner's shield", before, shield.GetShield());
		f.Kill(nullptr, true);
		f.Equal("module-generated kills do not trigger another module rule", before, shield.GetShield());
		f.Remove(overflow);
		const float speed = f.Owner(OwnerAttributeIds::AttackSpeed);
		const auto o3 = f.Install("Module.Offense.SharpScale.Uncommon", 1, 1, "Module.SideFeature.Shared.O3");
		f.Equal("O3 is a flat Attack Speed bonus from the moment it is equipped", speed + 4.f, f.Owner(OwnerAttributeIds::AttackSpeed));
		f.Kill();
		f.Equal("O3 does not change on kill", speed + 4.f, f.Owner(OwnerAttributeIds::AttackSpeed));
		f.ship->GetModuleTriggerRuntime().Tick(5.f);
		f.Equal("O3 does not expire", speed + 4.f, f.Owner(OwnerAttributeIds::AttackSpeed));
		f.Remove(o3);
		f.Equal("O3 releases its modifier when removed", speed, f.Owner(OwnerAttributeIds::AttackSpeed));
		const auto d3 = f.Install("Module.Defense.GritYourTeeth.Uncommon", 1, 1, "Module.SideFeature.Shared.D3");
		health.ChangeHealth(50.f - health.GetHealth());
		f.Kill();
		f.Equal("D3 restores one percent of actual MaxHealth", 50.f + health.GetMaxHealth() * .01f, health.GetHealth());
		f.Remove(d3);
		for (int cycle = 0; cycle < 10; ++cycle)
		{
			const auto id = f.Install("Module.Defense.GritYourTeeth.Uncommon", 1, 1, "Module.SideFeature.Shared.D3");
			f.Kill();
			f.Remove(id);
			report.Check(f.ship->GetModuleTriggerRuntime().GetActiveModifierHandleCount() == 0 && f.ship->GetModuleApplier().GetOwnedHandleCount() == baseline, "ten equip/kill/remove cycles leave no extra handles");
		}
	}
	void StateChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& health = f.ship->GetHealthComponent();
		auto& attributes = f.ship->GetAbilitySystemComponent().GetAttributes();
		attributes.SetBaseValue(OwnerAttributeIds::Armor, 0.f);
		const auto grit = f.Install("Module.Defense.GritYourTeeth.Uncommon", 1, 1, "Module.SideFeature.GritYourTeeth.CriticalCircuit");
		attributes.SetBaseValue(OwnerAttributeIds::AttackPower, 20.f);
		attributes.SetBaseValue(OwnerAttributeIds::EnergyPower, 10.f);
		const float maximum = health.GetMaxHealth();
		health.ChangeHealth(maximum * .35f - health.GetHealth());
		f.Equal("Critical Circuit picks AP at low health", 55.f, f.Owner(OwnerAttributeIds::AttackPower));
		attributes.SetBaseValue(OwnerAttributeIds::EnergyPower, 25.f);
		f.Equal("Critical Circuit comparison excludes its own bonus", 20.f, f.Owner(OwnerAttributeIds::AttackPower));
		f.Equal("Critical Circuit moves to the higher EP", 60.f, f.Owner(OwnerAttributeIds::EnergyPower));
		attributes.SetBaseValue(OwnerAttributeIds::EnergyPower, 20.f);
		f.Equal("Critical Circuit chooses AP on a tie", 55.f, f.Owner(OwnerAttributeIds::AttackPower));
		f.ship->GetShieldComponent().ChangeShield(-10000.f);
		DamagePayload payload;
		payload.criticalPolicy = DamageCriticalPolicy::Disabled;
		payload.kineticStacks = 0;
		for (int level = 1; level <= 3; ++level)
		{
			report.Check(f.loadout.SetLevel(f.rules, grit, level) && (f.ship->GetModuleApplier().Apply(f.loadout, f.rules) || level == 1), "Grit level resolves through loadout");
			health.ChangeHealth(maximum * .35f - health.GetHealth());
			const float before = health.GetHealth();
			ApplyCombatDamage(*f.ship, 10.f, nullptr, {}, payload);
			f.Equal("Grit reduces actual incoming damage at the pre-hit threshold", 10.f * (.8f - .03f * (level - 1)), before - health.GetHealth());
		}
		health.Regenerate(maximum * .5f - health.GetHealth());
		f.Equal("leaving health threshold removes conditional bonus immediately", 20.f, f.Owner(OwnerAttributeIds::AttackPower));
		const float before = health.GetHealth();
		ApplyCombatDamage(*f.ship, 10.f, nullptr, {}, payload);
		f.Equal("above threshold Grit does not reduce incoming damage", 10.f, before - health.GetHealth());
		health.ChangeHealth(maximum * .3f - health.GetHealth());
		f.Remove(grit);
		f.Equal("removing Grit while its bonus is active releases the power modifier", 20.f, f.Owner(OwnerAttributeIds::AttackPower));
		const float speedBase = f.Owner(OwnerAttributeIds::AttackSpeed);
		const std::size_t handles = f.ship->GetModuleApplier().GetOwnedHandleCount();
		for (int cycle = 0; cycle < 10; ++cycle)
		{
			const auto movement = f.Install("Module.Movement.MovementRhythm.Uncommon", 1, 3);
			f.ship->SetVelocity({ 300.f, 400.f });
			f.ship->GetMovementComponent().Tick(.1f);
			f.ship->GetModuleApplier().Tick(.1f);
			f.Equal("Movement Rhythm uses complete steps above the two-hundred threshold", speedBase + 18.f, f.Owner(OwnerAttributeIds::AttackSpeed));
			f.ship->SetVelocity({ 0.f, 0.f });
			f.ship->GetMovementComponent().Tick(.1f);
			f.ship->GetModuleApplier().Tick(.1f);
			f.Equal("stationary ship receives zero movement bonus", speedBase, f.Owner(OwnerAttributeIds::AttackSpeed));
			f.ship->SetVelocity({ 100.f, 0.f });
			f.ship->GetMovementComponent().Tick(.1f);
			f.ship->GetModuleApplier().Tick(.1f);
			f.Remove(movement);
			f.Equal("removing Movement Rhythm while moving releases its active modifier", speedBase, f.Owner(OwnerAttributeIds::AttackSpeed));
			report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == handles, "dynamic attribute handles stay stable after ten removals");
		}
	}
	void CooldownChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& abilities = f.ship->GetAbilitySystemComponent();
		GameAbility* offense = abilities.GetAbility(sas::AbilitySlot::Ability3);
		GameAbility* defense = abilities.GetAbility(sas::AbilitySlot::Ability4);
		report.Check(offense && defense, "P3 uses actual Q/E/F/R loadout abilities");
		const auto clock = f.Install("Module.Offense.HuntClock.Uncommon", 1, 3, "Module.SideFeature.HuntClock.FifthKill");
		report.Check(offense->TryActivate(), "real Offense ability activates for Hunt Clock");
		if (offense->IsActive()) offense->Cancel(sas::AbilityEndReason::Completed);
		report.Check(defense->TryActivate(), "real Defense ability activates for Hunt Clock");
		if (defense->IsActive()) defense->Cancel(sas::AbilityEndReason::Completed);
		for (int count = 1; count <= 5; ++count)
		{
			const bool offenseMatches = ModuleTriggerEventTagsMatch({ GameplayTags::Ability::Offense }, offense->GetDefinition().abilityTags);
			const bool defenseMatches = ModuleTriggerEventTagsMatch({ GameplayTags::Ability::Offense }, defense->GetDefinition().abilityTags);
			float expectedOffense = offense->GetCooldownRemaining() * (offenseMatches ? .93f : 1.f);
			float expectedDefense = defense->GetCooldownRemaining() * (defenseMatches ? .93f : 1.f);
			if (count == 5) { if (offenseMatches && (!defenseMatches || expectedOffense >= expectedDefense)) expectedOffense *= .8f; else if (defenseMatches) expectedDefense *= .8f; }
			f.Kill();
			f.Equal("Hunt Clock reduces pending offense cooldown and fifth-kill reward", expectedOffense, offense->GetCooldownRemaining());
			f.Equal("Hunt Clock reduces pending defense cooldown and fifth-kill reward", expectedDefense, defense->GetCooldownRemaining());
		}
		f.Remove(clock);
		offense->Tick(100.f);
		defense->Tick(100.f);
		const auto lucky = f.Install("Module.Utility.LuckyCast.Uncommon");
		auto& attributes = abilities.GetAttributes();
		attributes.SetBaseValue(OwnerAttributeIds::Luck, 20.f);
		const float full = offense->GetCooldownDuration();
		report.Check(offense->TryActivate(), "Lucky Cast real ability activates at guaranteed Luck probability");
		if (offense->IsActive()) offense->Cancel(sas::AbilityEndReason::Completed);
		f.Equal("Lucky Cast halves cooldown that starts on ability end", full * .5f, offense->GetCooldownRemaining());
		offense->Tick(100.f);
		const auto zeroLuck = attributes.AddModifier({ OwnerAttributeIds::Luck, sas::AttributeModifierOperation::Override, 0.f, 100000 });
		f.Equal("zero-Luck scenario has exactly zero effective Luck", 0.f, f.Owner(OwnerAttributeIds::Luck));
		report.Check(offense->TryActivate(), "Lucky Cast real ability activates at zero Luck");
		if (offense->IsActive()) offense->Cancel(sas::AbilityEndReason::Completed);
		f.Equal("zero Luck gives no refund", full, offense->GetCooldownRemaining());
		offense->Tick(100.f);
		GameAbilityDefinition activationDefinition = offense->GetDefinition();
		activationDefinition.cooldownStartPolicy = sas::AbilityCooldownStartPolicy::OnActivation;
		report.Check(abilities.RemoveAbility(offense->GetHandle()), "P3 replaces the test ability through the real ASC");
		std::string failure;
		const auto handle = abilities.GrantAbility(activationDefinition, &failure);
		report.Check(handle.IsValid(), "ASC grants OnActivation cooldown definition");
		offense = abilities.GetAbility(handle);
		attributes.RemoveModifier(zeroLuck);
		report.Check(offense->TryActivate(), "Lucky Cast activates OnActivation cooldown ability");
		f.Equal("Lucky Cast halves cooldown already started on activation", full * .5f, offense->GetCooldownRemaining());
		if (offense->IsActive()) offense->Cancel(sas::AbilityEndReason::Completed);
		f.Remove(lucky);
		offense->Tick(100.f);
		report.Check(offense->TryActivate(), "ability still activates after Lucky Cast removal");
		f.Equal("removing Lucky Cast leaves no future refund", full, offense->GetCooldownRemaining());
	}
}

int main(int argc, char** argv)
{
	if (argc != 2 && argc != 3) return 2;
	const bool killsOnly = argc == 3 && std::string{ argv[2] } == "kills";
	const bool wardOnly = argc == 3 && std::string{ argv[2] } == "killward";
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "P3 content registration failed" };
		std::string title = "Uncommon P3 E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		if (wardOnly) KillWardChecks(report, application);
		else
		{
			if (!killsOnly) { ContentChecks(report); HealthChecks(report, application); }
			KillChecks(report, application);
			if (!killsOnly) { KillWardChecks(report, application); StateChecks(report, application); CooldownChecks(report, application); }
		}
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "UncommonModuleP3E2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
