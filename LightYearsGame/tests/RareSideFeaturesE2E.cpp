#include "UncommonModuleP5E2E.h"

#include "gameplay/ShieldComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"
#include "gameplay/module/ModuleTriggerRuntime.h"
#include "gameplay/progression/RunGold.h"
#include "gameplay/tags/GameplayTags.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace rare_side_features_e2e
{
	using namespace ly;
	using namespace uncommon_p5_e2e;
	using Json = common_module_e2e::Json;

	namespace
	{
		constexpr const char* kRareHost = "Module.Defense.RareSideHost.Rare";
		constexpr const char* kUncommonHost = "Module.Defense.RareSideUncommonHost.Uncommon";
		constexpr const char* kFixturePool = "Module.SidePool.RareFixture";
		constexpr const char* kShieldSide = "Module.SideFeature.RareTest.ShieldRegen";

		// Test-only modules and side features parsed from fixture JSON. They never enter shipped content.
		List<ModuleDefinition> gFixtureModules;
		List<ModuleSideFeatureDefinition> gFixtureSideFeatures;

		std::string RareId(const char* name) { return std::string{ "Module.SideFeature.Shared." } + name; }

		const std::vector<std::string> kRareOffense{ RareId("O1.Rare"), RareId("O2.Rare"), RareId("O3.Rare"), RareId("O4.Rare"), RareId("O5.Rare") };
		const std::vector<std::string> kRareDefense{ RareId("D1.Rare"), RareId("D2.Rare"), RareId("D3.Rare"), RareId("D4.Rare"), RareId("D5.Rare") };
		const std::vector<std::string> kRareGeneral{ RareId("G1.Rare"), RareId("G2.Rare"), RareId("G3.Rare"), RareId("G4.Rare"), RareId("G5.Rare") };
		const std::vector<std::string> kRareSpecial{ RareId("R1"), RareId("R2"), RareId("R3"), RareId("R4"), RareId("R5"), RareId("R6") };

		const ModuleDefinition* FixtureModule(const sas::ContentId& id)
		{
			for (const ModuleDefinition& definition : gFixtureModules)
				if (definition.moduleId == id) return &definition;
			return content::ModuleContentCatalog::FindById(id);
		}

		const ModuleSideFeatureDefinition* FixtureSideFeature(const sas::ContentId& id)
		{
			for (const ModuleSideFeatureDefinition& feature : gFixtureSideFeatures)
				if (feature.sideFeatureId == id) return &feature;
			return content::ModuleSideFeatureCatalog::FindById(id);
		}

		void Wire(Fixture& f)
		{
			f.rules.lookup = FixtureModule;
			f.rules.sideFeatureLookup = FixtureSideFeature;
		}

		Json ShippedJson(const char* name)
		{
			Json document;
			std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data" / name };
			input >> document;
			return document;
		}

		void WriteJson(const std::filesystem::path& path, const Json& document)
		{
			std::filesystem::create_directories(path.parent_path());
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			if (!output) throw std::runtime_error{ "Could not write rare side-feature fixture" };
			output << document.dump(2) << '\n';
		}

		Json Permission(const char* poolId, const std::vector<std::string>& featureIds)
		{
			Json allowed = Json::array();
			for (const std::string& id : featureIds) allowed.push_back(id);
			return Json{ { "poolId", poolId }, { "allowedFeatureIds", std::move(allowed) } };
		}

		// Two size variants (1 and 2 cells) so the largest-variant gate is exercised.
		Json HostModule(const char* id, const char* displayName, const char* rarity, Json sharedPools)
		{
			return Json{
				{ "id", id }, { "displayName", displayName }, { "rarity", rarity }, { "levelCount", 3 },
				{ "family", "Ability.Defense" }, { "sizeLadder", Json::array({ 1, 2 }) },
				{ "sizeVariants", Json::array({
					Json{ { "shape", Json::array({ Json::array({ 0, 0 }) }) } },
					Json{ { "shape", Json::array({ Json::array({ 0, 0 }), Json::array({ 1, 0 }) }) } } }) },
				{ "sharedPools", std::move(sharedPools) }
			};
		}

		// Shipped modules (zone table, system zones) plus two test-only hosts: a Rare host that may equip every Rare copy,
		// and an Uncommon host that may equip only the Uncommon O1 and must reject Rare copies.
		Json FixtureModulesDocument()
		{
			Json rarePools = Json::array();
			rarePools.push_back(Permission("Module.SidePool.RareOffense", kRareOffense));
			rarePools.push_back(Permission("Module.SidePool.RareDefense", kRareDefense));
			rarePools.push_back(Permission("Module.SidePool.RareGeneral", kRareGeneral));
			rarePools.push_back(Permission("Module.SidePool.RareSpecial", kRareSpecial));
			rarePools.push_back(Permission(kFixturePool, { kShieldSide }));
			Json uncommonPools = Json::array();
			uncommonPools.push_back(Permission("Module.SidePool.Offense", { "Module.SideFeature.Shared.O1" }));
			Json document = ShippedJson("modules.json");
			document["modules"] = Json::array({
				HostModule(kRareHost, "Rare side host fixture", "Rare", std::move(rarePools)),
				HostModule(kUncommonHost, "Uncommon side host fixture", "Uncommon", std::move(uncommonPools)) });
			return document;
		}

		// Fixture side package with one shieldRules feature in its own pool. The rules argument varies for the loader cases.
		Json ShieldSideDocument(Json rules)
		{
			Json feature{ { "id", kShieldSide }, { "displayName", "Rare test shield regen" }, { "description", "Shield regeneration is continuous." },
				{ "poolId", kFixturePool }, { "levelCount", 3 }, { "rules", std::move(rules) } };
			Json pool{ { "id", kFixturePool }, { "choices", Json::array({ Json{ { "sideFeatureId", kShieldSide }, { "weight", 1.0 } } }) } };
			return Json{ { "schemaVersion", 1 }, { "sharedPools", Json::array({ std::move(pool) }) }, { "sideFeatures", Json::array({ std::move(feature) }) } };
		}

		Json ContinuousShieldRules()
		{
			return Json{ { "shieldRules", Json::array({ Json{ { "regenMode", "Continuous" }, { "continuousRateFraction", 0.5 } } }) } };
		}

		bool EndsWith(const std::string& text, const std::string& suffix)
		{
			return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
		}

		const ModuleSideFeatureDefinition* FindDefinition(const content::ModuleSideFeatureLoader::Result& loaded, const std::string& id)
		{
			for (const ModuleSideFeatureDefinition& definition : loaded.definitions)
				if (definition.sideFeatureId.ToString() == id) return &definition;
			return nullptr;
		}

		// Shipped content: the real loader and the bootstrapped catalog expose the Rare copies, R1-R6, and Uncommon O1 is unchanged.
		void ShippedChecks(Report& report)
		{
			const auto dataPath = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data";
			const auto side = content::ModuleSideFeatureLoader::LoadFromFile(dataPath / "module_side_features.json");
			report.Check(side.Succeeded(), "shipped side features load through the real loader", side.error);
			int rareCopies = 0;
			for (const ModuleSideFeatureDefinition& definition : side.definitions)
				if (EndsWith(definition.sideFeatureId.ToString(), ".Rare")) ++rareCopies;
			report.Check(rareCopies == 16, "all 15 Rare shared copies and the F1 fragmentation switch load", Json{ { "count", rareCopies } });
			for (const std::string& id : kRareSpecial)
				report.Check(FindDefinition(side, id) != nullptr, std::string{ "Rare-only feature loads: " } + id);
			for (const char* pool : { "Module.SidePool.RareOffense", "Module.SidePool.RareDefense", "Module.SidePool.RareGeneral", "Module.SidePool.RareSpecial" })
			{
				bool found = false;
				for (const ModuleSideFeaturePool& entry : side.pools) found = found || entry.poolId.ToString() == pool;
				report.Check(found, std::string{ "Rare pool loads: " } + pool);
			}
			report.Check(content::ModuleSideFeatureCatalog::FindById(sas::ContentId{ RareId("O1.Rare") }) != nullptr,
				"bootstrapped catalog exposes the Rare copies");
			const Json sides = ShippedJson("module_side_features.json");
			for (const Json& feature : sides["sideFeatures"])
			{
				if (feature["id"] != "Module.SideFeature.Shared.O1") continue;
				report.CheckValue("Uncommon O1 stays at +8 AP", 8.f, feature["rules"]["attributeModifiers"][0]["magnitude"][0].get<float>(), "shipped JSON");
			}
		}

		// Test-only side packages: shieldRules must load and not count as empty; zoneRules and no-op shieldRules must fail loudly.
		void LoaderChecks(Report& report, const std::filesystem::path& root)
		{
			const auto reject = [&](const char* name, Json rules, const char* expected)
			{
				const auto path = root / (std::string{ "side-" } + name + ".json");
				WriteJson(path, ShieldSideDocument(std::move(rules)));
				const auto loaded = content::ModuleSideFeatureLoader::LoadFromFile(path);
				report.Check(!loaded.Succeeded() && loaded.error.find(expected) != std::string::npos,
					std::string{ "side-feature loader rejects " } + name, Json{ { "error", loaded.error } });
			};
			reject("zone-rules", Json{ { "zoneRules", Json::array({ Json{ { "kind", "AmplifyZone" }, { "bonus", 0.3 } } }) } },
				"zoneRules is not supported on side features");
			reject("empty-shield-entry", Json{ { "shieldRules", Json::array({ Json{ { "regenMode", "Normal" } } }) } },
				"shieldRules entry has no effect");
			const auto path = root / "side-shield-only.json";
			WriteJson(path, ShieldSideDocument(ContinuousShieldRules()));
			const auto loaded = content::ModuleSideFeatureLoader::LoadFromFile(path);
			report.Check(loaded.Succeeded() && loaded.definitions.size() == 1 && loaded.definitions.front().rules.shieldRules.size() == 1,
				"a side package with only shieldRules is accepted, not treated as empty", Json{ { "error", loaded.error } });
		}

		float Dealt(Fixture& f, Actor& target, float damage = 10.f, const List<GameplayTag>& abilityTags = {})
		{
			const float dealt = Hit(target, f.ship.get(), damage, Plain(), {}, abilityTags);
			if (auto* health = target.FindComponent<HealthComponent>()) health->Regenerate(health->GetMaxHealth());
			return dealt;
		}

		// Damage the player ship takes from the enemy, with the shield emptied so the hit reaches health.
		float ShipDealt(Fixture& f, Actor& source, float damage = 10.f)
		{
			f.ship->GetShieldComponent().ChangeShield(-f.ship->GetShieldComponent().GetShield());
			const float dealt = Hit(*f.ship, &source, damage, Plain());
			f.ship->GetHealthComponent().Regenerate(f.ship->GetHealthComponent().GetMaxHealth());
			return dealt;
		}

		void ActivateEvent(Fixture& f, const GameplayTag& tag)
		{
			const List<GameplayTag> sourceTags{ tag };
			const List<GameplayTag> payloadTags{};
			ModuleTriggerEventContext event{ ModuleTriggerKind::AbilityActivated, 1.f, sourceTags, payloadTags };
			f.ship->GetCombatRuntime().ActivateOutgoingDamageRules(event);
		}

		void TickCombat(Fixture& f, float seconds)
		{
			f.ship->GetCombatRuntime().Tick(seconds);
		}

		float NormalRegen(Fixture& f)
		{
			return std::max(0.f, f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen)) *
				f.ship->GetShieldRegenMultiplier();
		}

		// Owner stats read back through the real ability attributes after the module is applied.
		void OwnerStatChecks(Report& report, Application& application)
		{
			const struct { const char* name; const char* feature; const sas::AttributeId* attribute; float delta; } cases[] = {
				{ "O1.Rare adds +14 Attack Power", "O1.Rare", &OwnerAttributeIds::AttackPower, 14.f },
				{ "O2.Rare adds +14 Critical chance", "O2.Rare", &OwnerAttributeIds::CriticalChance, 14.f },
				{ "O3.Rare adds +7 Attack Speed", "O3.Rare", &OwnerAttributeIds::AttackSpeed, 7.f },
				{ "D1.Rare adds +45 Max Health", "D1.Rare", &OwnerAttributeIds::MaxHealth, 45.f },
				{ "G2.Rare adds +7 Ability Haste", "G2.Rare", &OwnerAttributeIds::AbilityHaste, 7.f },
				{ "G4.Rare adds +1.4 horizontal movement", "G4.Rare", &OwnerAttributeIds::MoveSpeedHorizontal, 1.4f },
				{ "G4.Rare adds +1.4 vertical movement", "G4.Rare", &OwnerAttributeIds::MoveSpeedVertical, 1.4f },
				{ "G5.Rare adds +2 Luck", "G5.Rare", &OwnerAttributeIds::Luck, 2.f },
			};
			for (const auto& item : cases)
			{
				Fixture f{ report, application };
				Wire(f);
				const float base = f.Owner(*item.attribute);
				f.Install(kRareHost, 1, 3, RareId(item.feature).c_str());
				report.CheckValue(item.name, base + item.delta, f.Owner(*item.attribute), item.feature);
			}
		}

		// Ship stats (shield, afterburner) read back as a ratio against the same ship before the module.
		void ShipStatChecks(Report& report, Application& application)
		{
			const auto ratioCheck = [&](const char* name, const char* feature, float expected, const std::function<float(Fixture&)>& read)
			{
				Fixture f{ report, application };
				Wire(f);
				const float base = read(f);
				f.Install(kRareHost, 1, 3, RareId(feature).c_str());
				report.CheckValue(name, expected, read(f) / base, feature);
			};
			ratioCheck("D4.Rare scales shield regeneration by 1.25", "D4.Rare", 1.25f, [](Fixture& f) { return NormalRegen(f); });
			ratioCheck("D5.Rare scales Max Shield by 1.14", "D5.Rare", 1.14f, [](Fixture& f) { return f.ship->GetShieldComponent().GetMaxShield(); });
			ratioCheck("G3.Rare scales Afterburner capacity by 1.18", "G3.Rare", 1.18f, [](Fixture& f) { return f.ship->GetShipRuntime().GetAfterburnerCapacity(); });
		}

		// Placement gates: small variant keeps the feature locked, rolls outside the host's pools are rejected.
		void PlacementChecks(Report& report, Application& application)
		{
			{
				Fixture f{ report, application };
				Wire(f);
				const float base = f.Owner(OwnerAttributeIds::AttackPower);
				const auto small = f.Install(kRareHost, 0, 3, RareId("O1.Rare").c_str());
				report.CheckValue("Rare O1 stays locked on the smaller variant", base, f.Owner(OwnerAttributeIds::AttackPower), "variant 0");
				f.Remove(small);
				const auto large = f.Install(kRareHost, 1, 3, RareId("O1.Rare").c_str());
				report.CheckValue("Rare O1 applies once the largest variant is placed", base + 14.f, f.Owner(OwnerAttributeIds::AttackPower), "variant 1");
				f.Remove(large);
			}
			{
				Fixture f{ report, application };
				Wire(f);
				const auto* uncommon = FixtureModule(sas::ContentId{ kUncommonHost });
				const auto* rare = FixtureModule(sas::ContentId{ kRareHost });
				report.Check(uncommon != nullptr && rare != nullptr, "both fixture hosts resolve through the real loader");
				report.Check(f.loadout.AddToInventory(f.rules, *uncommon, 1, uncommon->rarity, 3, sas::ContentId{ "Module.SideFeature.Shared.O1" }) != 0,
					"Uncommon host accepts its own pool's Uncommon O1");
				report.Check(f.loadout.AddToInventory(f.rules, *uncommon, 1, uncommon->rarity, 3, sas::ContentId{ RareId("O1.Rare").c_str() }) == 0,
					"Uncommon host rejects a Rare copy outside its permissions");
				report.Check(f.loadout.AddToInventory(f.rules, *rare, 1, rare->rarity, 3, sas::ContentId{ "Module.SideFeature.Shared.O1" }) == 0,
					"Rare host rejects the Uncommon O1 (pool not permitted)");
			}
		}

		// Uncommon O1 (+8) and Rare O1 (+14) on the same ship, same host shape.
		void UncommonRareComparison(Report& report, Application& application)
		{
			Fixture f{ report, application };
			Wire(f);
			const float base = f.Owner(OwnerAttributeIds::AttackPower);
			const auto uncommon = f.Install(kUncommonHost, 1, 3, "Module.SideFeature.Shared.O1");
			report.CheckValue("Uncommon O1 adds +8 Attack Power on the same ship", base + 8.f, f.Owner(OwnerAttributeIds::AttackPower), "Uncommon");
			f.Remove(uncommon);
			const auto rare = f.Install(kRareHost, 1, 3, RareId("O1.Rare").c_str());
			report.CheckValue("Rare O1 adds +14 Attack Power on the same ship", base + 14.f, f.Owner(OwnerAttributeIds::AttackPower), "Rare");
			f.Remove(rare);
		}

		// Crit, speed and status copies that need a damage or status read through the real damage path.
		void DamagePathChecks(Report& report, Application& application)
		{
			{
				Fixture f{ report, application };
				Wire(f);
				auto baseline = Enemy(f);
				const float base = Dealt(f, *baseline);
				f.Install(kRareHost, 1, 3, RareId("O5.Rare").c_str());
				auto fresh = Enemy(f);
				const float first = Dealt(f, *fresh);
				const float second = Dealt(f, *fresh);
				report.CheckValue("O5.Rare multiplies the first hit by 1.25", 1.25f, first / second, "first vs repeat hit");
				report.CheckValue("O5.Rare leaves later hits at the base damage", base, second, "repeat hit");
			}
			{
				Fixture f{ report, application };
				Wire(f);
				auto enemy = Enemy(f);
				StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Electric);
				const float before = Effect(*enemy, DamageStatusEffectIds::ElectricEffectId)->spec.duration;
				f.Install(kRareHost, 1, 3, RareId("O4.Rare").c_str());
				auto target = Enemy(f);
				StatusHit(*target, f.ship.get(), DamageTypeSchema::Electric);
				const float after = Effect(*target, DamageStatusEffectIds::ElectricEffectId)->spec.duration;
				report.CheckValue("O4.Rare adds +1.5 seconds to applied status durations", 1.5f, after - before, "electric status");
			}
			{
				Fixture f{ report, application };
				Wire(f);
				const auto wallet = std::make_shared<RunGold>();
				f.ship->GetModuleApplier().SetGoldWallet(wallet);
				f.Install(kRareHost, 1, 3, RareId("G1.Rare").c_str());
				f.Kill();
				report.CheckValue("G1.Rare awards 2 Gold per kill", 2.f, static_cast<float>(wallet->GetAmount()), "kill");
			}
			{
				Fixture f{ report, application };
				Wire(f);
				f.Install(kRareHost, 1, 3, RareId("D3.Rare").c_str());
				auto& shield = f.ship->GetShieldComponent();
				shield.ChangeShield(-shield.GetShield());
				auto enemy = Enemy(f);
				ApplyCombatDamage(*f.ship, 50.f, enemy.get(), {}, Plain());
				auto& health = f.ship->GetHealthComponent();
				const float before = health.GetHealth();
				f.Kill();
				report.CheckValue("D3.Rare restores 1.75% of Max Health on kill", 0.0175f * health.GetMaxHealth(), health.GetHealth() - before, "kill restore");
			}
			{
				Fixture f{ report, application };
				Wire(f);
				const float armor = f.Owner(OwnerAttributeIds::Armor);
				f.Install(kRareHost, 1, 3, RareId("D2.Rare").c_str());
				auto enemy = Enemy(f);
				ShipDealt(f, *enemy);
				report.CheckValue("D2.Rare adds +20 Armor after damage", armor + 20.f, f.Owner(OwnerAttributeIds::Armor), "DamageTaken");
				f.ship->GetModuleTriggerRuntime().Tick(2.01f);
				report.CheckValue("D2.Rare Armor ends after 2 seconds", armor, f.Owner(OwnerAttributeIds::Armor), "t=2.01");
			}
		}

		// R1-R6 conditions and windows, each read through the real module application and damage path.
		void SpecialChecks(Report& report, Application& application)
		{
			const List<GameplayTag> primary{ GameplayTags::Ability::Primary };
			{
				Fixture f{ report, application };
				Wire(f);
				auto enemy = Enemy(f);
				const float base = Dealt(f, *enemy, 10.f, primary);
				f.Install(kRareHost, 1, 3, RareId("R1").c_str());
				report.CheckValue("R1 Guarded Strike is closed before a Defense ability", base, Dealt(f, *enemy, 10.f, primary), "no event");
				ActivateEvent(f, GameplayTags::Ability::Defense);
				report.CheckValue("R1 Guarded Strike raises the Primary shot by 1.15", 1.15f * base, Dealt(f, *enemy, 10.f, primary), "Defense window");
				report.CheckValue("R1 Guarded Strike ignores non-primary damage", base * 1.f, Dealt(f, *enemy), "non-primary");
				TickCombat(f, 4.01f);
				report.CheckValue("R1 Guarded Strike closes after 4 seconds", base, Dealt(f, *enemy, 10.f, primary), "t=4.01");
			}
			{
				Fixture f{ report, application };
				Wire(f);
				auto enemy = Enemy(f);
				const float base = Dealt(f, *enemy);
				f.Install(kRareHost, 1, 3, RareId("R3").c_str());
				report.CheckValue("R3 Boss Hunter ignores a normal target", base, Dealt(f, *enemy), "Normal");
				enemy->SetControlTargetClass(ControlTargetClass::Boss);
				report.CheckValue("R3 Boss Hunter multiplies boss-class damage by 1.10", 1.10f * base, Dealt(f, *enemy), "Boss");
			}
			{
				Fixture f{ report, application };
				Wire(f);
				auto enemy = Enemy(f);
				auto& shield = f.ship->GetShieldComponent();
				shield.ChangeShield(shield.GetMaxShield());
				const float base = Dealt(f, *enemy);
				f.Install(kRareHost, 1, 3, RareId("R4").c_str());
				report.CheckValue("R4 Full Charge multiplies damage by 1.08 while the shield is full", 1.08f * base, Dealt(f, *enemy), "full shield");
				shield.ChangeShield(-shield.GetShield());
				report.CheckValue("R4 Full Charge stops once the shield is empty", base, Dealt(f, *enemy), "empty shield");
			}
			{
				Fixture f{ report, application };
				Wire(f);
				auto enemy = Enemy(f);
				// Electric stacks add their own incoming bonus to the target, so every ratio compares the same
				// target state with the module installed and removed.
				StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Thermal);
				const auto oneKind = f.Install(kRareHost, 1, 3, RareId("R5").c_str());
				const float oneWith = Dealt(f, *enemy);
				f.Remove(oneKind);
				const float oneWithout = Dealt(f, *enemy);
				report.CheckValue("R5 Status Weaver ignores one status kind", 1.f, oneWith / oneWithout, "one kind");
				StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Electric);
				const auto twoKinds = f.Install(kRareHost, 1, 3, RareId("R5").c_str());
				const float twoWith = Dealt(f, *enemy);
				f.Remove(twoKinds);
				const float twoWithout = Dealt(f, *enemy);
				report.CheckValue("R5 Status Weaver multiplies damage by 1.20 with two status kinds", 1.20f, twoWith / twoWithout, "two kinds");
			}
			{
				Fixture f{ report, application };
				Wire(f);
				auto enemy = Enemy(f);
				const float base = ShipDealt(f, *enemy);
				f.Install(kRareHost, 1, 3, RareId("R6").c_str());
				report.CheckValue("R6 Fleet Guard is closed before a Movement ability", base, ShipDealt(f, *enemy), "no event");
				ActivateEvent(f, GameplayTags::Ability::Movement);
				report.CheckValue("R6 Fleet Guard reduces incoming damage to 0.85", 0.85f * base, ShipDealt(f, *enemy), "Movement window");
				TickCombat(f, 3.01f);
				report.CheckValue("R6 Fleet Guard closes after 3 seconds", base, ShipDealt(f, *enemy), "t=3.01");
			}
			{
				Fixture f{ report, application };
				Wire(f);
				f.Install(kRareHost, 1, 3, RareId("R2").c_str());
				auto enemy = Enemy(f);
				const float armor = f.Owner(OwnerAttributeIds::Armor);
				auto& shield = f.ship->GetShieldComponent();
				shield.ChangeShield(-shield.GetShield());
				shield.ChangeShield(0.5f * shield.GetMaxShield());
				Hit(*f.ship, enemy.get(), shield.GetShield(), Plain());
				report.CheckValue("R2 Reactive Plating adds +30 Armor when the shield breaks", armor + 30.f, f.Owner(OwnerAttributeIds::Armor), "ShieldBroken");
				f.ship->GetModuleTriggerRuntime().Tick(4.01f);
				report.CheckValue("R2 Reactive Plating ends after 4 seconds", armor, f.Owner(OwnerAttributeIds::Armor), "t=4.01");
			}
		}

		// shieldRules on a side feature reach the live shield through the real applier; the locked variant does not.
		void ShieldRuleChecks(Report& report, Application& application)
		{
			{
				Fixture f{ report, application };
				Wire(f);
				const auto locked = f.Install(kRareHost, 0, 3, kShieldSide);
				report.Check(f.ship->GetShieldComponent().GetModuleBehavior().regenMode == ShieldModuleBehavior::RegenMode::Normal,
					"shield side feature on the smaller variant does not change shield regen");
				f.Remove(locked);
			}
			{
				Fixture f{ report, application };
				Wire(f);
				f.Install(kRareHost, 1, 3, kShieldSide);
				const auto& behavior = f.ship->GetShieldComponent().GetModuleBehavior();
				report.Check(behavior.regenMode == ShieldModuleBehavior::RegenMode::Continuous && behavior.continuousRateFraction == 0.5f,
					"side-feature shieldRules reach the live shield through the real applier",
					Json{ { "continuousRateFraction", behavior.continuousRateFraction } });
			}
		}
	}
}

int main(int argc, char** argv)
{
	using namespace rare_side_features_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare side-feature content registration failed" };
		std::string title = "Rare Side Features E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);

		const std::filesystem::path root = std::filesystem::path{ argv[1] }.parent_path() / "rare-side-features-fixtures";
		ShippedChecks(report);
		LoaderChecks(report, root);

		const auto modulesPath = root / "rare-side-features-modules.json";
		WriteJson(modulesPath, FixtureModulesDocument());
		const auto modulesLoaded = content::ModuleLoader::LoadFromFile(modulesPath);
		report.Check(modulesLoaded.Succeeded(), "rare side-feature fixture modules load through the real ModuleLoader", modulesLoaded.error);
		gFixtureModules = modulesLoaded.definitions;

		const auto sidePath = root / "rare-side-features-shield.json";
		WriteJson(sidePath, ShieldSideDocument(ContinuousShieldRules()));
		const auto sideLoaded = content::ModuleSideFeatureLoader::LoadFromFile(sidePath);
		report.Check(sideLoaded.Succeeded(), "rare side-feature fixture shield package loads through the real loader", sideLoaded.error);
		gFixtureSideFeatures = sideLoaded.definitions;

		OwnerStatChecks(report, application);
		ShipStatChecks(report, application);
		PlacementChecks(report, application);
		UncommonRareComparison(report, application);
		DamagePathChecks(report, application);
		SpecialChecks(report, application);
		ShieldRuleChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareSideFeaturesE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
