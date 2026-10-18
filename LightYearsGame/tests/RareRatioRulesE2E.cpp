#include "gameConfigs/CollisionLayers.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"

#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/module/ModuleResolver.h"
#include "spaceShip/SpaceShip.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

namespace rare_ratio_rules_e2e
{
	using namespace vega;
	using namespace common_module_e2e;
	using namespace uncommon_p4_e2e;
	namespace
	{
		// Test-only hosts parsed from fixture JSON. They never enter shipped content.
		constexpr const char* kRatioHost = "Module.Offense.RatioHost.Rare";
		constexpr const char* kFlatHost = "Module.Offense.RatioFlatHost.Rare";
		constexpr const char* kUnderdogHost = "Module.Defense.UnderdogHost.Rare";
		List<ModuleDefinition> gFixtureModules;

		const ModuleDefinition* FixtureModule(const sas::ContentId& id)
		{
			for (const ModuleDefinition& definition : gFixtureModules)
				if (definition.moduleId == id) return &definition;
			return content::ModuleContentCatalog::FindById(id);
		}

		void Wire(Fixture& f)
		{
			f.rules.lookup = FixtureModule;
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
			if (!output) throw std::runtime_error{ "Could not write rare ratio fixture" };
			output << document.dump(2) << '\n';
		}

		// One 1-cell variant. damageRules and targetSlowRules are attached only when given.
		Json Variant(const Json& damageRules = Json(), const Json& targetSlowRules = Json())
		{
			Json variant{ { "shape", Json::array({ Json::array({ 0, 0 }) }) } };
			if (!damageRules.is_null()) variant["damageRules"] = damageRules;
			if (!targetSlowRules.is_null()) variant["targetSlowRules"] = targetSlowRules;
			return variant;
		}

		Json Host(const char* id, const char* family, Json variant)
		{
			return Json{
				{ "id", id }, { "displayName", id }, { "rarity", "Rare" }, { "levelCount", 3 },
				{ "family", family }, { "sizeLadder", Json::array({ 1 }) },
				{ "sizeVariants", Json::array({ std::move(variant) }) }
			};
		}

		// Shipped zone table plus the given test hosts only.
		Json ModulesDocument(Json hosts)
		{
			Json document = ShippedJson("modules.json");
			document["modules"] = std::move(hosts);
			return document;
		}

		// Ratio rule: x1.10 / x1.15 / x1.20 at threshold 1.5, +0.05 per unit of ratio above it. Toppler rides on the same host.
		Json RatioRules(bool withSlope = true)
		{
			Json rule{ { "condition", "TargetMaxHealthRatioAtLeast" }, { "conditionValue", 1.5 }, { "damageMultiplier", Json::array({ 1.10, 1.15, 1.20 }) } };
			if (withSlope) rule["ratioSlope"] = 0.05;
			return Json::array({ rule });
		}

		Json ToppleRules()
		{
			return Json::array({ Json{ { "minRatio", 2.5 }, { "slowFraction", 0.3 }, { "bossSlowFraction", 0.15 }, { "seconds", 2.0 } } });
		}

		Json FixtureDocument()
		{
			return ModulesDocument(Json::array({
				Host(kRatioHost, "Ability.Offense", Variant(RatioRules(), ToppleRules())),
				Host(kFlatHost, "Ability.Offense", Variant(Json::array({ Json{ { "condition", "TargetMaxHealthRatioAtLeast" }, { "conditionValue", 1.5 }, { "damageMultiplier", 1.10 } } }))),
				Host(kUnderdogHost, "Ability.Defense", Variant(Json::array({ Json{ { "incoming", true }, { "condition", "SourceMaxHealthGreaterThanOwner" }, { "damageMultiplier", 0.92 } } })))
			}));
		}

		void LoadFixtures(Report& report, const std::filesystem::path& root)
		{
			const auto path = root / "rare-ratio-modules.json";
			WriteJson(path, FixtureDocument());
			const auto loaded = content::ModuleLoader::LoadFromFile(path);
			report.Check(loaded.Succeeded(), "rare ratio fixture modules load through the real ModuleLoader", Json{ { "error", loaded.error } });
			gFixtureModules = loaded.definitions;
		}

		// Loads a single-host catalog whose only variant carries the given rules. The result is the loader outcome.
		content::ModuleLoader::Result LoadVariant(const std::filesystem::path& root, const std::string& name, const Json& damageRules, const Json& targetSlowRules = Json())
		{
			const auto path = root / (name + ".json");
			WriteJson(path, ModulesDocument(Json::array({ Host("Module.Offense.RejectHost.Rare", "Ability.Offense", Variant(damageRules, targetSlowRules)) })));
			return content::ModuleLoader::LoadFromFile(path);
		}

		void LoaderChecks(Report& report, const std::filesystem::path& root)
		{
			struct Reject { const char* name; const char* damageRules; const char* targetSlowRules; const char* expected; };
			const Reject rejects[] = {
				{ "ratio-without-value", R"([{"condition":"TargetMaxHealthRatioAtLeast","damageMultiplier":1.1}])", "", "TargetMaxHealthRatioAtLeast requires a conditionValue of at least 1" },
				{ "ratio-below-one", R"([{"condition":"TargetMaxHealthRatioAtLeast","conditionValue":0.5,"damageMultiplier":1.1}])", "", "TargetMaxHealthRatioAtLeast requires a conditionValue of at least 1" },
				{ "ratio-level-below-one", R"([{"condition":"TargetMaxHealthRatioAtLeast","conditionValue":[1.5,0.5,1.5],"damageMultiplier":1.1}])", "", "must be at least 1" },
				{ "ratio-negative-slope", R"([{"condition":"TargetMaxHealthRatioAtLeast","conditionValue":1.5,"damageMultiplier":1.1,"ratioSlope":-0.1}])", "", "ratioSlope must not be negative" },
				{ "slope-on-always", R"([{"condition":"Always","damageMultiplier":1.1,"ratioSlope":0.05}])", "", "ratioSlope requires an outgoing TargetMaxHealthRatioAtLeast rule" },
				{ "incoming-ratio", R"([{"incoming":true,"condition":"TargetMaxHealthRatioAtLeast","conditionValue":1.5,"damageMultiplier":0.9}])", "", "incoming rules require" },
				{ "outgoing-source-max", R"([{"condition":"SourceMaxHealthGreaterThanOwner","damageMultiplier":0.92}])", "", "SourceMaxHealthGreaterThanOwner is an incoming condition" },
				{ "source-max-with-value", R"([{"incoming":true,"condition":"SourceMaxHealthGreaterThanOwner","conditionValue":1,"damageMultiplier":0.92}])", "", "this damage condition does not accept conditionValue" },
				{ "slow-min-ratio-below-one", "", R"([{"minRatio":0.5,"slowFraction":0.3,"bossSlowFraction":0.15,"seconds":2}])", "minRatio must be at least 1" },
				{ "slow-fraction-zero", "", R"([{"minRatio":2.5,"slowFraction":0,"bossSlowFraction":0.15,"seconds":2}])", "slowFraction must be between 0 and 1" },
				{ "slow-fraction-one", "", R"([{"minRatio":2.5,"slowFraction":0.3,"bossSlowFraction":1,"seconds":2}])", "bossSlowFraction must be between 0 and 1" },
				{ "slow-seconds-zero", "", R"([{"minRatio":2.5,"slowFraction":0.3,"bossSlowFraction":0.15,"seconds":0}])", "seconds must be positive" },
				{ "slow-missing-seconds", "", R"([{"minRatio":2.5,"slowFraction":0.3,"bossSlowFraction":0.15}])", "seconds is required" },
				{ "slow-unknown-field", "", R"([{"minRatio":2.5,"slowFraction":0.3,"bossSlowFraction":0.15,"seconds":2,"extra":1}])", "unknown field: extra" }
			};
			for (const Reject& item : rejects)
			{
				const Json damage = *item.damageRules ? Json::parse(item.damageRules) : Json();
				const Json slow = *item.targetSlowRules ? Json::parse(item.targetSlowRules) : Json();
				const auto loaded = LoadVariant(root, std::string{ "reject-" } + item.name, damage, slow);
				report.Check(!loaded.Succeeded() && loaded.error.find(item.expected) != std::string::npos,
					std::string{ "Loader rejects invalid ratio or slow rule: " } + item.name,
					Json{ { "expected", item.expected }, { "error", loaded.error } });
			}

			const auto accepted = LoadVariant(root, "accept-ratio-slope", Json::parse(R"([{"condition":"TargetMaxHealthRatioAtLeast","conditionValue":1.5,"damageMultiplier":1.1,"ratioSlope":0.05}])"));
			report.Check(accepted.Succeeded() && accepted.definitions.size() == 1 && accepted.definitions.front().sizeVariants.front().damageRules.front().ratioSlope == 0.05f,
				"Loader accepts a ratio rule with ratioSlope", Json{ { "error", accepted.error } });
			const auto flat = LoadVariant(root, "accept-ratio-flat", Json::parse(R"([{"condition":"TargetMaxHealthRatioAtLeast","conditionValue":1.5,"damageMultiplier":1.1}])"));
			report.Check(flat.Succeeded(), "Loader accepts a flat ratio rule without ratioSlope", Json{ { "error", flat.error } });
			const auto underdog = LoadVariant(root, "accept-underdog", Json::parse(R"([{"incoming":true,"condition":"SourceMaxHealthGreaterThanOwner","damageMultiplier":0.92}])"));
			report.Check(underdog.Succeeded(), "Loader accepts an incoming SourceMaxHealthGreaterThanOwner rule", Json{ { "error", underdog.error } });
			const auto slow = LoadVariant(root, "accept-slow", Json(), Json::parse(R"([{"minRatio":2.5,"slowFraction":0.3,"bossSlowFraction":0.15,"seconds":2}])"));
			report.Check(slow.Succeeded() && slow.definitions.front().sizeVariants.front().targetSlowRules.size() == 1,
				"Loader accepts a valid targetSlowRules entry", Json{ { "error", slow.error } });
		}

		std::shared_ptr<SpaceShip> Target(Fixture& f, float maxHealth, ControlTargetClass targetClass = ControlTargetClass::Normal)
		{
			const auto enemy = f.level->SpawnActor<SpaceShip>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock();
			f.level->TickInternal(0.f);
			enemy->SetCollisionLayer(vega::GameCollision::Enemy);
			enemy->SetCollisionMask(CollisionLayer::None);
			enemy->SetEnablePhysics(false);
			enemy->SetActorLocation({ 20000.f, 20000.f });
			enemy->GetShieldComponent().ChangeShield(-10000.f);
			enemy->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f);
			enemy->GetHealthComponent().SetMaxHealth(maxHealth);
			enemy->GetHealthComponent().Regenerate(maxHealth);
			enemy->SetControlTargetClass(targetClass);
			f.report.Check(enemy != nullptr, "Rare ratio E2E spawns a real enemy SpaceShip");
			return enemy;
		}

		// Health removed by one real outgoing hit from the owner, then the target is restored.
		float Dealt(Fixture& f, SpaceShip& target, float damage = 100.f)
		{
			const float dealt = Hit(target, f.ship.get(), damage, Plain());
			target.GetHealthComponent().Regenerate(target.GetHealthComponent().GetMaxHealth());
			return dealt;
		}

		// Health removed by one real incoming hit from the source, with the owner shield emptied first.
		float Incoming(Fixture& f, Actor& source, float damage = 100.f)
		{
			f.ship->GetShieldComponent().ChangeShield(-f.ship->GetShieldComponent().GetShield());
			const float dealt = Hit(*f.ship, &source, damage, Plain());
			f.ship->GetHealthComponent().Regenerate(f.ship->GetHealthComponent().GetMaxHealth());
			return dealt;
		}

		bool HasSlow(SpaceShip& target)
		{
			return target.GetAbilitySystemComponent().FindGameplayEffectById(MovementEffectSchema::SlowEffectId) != nullptr;
		}

		float SlowMagnitude(SpaceShip& target)
		{
			return target.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::MovementSlow);
		}

		void TickTarget(SpaceShip& target, float seconds)
		{
			target.GetAbilitySystemComponent().Tick(seconds);
		}

		void OutgoingRatioChecks(Report& report, Application& application)
		{
			Fixture base{ report, application };
			Wire(base);
			const float baseline = Dealt(base, *Target(base, 2.f * base.ship->GetHealthComponent().GetMaxHealth()));
			report.Check(baseline > 0.f, "Baseline outgoing hit deals real health damage", Json{ { "baseline", baseline } });

			Fixture f{ report, application };
			Wire(f);
			f.Install(kRatioHost, 0, 1);
			const float ownerMax = f.ship->GetHealthComponent().GetMaxHealth();
			report.Check(ownerMax > 0.f, "Owner has positive MaxHealth after install", Json{ { "ownerMax", ownerMax } });
			const float ratios[] = { 1.4f, 1.5f, 3.f, 10.f };
			const float expected[] = { 1.f, 1.10f, 1.175f, 1.525f };
			for (std::size_t index = 0; index < 4; ++index)
			{
				const auto target = Target(f, ratios[index] * ownerMax);
				const float dealt = Dealt(f, *target, 100.f);
				report.CheckValue("Outgoing ratio " + std::to_string(ratios[index]) + " multiplies by the scaled bonus",
					expected[index], dealt / baseline, "L1 ratio " + std::to_string(ratios[index]));
			}

			// Level tables: the authored base is x1.15 at level 2 and x1.20 at level 3.
			Fixture level2{ report, application };
			Wire(level2);
			level2.Install(kRatioHost, 0, 2);
			const float owner2 = level2.ship->GetHealthComponent().GetMaxHealth();
			report.CheckValue("Level 2 base at ratio 1.5 is x1.15", 1.15f, Dealt(level2, *Target(level2, 1.5f * owner2), 100.f) / baseline, "L2 ratio 1.5");

			Fixture level3{ report, application };
			Wire(level3);
			level3.Install(kRatioHost, 0, 3);
			const float owner3 = level3.ship->GetHealthComponent().GetMaxHealth();
			report.CheckValue("Level 3 base at ratio 1.5 is x1.20", 1.20f, Dealt(level3, *Target(level3, 1.5f * owner3), 100.f) / baseline, "L3 ratio 1.5");

			// Zero slope keeps the authored flat multiplier at every ratio above the threshold.
			Fixture flat{ report, application };
			Wire(flat);
			flat.Install(kFlatHost, 0, 1);
			const float flatOwner = flat.ship->GetHealthComponent().GetMaxHealth();
			report.CheckValue("Zero slope: ratio 3 keeps the flat x1.10", 1.10f, Dealt(flat, *Target(flat, 3.f * flatOwner), 100.f) / baseline, "flat ratio 3");
			report.CheckValue("Zero slope: ratio 1.4 stays unchanged", 1.f, Dealt(flat, *Target(flat, 1.4f * flatOwner), 100.f) / baseline, "flat ratio 1.4");

			// Owner MaxHealth changes move the ratio: the same target drops from 3x to 1.5x when the owner doubles.
			Fixture moving{ report, application };
			Wire(moving);
			moving.Install(kRatioHost, 0, 1);
			const float movingOwner = moving.ship->GetHealthComponent().GetMaxHealth();
			const auto stable = Target(moving, 3.f * movingOwner);
			report.CheckValue("Ratio 3 before the owner grows applies x1.175", 1.175f, Dealt(moving, *stable, 100.f) / baseline, "owner before growth");
			moving.ship->GetHealthComponent().SetMaxHealth(2.f * movingOwner);
			report.CheckValue("Owner MaxHealth growth lowers the ratio to 1.5 and the bonus to x1.10", 1.10f,
				Dealt(moving, *stable, 100.f) / baseline, "owner after growth");
		}

		void UnderdogChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			Wire(f);
			const float ownerMax = f.ship->GetHealthComponent().GetMaxHealth();
			const auto larger = Target(f, 1.5f * ownerMax);
			const auto equal = Target(f, ownerMax);
			const auto smaller = Target(f, 0.5f * ownerMax);
			const float baseLarger = Incoming(f, *larger);
			const float baseEqual = Incoming(f, *equal);
			const float baseSmaller = Incoming(f, *smaller);
			report.Check(baseLarger > 0.f && baseEqual > 0.f && baseSmaller > 0.f, "Baseline incoming hits deal real health damage");

			f.Install(kUnderdogHost, 0, 1);
			report.CheckValue("Underdog: larger attacker deals x0.92", 0.92f, Incoming(f, *larger) / baseLarger, "source MaxHealth 1.5x owner");
			report.CheckValue("Underdog: equal MaxHealth attacker is unchanged", 1.f, Incoming(f, *equal) / baseEqual, "source MaxHealth equal to owner");
			report.CheckValue("Underdog: smaller attacker is unchanged", 1.f, Incoming(f, *smaller) / baseSmaller, "source MaxHealth 0.5x owner");
		}

		void ToppleChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			Wire(f);
			const std::uint32_t instance = f.Install(kRatioHost, 0, 1);
			const float ownerMax = f.ship->GetHealthComponent().GetMaxHealth();

			const auto below = Target(f, 2.4f * ownerMax);
			Dealt(f, *below, 100.f);
			report.Check(!HasSlow(*below), "Toppler: a ratio of 2.4 does not slow the target");

			const auto slowed = Target(f, 2.5f * ownerMax);
			Dealt(f, *slowed, 100.f);
			report.Check(HasSlow(*slowed), "Toppler: a ratio of 2.5 slows the target");
			report.CheckValue("Toppler: normal target slows by 30 percent", 0.3f, SlowMagnitude(*slowed), "Normal target");

			const auto miniBoss = Target(f, 2.5f * ownerMax, ControlTargetClass::MiniBoss);
			Dealt(f, *miniBoss, 100.f);
			report.CheckValue("Toppler: MiniBoss is not a Boss and slows by 30 percent", 0.3f, SlowMagnitude(*miniBoss), "MiniBoss target");

			const auto boss = Target(f, 2.5f * ownerMax, ControlTargetClass::Boss);
			Dealt(f, *boss, 100.f);
			report.CheckValue("Toppler: Boss slows by 15 percent", 0.15f, SlowMagnitude(*boss), "Boss target");

			TickTarget(*slowed, 1.5f);
			report.Check(HasSlow(*slowed), "Toppler: the slow still holds at 1.5 seconds");
			TickTarget(*slowed, 0.6f);
			report.Check(!HasSlow(*slowed), "Toppler: the slow expires after 2 seconds");

			// A repeat hit inside the window refreshes the slow without stacking it.
			const auto repeated = Target(f, 2.5f * ownerMax);
			Dealt(f, *repeated, 100.f);
			TickTarget(*repeated, 1.5f);
			Dealt(f, *repeated, 100.f);
			report.CheckValue("Toppler: a repeat hit does not stack the slow", 0.3f, SlowMagnitude(*repeated), "repeat hit");
			TickTarget(*repeated, 1.5f);
			report.Check(HasSlow(*repeated), "Toppler: a repeat hit restarts the 2 second window");
			TickTarget(*repeated, 0.6f);
			report.Check(!HasSlow(*repeated), "Toppler: the refreshed slow expires 2 seconds after the repeat hit");

			// Removing the module ends every slow it applied.
			const auto removed = Target(f, 2.5f * ownerMax);
			Dealt(f, *removed, 100.f);
			report.Check(HasSlow(*removed), "Toppler: the slow is present before the module is removed");
			f.Remove(instance);
			report.Check(!HasSlow(*removed), "Toppler: removing the module clears the slow it applied");
		}
	}

	void RunChecks(Report& report, Application& application, const std::filesystem::path& fixtureRoot)
	{
		LoadFixtures(report, fixtureRoot);
		LoaderChecks(report, fixtureRoot);
		OutgoingRatioChecks(report, application);
		UnderdogChecks(report, application);
		ToppleChecks(report, application);
	}
}

int main(int argc, char** argv)
{
	vega::InstallVegaEngineAssets();
	using namespace rare_ratio_rules_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare ratio rules content registration failed" };
		std::string title = "Rare Ratio Rules E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		RunChecks(report, application, std::filesystem::path{ argv[1] }.parent_path() / "rare-ratio-fixtures");
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareRatioRulesE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
