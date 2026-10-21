#include "framework/EngineContext.h"
#include "gameConfigs/CollisionLayers.h"
#include "gameFramework/LightYearsAssets.h"
#include "CommonModuleE2ETestSupport.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/module/ModuleRulePredicates.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/tags/GameplayTags.h"
#include "spaceShip/SpaceShip.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

namespace rare_damage_conditions_e2e
{
	using namespace vega;
	using namespace common_module_e2e;
	namespace
	{
		constexpr const char* kArcExtenderId = "Module.Offense.ArcExtender.Uncommon";

		struct TextCase
		{
			const char* name;
			const char* rules;
			const char* expected;
		};

		DamagePayload Plain()
		{
			DamagePayload payload;
			payload.criticalPolicy = DamageCriticalPolicy::Disabled;
			payload.kineticStacks = 0;
			return payload;
		}

		struct Harness
		{
			Report& report;
			std::shared_ptr<HeadlessModuleLevel> level;
			std::shared_ptr<PlayerSpaceShip> ship;
			CombatRuntime::OutgoingDamageRuleRegistration registration;
			Json modules;
			std::filesystem::path fixtureRoot;

			Harness(Report& result, Application& application, const std::filesystem::path& root)
				: report{ result }, level{ std::make_shared<HeadlessModuleLevel>(&application) }, fixtureRoot{ root }
			{
				const auto dataPath = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data";
				std::ifstream input{ dataPath / "modules.json" };
				report.Check(static_cast<bool>(input), "Shipped modules.json can be opened");
				input >> modules;
				level->BeginPlayInternal();
				const auto& definition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
				ship = level->SpawnActor<PlayerSpaceShip>(definition).lock();
				report.Check(ship != nullptr, "Rare damage E2E spawns a real player ship");
				level->TickInternal(0.f);
				ship->SetInvulnerability(false);
				ship->SetUseScreenClamp(false);
				ship->SetActorLocation({ 10000.f, 10000.f });
				registration = ship->GetCombatRuntime().RegisterOutgoingDamageRules();
			}
		};

		// Writes a copy of the shipped module catalog whose Arc Extender variant carries the given
		// damage rules, then loads it through the real ModuleLoader.
		content::ModuleLoader::Result LoadRuleFixture(const Harness& harness, const std::string& name, const Json& damageRules)
		{
			Json changed = harness.modules;
			for (auto& module : changed["modules"])
				if (module["id"] == kArcExtenderId) module["sizeVariants"][0]["damageRules"] = damageRules;
			std::filesystem::create_directories(harness.fixtureRoot);
			const std::filesystem::path path = harness.fixtureRoot / (name + ".json");
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			if (!output) throw std::runtime_error{ "Could not write rare damage fixture" };
			output << changed.dump(2) << '\n';
			output.close();
			return content::ModuleLoader::LoadFromFile(path);
		}

		List<ModuleDamageRule> ParsedRules(const content::ModuleLoader::Result& loaded)
		{
			for (const ModuleDefinition& definition : loaded.definitions)
				if (definition.moduleId.ToString() == kArcExtenderId && !definition.sizeVariants.empty())
					return definition.sizeVariants.front().damageRules;
			return {};
		}

		void SetRules(Harness& harness, const List<ModuleDamageRule>& rules)
		{
			List<ResolvedModuleDamageRule> resolved;
			std::size_t index = 0;
			for (const ModuleDamageRule& rule : rules)
			{
				ResolvedModuleDamageRule entry;
				entry.instanceId = 1;
				entry.ruleIndex = index++;
				entry.rule = rule;
				resolved.push_back(std::move(entry));
			}
			harness.registration.ReplaceRules(resolved);
		}

		// Health delta of one real hit, then the target is restored to full health.
		float DealtDamage(Actor& target, Actor* source, float damage = 10.f)
		{
			HealthComponent* health = target.FindComponent<HealthComponent>();
			const float before = health->GetHealth();
			ApplyCombatDamage(target, damage, source, {}, Plain());
			const float dealt = before - health->GetHealth();
			health->Regenerate(health->GetMaxHealth());
			return dealt;
		}

		// Measures the same hit with and without the rules on an identical target state.
		float Ratio(Harness& harness, Actor& target, const List<ModuleDamageRule>& rules)
		{
			SetRules(harness, rules);
			const float withRule = DealtDamage(target, harness.ship.get());
			SetRules(harness, {});
			const float withoutRule = DealtDamage(target, harness.ship.get());
			harness.report.Check(withoutRule > 0.f, "Baseline hit deals real health damage", Json{ { "withoutRule", withoutRule } });
			return withRule / withoutRule;
		}

		std::shared_ptr<SpaceShip> SpawnEnemy(Harness& harness, ControlTargetClass targetClass = ControlTargetClass::Normal)
		{
			const auto enemy = harness.level->SpawnActor<SpaceShip>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock();
			harness.level->TickInternal(0.f);
			enemy->SetCollisionLayer(vega::GameCollision::Enemy);
			enemy->SetCollisionMask(vega::CollisionLayer::None);
			enemy->SetEnablePhysics(false);
			enemy->SetActorLocation({ 20000.f, 20000.f });
			enemy->GetShieldComponent().ChangeShield(-10000.f);
			enemy->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f);
			enemy->GetHealthComponent().SetMaxHealth(1000.f);
			enemy->GetHealthComponent().Regenerate(1000.f);
			enemy->SetControlTargetClass(targetClass);
			harness.report.Check(enemy != nullptr, "Rare damage E2E spawns a real enemy SpaceShip");
			return enemy;
		}

		void ApplyStatus(SpaceShip& target, Actor* source, const GameplayTag& type)
		{
			DamagePayload payload = Plain();
			if (type == DamageTypeSchema::Electric) payload.electricStacks = 1;
			if (type == DamageTypeSchema::Thermal) payload.igniteStacks = 1;
			ApplyCombatDamage(target, 0.1f, source, List<GameplayTag>{ type }, payload);
		}

		int Stacks(const SpaceShip& target, const char* effectId)
		{
			const auto* effect = target.GetAbilitySystemComponent().FindGameplayEffectById(effectId);
			return effect ? effect->stackCount : 0;
		}

		void ActivateMovement(Harness& harness)
		{
			const List<GameplayTag> sourceTags{ GameplayTags::Ability::Movement };
			const List<GameplayTag> payloadTags{};
			ModuleTriggerEventContext event{ ModuleTriggerKind::AbilityActivated, 1.f, sourceTags, payloadTags };
			harness.ship->GetCombatRuntime().ActivateOutgoingDamageRules(event);
		}

		void Tick(Harness& harness, float seconds)
		{
			harness.ship->GetCombatRuntime().Tick(seconds);
		}

		float IncomingDealt(Harness& harness, Actor& source)
		{
			harness.ship->GetShieldComponent().ChangeShield(-100000.f);
			return DealtDamage(*harness.ship, &source);
		}

		void LoaderChecks(Harness& harness)
		{
			const TextCase rejects[] = {
				{ "unknown-condition", R"([{"condition":"Sometimes","damageMultiplier":1.2}])", "Unknown module damage condition" },
				{ "status-count-one", R"([{"condition":"TargetStatusKindsAtLeast","conditionValue":1,"damageMultiplier":1.2}])", "TargetStatusKindsAtLeast requires an integer conditionValue of at least 2" },
				{ "status-count-fraction", R"([{"condition":"TargetStatusKindsAtLeast","conditionValue":2.5,"damageMultiplier":1.2}])", "TargetStatusKindsAtLeast requires an integer conditionValue of at least 2" },
				{ "status-count-missing", R"([{"condition":"TargetStatusKindsAtLeast","damageMultiplier":1.2}])", "TargetStatusKindsAtLeast requires an integer conditionValue of at least 2" },
				{ "status-count-level-one", R"([{"condition":"TargetStatusKindsAtLeast","conditionValue":[2,1,3],"damageMultiplier":1.2}])", "must be an integer of at least 2" },
				{ "always-with-value", R"([{"condition":"Always","conditionValue":0.5,"damageMultiplier":1.2}])", "this damage condition does not accept conditionValue" },
				{ "boss-with-value", R"([{"condition":"TargetIsBossClass","conditionValue":1,"damageMultiplier":1.2}])", "this damage condition does not accept conditionValue" },
				{ "incoming-boss-class", R"([{"incoming":true,"condition":"TargetIsBossClass","damageMultiplier":0.85}])", "incoming rules require" },
				{ "incoming-owner-shield", R"([{"incoming":true,"condition":"OwnerShieldFull","damageMultiplier":0.85}])", "incoming rules require" },
				{ "incoming-status-count", R"([{"incoming":true,"condition":"TargetStatusKindsAtLeast","conditionValue":2,"damageMultiplier":0.85}])", "incoming rules require" },
				{ "incoming-window-without-duration", R"([{"incoming":true,"condition":"Always","damageMultiplier":0.85,"activeWhileTrigger":"AbilityActivated","activeWhileEventTags":["Ability.Movement"]}])", "active trigger requires activeDurationSeconds" },
				{ "tags-without-trigger", R"([{"condition":"Always","damageMultiplier":1.2,"activeWhileEventTags":["Ability.Movement"]}])", "require activeWhileTrigger" }
			};
			for (const TextCase& item : rejects)
			{
				const auto loaded = LoadRuleFixture(harness, std::string{ "reject-" } + item.name, Json::parse(item.rules));
				harness.report.Check(!loaded.Succeeded() && loaded.error.find(item.expected) != std::string::npos,
					std::string{ "Loader rejects invalid damage rule: " } + item.name,
					Json{ { "input", item.rules }, { "error", loaded.error } });
			}

			const TextCase accepts[] = {
				{ "always", R"([{"condition":"Always","damageMultiplier":1.2}])", "" },
				{ "owner-shield-full", R"([{"condition":"OwnerShieldFull","damageMultiplier":1.2}])", "" },
				{ "target-boss-class", R"([{"condition":"TargetIsBossClass","damageMultiplier":1.2}])", "" },
				{ "status-kinds-two", R"([{"condition":"TargetStatusKindsAtLeast","conditionValue":2,"damageMultiplier":1.2}])", "" },
				{ "status-kinds-level-table", R"([{"condition":"TargetStatusKindsAtLeast","conditionValue":[2,2,3],"damageMultiplier":1.2}])", "" },
				{ "incoming-always-window", R"([{"incoming":true,"condition":"Always","damageMultiplier":0.85,"activeWhileTrigger":"AbilityActivated","activeWhileEventTags":["Ability.Movement"],"activeDurationSeconds":3}])", "" },
				{ "incoming-source-status-window", R"([{"incoming":true,"condition":"SourceHasStatus","damageMultiplier":0.85,"activeWhileTrigger":"AbilityActivated","activeWhileEventTags":["Ability.Movement"],"activeDurationSeconds":3}])", "" }
			};
			for (const TextCase& item : accepts)
			{
				const auto loaded = LoadRuleFixture(harness, std::string{ "accept-" } + item.name, Json::parse(item.rules));
				harness.report.Check(loaded.Succeeded() && ParsedRules(loaded).size() == 1,
					std::string{ "Loader accepts damage rule: " } + item.name,
					Json{ { "input", item.rules }, { "error", loaded.error } });
			}
		}

		void OutgoingChecks(Harness& harness)
		{
			const auto normal = SpawnEnemy(harness);
			const List<ModuleDamageRule> always = ParsedRules(LoadRuleFixture(harness, "outgoing-always", Json::parse(R"([{"condition":"Always","damageMultiplier":1.2}])")));
			harness.report.CheckValue("Always multiplies every outgoing hit", 1.2f, Ratio(harness, *normal, always), "Always x1.2");

			const List<ModuleDamageRule> shield = ParsedRules(LoadRuleFixture(harness, "outgoing-owner-shield", Json::parse(R"([{"condition":"OwnerShieldFull","damageMultiplier":1.2}])")));
			auto& ownerShield = harness.ship->GetShieldComponent();
			harness.report.Check(ownerShield.GetMaxShield() > 0.f, "Player ship has shield capacity for OwnerShieldFull", Json{ { "maxShield", ownerShield.GetMaxShield() } });
			ownerShield.ChangeShield(ownerShield.GetMaxShield());
			harness.report.Check(ownerShield.GetShield() >= ownerShield.GetMaxShield(), "Owner shield is full before the shot");
			harness.report.CheckValue("OwnerShieldFull applies while the owner shield is full", 1.2f, Ratio(harness, *normal, shield), "owner shield full");
			ownerShield.ChangeShield(-ownerShield.GetMaxShield());
			harness.report.CheckValue("OwnerShieldFull does not apply once the owner shield is spent", 1.f, Ratio(harness, *normal, shield), "owner shield empty");

			const List<ModuleDamageRule> bossClass = ParsedRules(LoadRuleFixture(harness, "outgoing-boss-class", Json::parse(R"([{"condition":"TargetIsBossClass","damageMultiplier":1.2}])")));
			const struct { const char* name; ControlTargetClass value; float expected; } classes[] = {
				{ "Normal", ControlTargetClass::Normal, 1.f },
				{ "Elite", ControlTargetClass::Elite, 1.f },
				{ "MiniBoss", ControlTargetClass::MiniBoss, 1.2f },
				{ "Boss", ControlTargetClass::Boss, 1.2f }
			};
			for (const auto& item : classes)
			{
				const auto target = SpawnEnemy(harness, item.value);
				harness.report.CheckValue(std::string{ "TargetIsBossClass on " } + item.name, item.expected, Ratio(harness, *target, bossClass), item.name);
			}

			const List<ModuleDamageRule> statusKinds = ParsedRules(LoadRuleFixture(harness, "outgoing-status-kinds", Json::parse(R"([{"condition":"TargetStatusKindsAtLeast","conditionValue":2,"damageMultiplier":1.2}])")));
			const auto statused = SpawnEnemy(harness);
			harness.report.CheckValue("TargetStatusKindsAtLeast 2 does not apply with no status", 1.f, Ratio(harness, *statused, statusKinds), "no status");
			ApplyStatus(*statused, harness.ship.get(), DamageTypeSchema::Thermal);
			harness.report.Check(Stacks(*statused, DamageStatusEffectIds::IgniteEffectId) > 0, "Thermal hit leaves an Ignite status on the target");
			harness.report.CheckValue("TargetStatusKindsAtLeast 2 does not apply with one status kind", 1.f, Ratio(harness, *statused, statusKinds), "one status kind");
			ApplyStatus(*statused, harness.ship.get(), DamageTypeSchema::Electric);
			harness.report.Check(Stacks(*statused, DamageStatusEffectIds::ElectricEffectId) > 0, "Electric hit leaves an Electric status on the target");
			harness.report.CheckValue("TargetStatusKindsAtLeast 2 applies with two distinct status kinds", 1.2f, Ratio(harness, *statused, statusKinds), "two status kinds");

			const List<ModuleDamageRule> withoutStatus = ParsedRules(LoadRuleFixture(harness, "outgoing-without-status", Json::parse(R"([{"condition":"TargetWithoutStatus","damageMultiplier":1.2}])")));
			const auto clean = SpawnEnemy(harness);
			harness.report.CheckValue("Regression: TargetWithoutStatus applies to a clean target", 1.2f, Ratio(harness, *clean, withoutStatus), "clean target");
			ApplyStatus(*clean, harness.ship.get(), DamageTypeSchema::Thermal);
			harness.report.CheckValue("Regression: TargetWithoutStatus stops once the target is statused", 1.f, Ratio(harness, *clean, withoutStatus), "statused target");
		}

		void IncomingWindowChecks(Harness& harness)
		{
			const auto enemy = SpawnEnemy(harness);
			const List<ModuleDamageRule> window = ParsedRules(LoadRuleFixture(harness, "incoming-window",
				Json::parse(R"([{"incoming":true,"condition":"Always","damageMultiplier":0.85,"activeWhileTrigger":"AbilityActivated","activeWhileEventTags":["Ability.Movement"],"activeDurationSeconds":3}])")));

			SetRules(harness, {});
			const float base = IncomingDealt(harness, *enemy);
			harness.report.Check(base > 0.f, "Baseline incoming hit deals real health damage", Json{ { "base", base } });
			SetRules(harness, window);

			const auto ratio = [&]() { return IncomingDealt(harness, *enemy) / base; };
			harness.report.CheckValue("Incoming window is closed before any ability event", 1.f, ratio(), "no event yet");
			ActivateMovement(harness);
			harness.report.CheckValue("Movement activation opens the incoming x0.85 window", 0.85f, ratio(), "t=0");
			Tick(harness, 1.5f);
			harness.report.CheckValue("Incoming window is still open at 1.5s", 0.85f, ratio(), "t=1.5");
			Tick(harness, 1.6f);
			harness.report.CheckValue("Incoming window closes after 3s", 1.f, ratio(), "t=3.1");

			ActivateMovement(harness);
			Tick(harness, 2.5f);
			ActivateMovement(harness);
			harness.report.CheckValue("Re-triggering refreshes the incoming window", 0.85f, ratio(), "refresh at t=2.5");
			Tick(harness, 2.5f);
			harness.report.CheckValue("Refreshed incoming window is still open 2.5s after the refresh", 0.85f, ratio(), "refresh at t=2.5, observed t=5.0");
			Tick(harness, 0.6f);
			harness.report.CheckValue("Refreshed incoming window expires 3s after the refresh", 1.f, ratio(), "refresh at t=2.5, observed t=5.6");

			ActivateMovement(harness);
			ActivateMovement(harness);
			Tick(harness, 3.1f);
			harness.report.CheckValue("Repeated triggers do not stack the incoming window", 1.f, ratio(), "two triggers, t=3.1");
		}
	}

	void RunChecks(Report& report, Application& application, const std::filesystem::path& fixtureRoot)
	{
		Harness harness{ report, application, fixtureRoot };
		LoaderChecks(harness);
		OutgoingChecks(harness);
		IncomingWindowChecks(harness);
	}
}

int main(int argc, char** argv)
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	using namespace rare_damage_conditions_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare damage conditions content registration failed" };
		std::string title = "Rare Damage Conditions E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		RunChecks(report, application, std::filesystem::path{ argv[1] }.parent_path() / "rare-damage-fixtures");
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareDamageConditionsE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
