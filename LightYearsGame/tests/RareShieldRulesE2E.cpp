#include "UncommonModuleP4E2E.h"

#include "attributes/AttributeMath.h"
#include "gameplay/ShieldComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/module/ModuleTriggerRuntime.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>

namespace rare_shield_e2e
{
	using namespace uncommon_p4_e2e;
	using Json = common_module_e2e::Json;
	using ShieldMode = ShieldModuleBehavior::RegenMode;

	namespace
	{
		constexpr const char* kSpeedId = "Module.Defense.RareShieldSpeedFixture.Rare";
		constexpr const char* kContinuousId = "Module.Defense.RareShieldContinuousFixture.Rare";
		constexpr const char* kRearmId = "Module.Defense.RareShieldRearmFixture.Rare";
		constexpr const char* kBrokenId = "Module.Defense.RareShieldBrokenFixture.Rare";
		constexpr const char* kArmorId = "Module.Defense.RareShieldArmorFixture.Rare";

		// Test-only definitions parsed from fixture JSON. They never enter shipped content.
		ly::List<ly::ModuleDefinition> gFixtureDefinitions;

		const ModuleDefinition* FixtureLookup(const sas::ContentId& id)
		{
			for (const ModuleDefinition& definition : gFixtureDefinitions)
				if (definition.moduleId == id) return &definition;
			return content::ModuleContentCatalog::FindById(id);
		}

		std::filesystem::path WriteJson(const std::filesystem::path& root, const std::string& name, const Json& value)
		{
			std::filesystem::create_directories(root);
			const std::filesystem::path path = root / name;
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			output << value.dump(2) << '\n';
			return path;
		}

		Json RareModule(const char* id, const char* displayName, Json shieldRules)
		{
			return Json{
				{ "id", id }, { "displayName", displayName }, { "rarity", "Rare" }, { "levelCount", 3 },
				{ "family", "Ability.Defense" }, { "sizeLadder", Json::array({ 1 }) },
				{ "sizeVariants", Json::array({ Json{
					{ "shape", Json::array({ Json::array({ 0, 0 }) }) },
					{ "shieldRules", std::move(shieldRules) }
				} }) }
			};
		}

		// A probe that only reports ShieldBroken events: its buff is active exactly while an event was raised.
		Json ShieldBrokenBuff()
		{
			return Json::array({ Json{ { "trigger", "ShieldBroken" }, { "durationSeconds", Json::array({ 0.5, 0.5, 0.5 }) },
				{ "movementSpeedMultiplier", 1.1 }, { "modifiers", Json::array() } } });
		}

		// Shipped modules plus test-only Defense modules. speedRules replaces the SpeedScaled probe's shieldRules
		// so the loader rejection cases can reuse this exact package.
		Json FixtureSet(const Json& shipped, const Json& speedRules)
		{
			Json modules = shipped;
			modules["modules"].push_back(RareModule(kSpeedId, "Rare shield speed fixture", speedRules));
			modules["modules"].push_back(RareModule(kContinuousId, "Rare shield continuous fixture",
				Json::array({ Json{ { "regenMode", "Continuous" }, { "continuousRateFraction", 0.5 } } })));
			Json rearm = RareModule(kRearmId, "Rare shield rearm fixture", Json::array({ Json{ { "breakRearmFraction", 0.25 } } }));
			rearm["sizeVariants"][0]["triggerBuffs"] = ShieldBrokenBuff();
			modules["modules"].push_back(std::move(rearm));
			Json broken = RareModule(kBrokenId, "Rare shield broken fixture", Json::array());
			broken["sizeVariants"][0]["triggerBuffs"] = ShieldBrokenBuff();
			modules["modules"].push_back(std::move(broken));
			modules["modules"].push_back(RareModule(kArmorId, "Rare shield armor fixture", Json::array({ Json{ { "armorShare", 0.5 } } })));
			return modules;
		}

		void SetShield(SpaceShip& ship, float value)
		{
			ShieldComponent& shield = ship.GetShieldComponent();
			shield.ChangeShield(-shield.GetShield());
			if (value > 0.f) shield.ChangeShield(value);
		}

		// Normal passive regen rate as UpdateRegeneration computes it before any module shaping.
		float NormalRegen(SpaceShip& ship)
		{
			return std::max(0.f, ship.GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen)) *
				ship.GetShieldRegenMultiplier();
		}

		float Speed(const SpaceShip& ship)
		{
			const sf::Vector2f velocity = ship.GetVelocity();
			return std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
		}

		void Advance(Fixture& f, float seconds)
		{
			f.level->TickInternal(seconds);
		}

		void SpeedChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			ShieldComponent& shield = f.ship->GetShieldComponent();
			const float maxShield = shield.GetMaxShield();
			const float normal = NormalRegen(*f.ship);
			report.Check(maxShield > 20.f && normal > 0.f, "precondition: player shield has capacity and passive regen",
				Json{ { "maxShield", maxShield }, { "normalRegen", normal } });
			const auto speedInstance = f.Install(kSpeedId, 0, 2);
			report.Check(shield.GetModuleBehavior().regenMode == ShieldMode::SpeedScaled &&
				shield.GetModuleBehavior().speedMultiplierAtMax == 1.5f,
				"SpeedScaled level two reaches the live shield through the real applier");
			// LegacyVelocity (the default) resets velocity to the zero input target every tick, so a
			// stationary-by-input ship would always measure 0. ThrustDrift keeps drift velocity, as in ArenaLevel.
			f.ship->SetMovementMode(ShipMovementMode::ThrustDrift);

			SetShield(*f.ship, 0.5f * maxShield);
			f.ship->SetVelocity({ 0.f, 0.f });
			float before = shield.GetShield();
			Advance(f, 1.f);
			report.Check(std::abs(shield.GetShield() - before) < 1e-4f, "stationary SpeedScaled ship has no passive regen",
				Json{ { "gain", shield.GetShield() - before } });

			f.ship->SetVelocity({ 40.f, 0.f });
			before = shield.GetShield();
			Advance(f, 1.f);
			const float fastSpeed = Speed(*f.ship);
			const float fastGain = shield.GetShield() - before;
			f.ship->SetVelocity({ 20.f, 0.f });
			before = shield.GetShield();
			Advance(f, 1.f);
			const float slowSpeed = Speed(*f.ship);
			const float slowGain = shield.GetShield() - before;
			report.Check(fastGain > 0.f && slowGain > 0.f &&
				std::abs(fastGain * slowSpeed - slowGain * fastSpeed) <= 0.02f * fastGain * slowSpeed,
				"SpeedScaled regen is proportional to measured speed below the cap",
				Json{ { "fastGain", fastGain }, { "slowGain", slowGain }, { "fastSpeed", fastSpeed }, { "slowSpeed", slowSpeed } });

			auto enemy = Enemy(f);
			Hit(*f.ship, enemy.get(), 10.f, Plain());
			f.ship->SetVelocity({ 40.f, 0.f });
			before = shield.GetShield();
			Advance(f, 0.5f);
			report.Check(shield.GetShield() > before && shield.GetRechargeDelayRemaining() == 0.f,
				"SpeedScaled regenerates on the first tick after damage with no recharge delay",
				Json{ { "gain", shield.GetShield() - before }, { "delayRemaining", shield.GetRechargeDelayRemaining() } });
			f.Remove(speedInstance);
		}

		void ContinuousChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			ShieldComponent& shield = f.ship->GetShieldComponent();
			const float normal = NormalRegen(*f.ship);
			auto enemy = Enemy(f);

			SetShield(*f.ship, 0.5f * shield.GetMaxShield());
			Hit(*f.ship, enemy.get(), 10.f, Plain());
			float before = shield.GetShield();
			Advance(f, 1.f);
			report.Check(std::abs(shield.GetShield() - before) < 1e-4f && shield.GetRechargeDelayRemaining() > 0.f,
				"baseline Normal regen waits out the recharge delay after damage",
				Json{ { "gain", shield.GetShield() - before }, { "delayRemaining", shield.GetRechargeDelayRemaining() } });

			f.Install(kContinuousId, 0, 1);
			Hit(*f.ship, enemy.get(), 10.f, Plain());
			before = shield.GetShield();
			Advance(f, 1.f);
			const float gain = shield.GetShield() - before;
			const float expected = normal * 0.5f;
			report.Check(normal > 0.f && std::abs(gain - expected) <= 0.01f * expected + 1e-4f,
				"Continuous regen runs while damaged at the rate fraction with no delay",
				Json{ { "gain", gain }, { "expected", expected } });
		}

		// Breaks the shield three times: fill to 50%, refill to 10% and break, refill to 25% and break.
		// Returns the ShieldBroken events raised by each break.
		std::array<std::size_t, 3> BreakEvents(Report& report, Application& application, const char* probeId)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			const float maxShield = f.ship->GetShieldComponent().GetMaxShield();
			auto enemy = Enemy(f);
			f.Install(probeId, 0, 1);
			const auto breakAt = [&](float fill) -> std::size_t
			{
				SetShield(*f.ship, fill);
				Hit(*f.ship, enemy.get(), fill, Plain());
				const std::size_t events = f.ship->GetModuleTriggerRuntime().GetActiveBuffCount();
				f.ship->GetModuleTriggerRuntime().Tick(10.f);
				return events;
			};
			const std::size_t first = breakAt(0.5f * maxShield);
			const std::size_t second = breakAt(0.1f * maxShield);
			const std::size_t third = breakAt(0.25f * maxShield);
			return { first, second, third };
		}

		void RearmChecks(Report& report, Application& application)
		{
			const auto rearm = BreakEvents(report, application, kRearmId);
			report.Check(rearm[0] == 1 && rearm[1] == 0 && rearm[2] == 1,
				"breakRearmFraction latches ShieldBroken until the shield refills to 25 percent",
				Json{ { "events", Json::array({ rearm[0], rearm[1], rearm[2] }) } });
			const auto baseline = BreakEvents(report, application, kBrokenId);
			report.Check(baseline[0] == 1 && baseline[1] == 1 && baseline[2] == 1,
				"with no re-arm fraction every break raises ShieldBroken as before",
				Json{ { "events", Json::array({ baseline[0], baseline[1], baseline[2] }) } });
		}

		// Expected hull loss for 30 source damage into a 10-point shield, with Armor reduction m on the hull and
		// the shield share f = 1 - share * (1 - m). Armor is applied once to what passes the shield.
		float ExpectedArmorHullLoss(float armor, bool share)
		{
			const float m = sas::AttributeMath::GetArmorDamageMultiplier(armor);
			const float factor = share ? 1.f - 0.5f * (1.f - m) : 1.f;
			const float absorbedSource = std::min(30.f, 10.f / factor);
			return (30.f - absorbedSource) * m;
		}

		void ArmorChecks(Report& report, Application& application)
		{
			const auto armorCase = [&](bool withModule, float armorBase) -> Json
			{
				Fixture f{ report, application };
				f.rules.lookup = FixtureLookup;
				if (withModule) f.Install(kArmorId, 0, 1);
				f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, armorBase);
				const float armor = f.ship->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::Armor);
				f.ship->GetHealthComponent().SetMaxHealth(1000.f);
				f.ship->GetHealthComponent().Regenerate(1000.f);
				SetShield(*f.ship, 10.f);
				auto enemy = Enemy(f);
				const float healthBefore = f.ship->GetHealthComponent().GetHealth();
				Hit(*f.ship, enemy.get(), 30.f, Plain());
				return Json{ { "armor", armor }, { "healthLoss", healthBefore - f.ship->GetHealthComponent().GetHealth() },
					{ "shieldLeft", f.ship->GetShieldComponent().GetShield() } };
			};
			const Json shareNoArmor = armorCase(false, 0.f);
			const Json shareNoArmorWithModule = armorCase(true, 0.f);
			const Json noShareArmor = armorCase(false, 100.f);
			const Json shareArmor = armorCase(true, 100.f);
			// Each case is checked against the Armor value it actually ran with, so ship-default Armor modifiers are harmless.
			const float zeroArmor = shareNoArmor["armor"].get<float>();
			const float armorValue = noShareArmor["armor"].get<float>();
			const float healthNoArmor = shareNoArmor["healthLoss"].get<float>();
			const float healthNoArmorShare = shareNoArmorWithModule["healthLoss"].get<float>();
			const float healthNoShare = noShareArmor["healthLoss"].get<float>();
			const float healthShare = shareArmor["healthLoss"].get<float>();
			report.Check(std::abs(healthNoArmor - ExpectedArmorHullLoss(zeroArmor, false)) < 0.01f,
				"armorShare 0 applies Armor once to the overflow (low Armor case)",
				Json{ { "armor", zeroArmor }, { "healthLoss", healthNoArmor } });
			report.Check(std::abs(healthNoArmorShare - ExpectedArmorHullLoss(zeroArmor, true)) < 0.01f,
				"armorShare 0.5 matches its formula at the low Armor case",
				Json{ { "armor", zeroArmor }, { "healthLoss", healthNoArmorShare } });
			report.Check(std::abs(healthNoShare - ExpectedArmorHullLoss(armorValue, false)) < 0.01f,
				"armorShare 0 applies Armor once to the overflow",
				Json{ { "armor", armorValue }, { "healthLoss", healthNoShare } });
			report.Check(std::abs(healthShare - ExpectedArmorHullLoss(armorValue, true)) < 0.01f &&
				(armorValue <= 0.f || healthShare < healthNoShare),
				"armorShare 0.5 lets the shield take more source damage with no double Armor on the overflow",
				Json{ { "armor", armorValue }, { "healthLoss", healthShare }, { "healthLossNoShare", healthNoShare } });
			const float shieldLeft = shareArmor["shieldLeft"].get<float>();
			report.Check(std::abs(shieldLeft) < 1e-4f && std::abs(noShareArmor["shieldLeft"].get<float>()) < 1e-4f,
				"both armor cases spend the whole 10-point shield on 30 damage");
		}

		void RemovalAndPriorityChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			ShieldComponent& shield = f.ship->GetShieldComponent();
			const auto speedInstance = f.Install(kSpeedId, 0, 1);
			const auto continuousInstance = f.Install(kContinuousId, 0, 1);
			report.Check(shield.GetModuleBehavior().regenMode == ShieldMode::Continuous &&
				shield.GetModuleBehavior().continuousRateFraction == 0.5f,
				"Continuous outranks SpeedScaled when both shield rules are placed");
			f.Remove(continuousInstance);
			report.Check(shield.GetModuleBehavior().regenMode == ShieldMode::SpeedScaled &&
				shield.GetModuleBehavior().speedMultiplierAtMax == 1.f,
				"removing the Continuous rule falls back to SpeedScaled");
			f.Remove(speedInstance);
			const ShieldModuleBehavior& cleared = shield.GetModuleBehavior();
			report.Check(cleared.regenMode == ShieldMode::Normal && cleared.breakRearmFraction == 0.f && cleared.armorShare == 0.f,
				"removing every shield rule restores the default behaviour");
			auto enemy = Enemy(f);
			SetShield(*f.ship, 0.5f * shield.GetMaxShield());
			Hit(*f.ship, enemy.get(), 10.f, Plain());
			const float before = shield.GetShield();
			Advance(f, 1.f);
			report.Check(std::abs(shield.GetShield() - before) < 1e-4f && shield.GetRechargeDelayRemaining() > 0.f,
				"after removal the normal recharge delay applies again");
		}

		void LoaderChecks(Report& report, const std::filesystem::path& root, const Json& shipped)
		{
			const auto reject = [&](const char* name, const Json& rules)
			{
				const auto path = WriteJson(root, std::string{ "reject-" } + name + ".json", FixtureSet(shipped, rules));
				const auto loaded = content::ModuleLoader::LoadFromFile(path);
				report.Check(!loaded.Succeeded(), std::string{ "shieldRules loader rejects " } + name, loaded.error);
			};
			reject("unknown-regen-mode", Json::array({ Json{ { "regenMode", "Fast" }, { "speedMultiplierAtMax", 1.0 } } }));
			reject("speed-without-multiplier", Json::array({ Json{ { "regenMode", "SpeedScaled" } } }));
			reject("zero-speed-multiplier", Json::array({ Json{ { "regenMode", "SpeedScaled" }, { "speedMultiplierAtMax", 0.0 } } }));
			reject("speed-field-on-normal", Json::array({ Json{ { "speedMultiplierAtMax", 1.5 }, { "armorShare", 0.5 } } }));
			reject("continuous-without-rate", Json::array({ Json{ { "regenMode", "Continuous" } } }));
			reject("continuous-rate-above-one", Json::array({ Json{ { "regenMode", "Continuous" }, { "continuousRateFraction", 1.5 } } }));
			reject("rate-field-on-speed-scaled", Json::array({ Json{ { "regenMode", "SpeedScaled" }, { "speedMultiplierAtMax", 2.0 }, { "continuousRateFraction", 0.5 } } }));
			reject("armor-share-above-one", Json::array({ Json{ { "armorShare", 1.5 } } }));
			reject("negative-rearm", Json::array({ Json{ { "breakRearmFraction", -0.1 } } }));
			reject("unknown-field", Json::array({ Json{ { "armorShare", 0.5 }, { "shield", 3 } } }));
			reject("no-effect-entry", Json::array({ Json{ { "regenMode", "Normal" } } }));
			reject("short-speed-table", Json::array({ Json{ { "regenMode", "SpeedScaled" }, { "speedMultiplierAtMax", Json::array({ 1.0, 1.5 }) } } }));
			reject("rules-not-array", Json::object());
			reject("entry-not-object", Json::array({ 1.0 }));
			const auto shippedLoaded = content::ModuleLoader::LoadFromFile(
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json");
			report.Check(shippedLoaded.Succeeded(), "shipped modules still load through the shieldRules parser", shippedLoaded.error);
		}
	}
}

int main(int argc, char** argv)
{
	using namespace rare_shield_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare shield content registration failed" };
		std::string title = "Rare Shield Rules E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);

		const std::filesystem::path root = std::filesystem::path{ argv[1] }.parent_path() / "rare-shield-fixtures";
		Json shipped;
		{
			std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json" };
			input >> shipped;
		}
		const Json validSpeedRules = Json::array({ Json{ { "regenMode", "SpeedScaled" }, { "speedMultiplierAtMax", Json::array({ 1.0, 1.5, 2.0 }) } } });
		const auto fixturePath = WriteJson(root, "rare-shield-modules.json", FixtureSet(shipped, validSpeedRules));
		const auto loaded = content::ModuleLoader::LoadFromFile(fixturePath);
		report.Check(loaded.Succeeded(), "rare shield fixture modules load through the real ModuleLoader", loaded.error);
		gFixtureDefinitions = loaded.definitions;

		SpeedChecks(report, application);
		ContinuousChecks(report, application);
		RearmChecks(report, application);
		ArmorChecks(report, application);
		RemovalAndPriorityChecks(report, application);
		LoaderChecks(report, root, shipped);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareShieldRulesE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
