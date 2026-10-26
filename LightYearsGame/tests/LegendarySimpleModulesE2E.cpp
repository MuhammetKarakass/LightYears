#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "gameplay/module/rules/legendary/LegendaryBulwarkPulseRules.h"
#include "gameplay/module/rules/legendary/LegendaryUnderdogCoreRules.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
	using namespace uncommon_p6_e2e;

	constexpr const char* UnderdogId = "Module.Utility.UnderdogCore.Legendary";
	constexpr const char* BulwarkId = "Module.Defense.BulwarkPulse.Legendary";
	constexpr const char* ShieldId = "Ability.Defense.Shield.Basic";
	constexpr const char* RocketId = "Ability.Offense.Rocket.Basic";

	ModuleRulesContext MakeRules(const ShipSchematic& schematic)
	{
		return ModuleRulesContext{ schematic,
			[](const sas::ContentId& id) { return content::ModuleContentCatalog::FindById(id); },
			content::ModuleContentCatalog::GetZoneTable(),
			[](const sas::ContentId& id) { return content::ModuleSideFeatureCatalog::FindById(id); } };
	}

	ModulePlacement AddPlacement(List<ModulePlacement>& placements, const char* id, int level = 1)
	{
		const auto* module = content::ModuleContentCatalog::FindById(id);
		if (!module) throw std::runtime_error{ std::string{ "missing module " } + id };
		ModulePlacement placement;
		placement.instanceId = static_cast<std::uint32_t>(placements.size() + 1);
		placement.moduleId = module->moduleId;
		placement.rarity = module->rarity;
		placement.level = level;
		placements.push_back(placement);
		return placement;
	}

	void CheckWeights(Report& report, const ResolvedModuleLegendaryUnderdogCoreRule& rule, const char* label)
	{
		report.Check(rule.offenseWeight == 14, std::string{ label } + " Offense W_f", rule.offenseWeight);
		report.Check(rule.defenseWeight == 6, std::string{ label } + " Defense W_f", rule.defenseWeight);
		report.Check(rule.primaryWeight == 2, std::string{ label } + " Primary W_f", rule.primaryWeight);
		report.Check(rule.movementWeight == 4, std::string{ label } + " Movement W_f", rule.movementWeight);
		report.Check(rule.utilityWeight == 4, std::string{ label } + " Utility W_f", rule.utilityWeight);
	}

	void CheckResolvedValues(Report& report, const ResolvedModuleLegendaryUnderdogCoreRule& rule, float effectiveLevel, const char* label)
	{
		const float n = effectiveLevel - 1.f;
		report.CheckValue(std::string{ label } + " Offense AP/EP per weight", 1.f + .125f * n, rule.offensePowerPerWeight, label);
		report.CheckValue(std::string{ label } + " Defense MaxHealth per weight", 4.f + .5f * n, rule.defenseHealthPerWeight, label);
		report.CheckValue(std::string{ label } + " Primary attack speed per weight", .375f + .05f * n, rule.primaryAttackSpeedPercentPerWeight, label);
		report.CheckValue(std::string{ label } + " Movement rating per weight", .05f + .0075f * n, rule.movementRatingPerWeight, label);
		report.CheckValue(std::string{ label } + " Utility haste per weight", .25f + .025f * n, rule.utilityHastePercentPerWeight, label);
	}

	void ResolverChecks(Report& report)
	{
		const auto makePlacements = []()
		{
			List<ModulePlacement> placements;
			AddPlacement(placements, UnderdogId, 1);
			AddPlacement(placements, "Module.Offense.AttackMatrix.Basic");
			AddPlacement(placements, "Module.Offense.AssaultCore.Basic");
			AddPlacement(placements, "Module.Offense.EmberAccumulation.Rare");
			AddPlacement(placements, "Module.Offense.PrimedArsenal.Legendary");
			AddPlacement(placements, "Module.Defense.VitalAlloy.Epic");
			AddPlacement(placements, "Module.Defense.LootShield.Uncommon");
			AddPlacement(placements, "Module.Primary.TurretMount.Epic");
			AddPlacement(placements, "Module.Movement.HuntClock.Uncommon");
			AddPlacement(placements, "Module.Utility.TripleCore.Rare");
			AddPlacement(placements, "Module.Utility.TwinRack.Legendary");
			AddPlacement(placements, "Module.Movement.Afterburner.Standard");
			AddPlacement(placements, "Module.Utility.AbilitySlot1.Standard");
			AddPlacement(placements, "Module.AbilityModule.UltimateProtocol");
			return placements;
		};

		{
			ShipSchematic schematic; schematic.width = 16; schematic.height = 16; schematic.cellZones.assign(256, GameplayTag{});
			ModuleRulesContext rules = MakeRules(schematic);
			List<ModulePlacement> placements = makePlacements();
			const auto effects = ModuleResolver::Resolve(rules, placements);
			report.Check(effects.legendaryRules.underdogCores.size() == 1, "one Underdog rule resolves");
			const auto& rule = effects.legendaryRules.underdogCores.front();
			CheckWeights(report, rule, "L1");
			CheckResolvedValues(report, rule, 1.f, "L1");
		}

		{
			ShipSchematic schematic; schematic.width = 16; schematic.height = 16; schematic.cellZones.assign(256, GameplayTags::Module::Zone::Hull);
			ModuleRulesContext rules = MakeRules(schematic);
			List<ModulePlacement> placements = makePlacements();
			placements.front().level = 3;
			const auto effects = ModuleResolver::Resolve(rules, placements);
			report.Check(effects.legendaryRules.underdogCores.size() == 1, "one Underdog rule resolves at L5");
			const auto& rule = effects.legendaryRules.underdogCores.front();
			CheckWeights(report, rule, "L5");
			CheckResolvedValues(report, rule, 5.f, "L5");
		}
	}

	void ClearDefaults(Fixture& f)
	{
		const auto defaults = f.loadout.GetPlacements();
		for (const auto& placement : defaults) f.report.Check(f.loadout.Remove(placement.instanceId), "clear a default module");
		f.report.Check(f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "apply the cleared loadout");
	}

	std::uint32_t InstallAtZone(Fixture& f, const char* id, int level, int zoneBonus)
	{
		const auto* module = f.rules.lookup(id);
		f.report.Check(module != nullptr, std::string{ "shipped module " } + id);
		const auto instance = f.loadout.AddToInventory(f.rules, *module, 0, module->rarity, level);
		f.report.Check(instance != 0, std::string{ "inventory accepts " } + id);
		ModulePlacement candidate = *f.loadout.FindInventoryItem(instance);
		for (int rotation = 0; rotation < 4; ++rotation)
		{
			for (int y = 0; y < f.rules.schematic.height; ++y)
			{
				for (int x = 0; x < f.rules.schematic.width; ++x)
				{
					candidate.origin = { x, y };
					candidate.rotationQuarterTurns = rotation;
					if (ModulePlacementRules::CanPlace(f.rules, f.loadout, candidate) != ModulePlacementFailure::None) continue;
					if (ModuleResolver::GetZoneLevelBonus(f.rules, *module, candidate) != zoneBonus) continue;
					f.report.Check(f.loadout.Install(f.rules, instance, candidate.origin, rotation) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), std::string{ "install " } + id);
					return instance;
				}
			}
		}
		throw std::runtime_error{ std::string{ "no zone placement for " } + id };
	}

	GameAbility& Grant(Fixture& f, const char* id, sas::AbilitySlot slot)
	{
		auto& asc = f.ship->GetAbilitySystemComponent();
		if (const auto* previous = asc.GetAbility(slot)) f.report.Check(asc.RemoveAbility(previous->GetHandle()), "replace the selected ability slot");
		if (const auto* existing = asc.GetAbilityById(id)) f.report.Check(asc.RemoveAbility(existing->GetHandle()), "remove a duplicate ability id");
		auto definition = *content::AbilityContentCatalog::FindById(id);
		definition.slot = slot;
		f.report.Check(asc.GrantAbility(definition).IsValid(), std::string{ "grant " } + id);
		return *asc.GetAbility(slot);
	}

	void Activate(Fixture& f, GameAbility& ability)
	{
		if (ability.IsActive()) ability.Cancel(sas::AbilityEndReason::Completed);
		ability.Tick(1000.f);
		f.report.Check(ability.TryActivate(), std::string{ "activate " } + ability.GetDefinition().abilityId);
	}

	void SetMaxHealth(Fixture& f, float maxHealth)
	{
		auto& attributes = f.ship->GetAbilitySystemComponent().GetAttributes();
		attributes.SetBaseValue(OwnerAttributeIds::MaxHealth, maxHealth);
		f.ship->RefreshMovementAttributesFromRuntime();
		f.ship->GetHealthComponent().Regenerate(maxHealth);
		f.report.CheckValue("MaxHealth fixture value", maxHealth, f.ship->GetHealthComponent().GetMaxHealth(), "Bulwark MaxHealth control");
	}

	void InstallUnderdogSupport(Fixture& f)
	{
		f.Install("Module.Offense.AttackMatrix.Basic");
		f.Install("Module.Offense.AssaultCore.Basic");
		f.Install("Module.Defense.ContinuousFieldGenerator.Rare");
		f.Install("Module.Primary.TurretMount.Epic");
		f.Install("Module.Movement.HuntClock.Uncommon");
		f.Install("Module.Utility.TripleCore.Rare");
		f.Install("Module.Movement.Afterburner.Standard");
		f.Install("Module.Utility.AbilitySlot1.Standard");
	}

	void UnderdogRuntimeChecks(Report& report, Application& application)
	{
		const struct Case { int level; int zoneBonus; float effective; } cases[] = { { 1, 0, 1.f }, { 3, 2, 5.f } };
		for (const Case& item : cases)
		{
			Fixture f{ report, application };
			ClearDefaults(f);
			const auto underdog = InstallAtZone(f, UnderdogId, item.level, item.zoneBonus);
			InstallUnderdogSupport(f);
			const ModulePlacement placement = *f.loadout.FindPlacement(underdog);
			f.report.Check(f.loadout.Uninstall(underdog) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "remove Underdog for a baseline");
			auto& attributes = f.ship->GetAbilitySystemComponent().GetAttributes();
			attributes.SetBaseValue(OwnerAttributeIds::AttackPower, 100.f);
			attributes.SetBaseValue(OwnerAttributeIds::EnergyPower, 50.f);
			attributes.SetBaseValue(OwnerAttributeIds::MaxHealth, 1000.f);
			f.ship->RefreshMovementAttributesFromRuntime();
			const float attack = f.Owner(OwnerAttributeIds::AttackPower);
			const float energy = f.Owner(OwnerAttributeIds::EnergyPower);
			const float health = f.Owner(OwnerAttributeIds::MaxHealth);
			const float horizontal = f.Owner(OwnerAttributeIds::MoveSpeedHorizontal);
			const float vertical = f.Owner(OwnerAttributeIds::MoveSpeedVertical);
			auto& asc = f.ship->GetAbilitySystemComponent();
			const sas::GameplayAttribute fireRateAttribute{ CommonAttributeIds::FireRate, 1.f, 1.f };
			const float fireRate = asc.ApplyPrimaryWeaponModifiers(fireRateAttribute);
			GameAbility& utility = Grant(f, "Ability.Utility.CryoBola.Basic", sas::AbilitySlot::Ability1);
			const float cooldown = asc.GetScopedAbilityCooldownMultiplier(utility.GetDefinition());
			f.report.Check(f.loadout.Install(f.rules, underdog, placement.origin, placement.rotationQuarterTurns) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "reinstall Underdog");
			const auto& resolved = f.ship->GetModuleApplier().GetResolvedEffects().legendaryRules.underdogCores;
			f.report.Check(resolved.size() == 1, "Underdog reaches its runtime");
			const auto& rule = resolved.front();
			f.report.Check(rule.instanceId == underdog, "Underdog rule belongs to the installed instance");
			f.report.CheckValue("Underdog effective level", item.effective, static_cast<float>(item.level + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), underdog)), "zone placement");
			f.report.Check(rule.offenseWeight == 10 && rule.defenseWeight == 3 && rule.primaryWeight == 2 && rule.movementWeight == 4 && rule.utilityWeight == 3, "Underdog weights exclude system and ability-slot shells", { { "offense", rule.offenseWeight }, { "defense", rule.defenseWeight }, { "primary", rule.primaryWeight }, { "movement", rule.movementWeight }, { "utility", rule.utilityWeight } });
			const float n = item.effective - 1.f;
			f.Equal("Underdog AP uses W_offense", attack + (1.f + .125f * n) * static_cast<float>(rule.offenseWeight), f.Owner(OwnerAttributeIds::AttackPower));
			f.Equal("Underdog EP uses W_offense", energy + (1.f + .125f * n) * static_cast<float>(rule.offenseWeight), f.Owner(OwnerAttributeIds::EnergyPower));
			f.Equal("Underdog MaxHealth uses W_defense", health + (4.f + .5f * n) * static_cast<float>(rule.defenseWeight), f.Owner(OwnerAttributeIds::MaxHealth));
			f.Equal("Underdog horizontal rating uses W_movement", horizontal + (.05f + .0075f * n) * static_cast<float>(rule.movementWeight), f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
			f.Equal("Underdog vertical rating uses W_movement", vertical + (.05f + .0075f * n) * static_cast<float>(rule.movementWeight), f.Owner(OwnerAttributeIds::MoveSpeedVertical));
			f.Equal("Underdog Primary fire rate uses W_primary", fireRate * (1.f + (.375f + .05f * n) * static_cast<float>(rule.primaryWeight) * .01f), asc.ApplyPrimaryWeaponModifiers(fireRateAttribute));
			f.Equal("Underdog Utility haste uses W_utility", cooldown / (1.f + (.25f + .025f * n) * static_cast<float>(rule.utilityWeight) * .01f), asc.GetScopedAbilityCooldownMultiplier(utility.GetDefinition()));
		}
	}

	void CoexistenceChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		ClearDefaults(f);
		auto& attributes = f.ship->GetAbilitySystemComponent().GetAttributes();
		attributes.SetBaseValue(OwnerAttributeIds::AttackPower, 100.f);
		attributes.SetBaseValue(OwnerAttributeIds::EnergyPower, 50.f);
		f.Install("Module.Offense.AssaultCore.Basic");
		f.Install("Module.Offense.AttackMatrix.Basic");
		const float attack = f.Owner(OwnerAttributeIds::AttackPower);
		const float energy = f.Owner(OwnerAttributeIds::EnergyPower);
		const auto underdog = InstallAtZone(f, UnderdogId, 1, 0);
		const auto underdogRule = f.ship->GetModuleApplier().GetResolvedEffects().legendaryRules.underdogCores.front();
		f.report.Check(underdogRule.offenseWeight == 10, "Underdog counts both Common Offense modules", underdogRule.offenseWeight);
		f.Equal("Underdog alone adds AP for two Common Offense modules", attack + 10.f, f.Owner(OwnerAttributeIds::AttackPower));
		f.Equal("Underdog alone adds EP for two Common Offense modules", energy + 10.f, f.Owner(OwnerAttributeIds::EnergyPower));
		const auto confluence = f.Install("Module.Utility.ConfluenceCore.Epic", 0, 1);
		const float effective = static_cast<float>(1 + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), confluence));
		const float perModule = 2.f + (effective - 1.f);
		f.Equal("Confluence keeps its own AP contribution", attack + 10.f + 2.f * perModule, f.Owner(OwnerAttributeIds::AttackPower));
		f.Equal("Confluence keeps its own EP contribution", energy + 10.f + 2.f * perModule, f.Owner(OwnerAttributeIds::EnergyPower));
		f.report.Check(f.ship->GetModuleApplier().GetResolvedEffects().legendaryRules.underdogCores.front().offenseWeight == 10, "Confluence stat does not change Underdog W_f", f.ship->GetModuleApplier().GetResolvedEffects().legendaryRules.underdogCores.front().offenseWeight);
		f.Remove(confluence);
		f.Equal("removing Confluence leaves Underdog AP intact", attack + 10.f, f.Owner(OwnerAttributeIds::AttackPower));
		f.Remove(underdog);
		f.Equal("removing Underdog restores the baseline AP", attack, f.Owner(OwnerAttributeIds::AttackPower));
	}

	std::uint32_t PrepareBulwark(Fixture& f, int level)
	{
		ClearDefaults(f);
		f.Install("Module.Utility.AbilitySlot1.Standard");
		f.Install("Module.Utility.AbilitySlot2.Standard");
		return InstallAtZone(f, BulwarkId, level, 0);
	}

	float Health(const shared_ptr<SpaceShip>& ship) { return ship->GetHealthComponent().GetHealth(); }

	void BulwarkChecks(Report& report, Application& application)
	{
		for (const int level : { 1, 3 })
		{
			Fixture f{ report, application };
			const auto bulwark = PrepareBulwark(f, level);
			f.report.CheckValue("Bulwark effective level", static_cast<float>(level), static_cast<float>(level + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), bulwark)), "Bulwark zone placement");
			const auto& rules = f.ship->GetModuleApplier().GetLegendaryBulwarkPulseRuntime().GetRules();
			report.Check(rules.size() == 1, "Bulwark reaches its runtime");
			const auto rule = rules.front();
			GameAbility& defense = Grant(f, ShieldId, sas::AbilitySlot::Ability1);
			Grant(f, RocketId, sas::AbilitySlot::Ability2);
			for (const float maxHealth : { 200.f, 300.f })
			{
				SetMaxHealth(f, maxHealth);
				const auto inside = Enemy(f, 400.f);
				const auto outside = Enemy(f, 700.f);
				const float expected = rule.baseDamage + rule.healthCoefficient * std::max(0.f, maxHealth - rule.healthThreshold);
				const float outsideHealth = Health(outside);
				const sf::Vector2f insideLocation = inside->GetActorLocation();
				Activate(f, defense);
				report.CheckValue("Bulwark damage uses snapshotted MaxHealth and resolved level", expected, 1000.f - Health(inside), "Bulwark wave");
				report.CheckValue("Bulwark leaves targets outside 500 untouched", outsideHealth, Health(outside), "Bulwark radius");
				report.Check(inside->GetActorLocation() == insideLocation, "Bulwark does not push targets");
				inside->GetHealthComponent().Regenerate(1000.f);
				if (maxHealth < 300.f)
				{
					f.ship->GetModuleApplier().Tick(.1f);
					Activate(f, defense);
					report.CheckValue("Bulwark internal cooldown blocks a second wave", 1000.f, Health(inside), "Bulwark cooldown");
					f.ship->GetModuleApplier().Tick(5.9f);
				}
				inside->Destroy();
				outside->Destroy();
			}
		}

		{
			Fixture f{ report, application };
			PrepareBulwark(f, 1);
			GameAbility& rocket = Grant(f, RocketId, sas::AbilitySlot::Ability2);
			const auto enemy = Enemy(f, 400.f);
			SetMaxHealth(f, 200.f);
			const float before = Health(enemy);
			Activate(f, rocket);
			report.CheckValue("non-Defense activation produces no Bulwark wave", before, Health(enemy), "Rocket activation");
		}
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext context; vega::EngineContext::Scope scope{ context }; vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		report.Check(GameContentBootstrap::Register(), "register shipped game content");
		ResolverChecks(report);
		std::string title = "Legendary simple modules E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		UnderdogRuntimeChecks(report, application);
		CoexistenceChecks(report, application);
		BulwarkChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "LightYearsLegendarySimpleModulesE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
