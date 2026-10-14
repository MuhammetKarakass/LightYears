#include "UncommonModuleP4E2E.h"

#include "gameplay/ShieldComponent.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/control/ControlResponse.h"
#include "gameplay/module/ModuleTriggerRuntime.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "gameplay/tags/GameplayTags.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace rare_owner_burst_e2e
{
	using namespace uncommon_p4_e2e;
	using Json = common_module_e2e::Json;

	namespace
	{
		constexpr const char* kBurstId = "Module.Defense.RareOwnerBurstFixture.Rare";
		constexpr const char* kLevelsId = "Module.Defense.RareOwnerBurstLevelsFixture.Rare";
		constexpr const char* kPushId = "Module.Defense.RareOwnerBurstPushFixture.Rare";
		constexpr const char* kStunId = "Module.Defense.RareOwnerBurstStunFixture.Rare";
		constexpr const char* kEchoId = "Module.Defense.RareOwnerBurstEchoFixture.Rare";
		constexpr const char* kProbeId = "Module.Defense.RareOwnerBurstProbeFixture.Rare";
		constexpr const char* kRejectId = "Module.Defense.RareOwnerBurstRejectFixture.Rare";

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

		// A one-cell Rare Defense module. The variant fields (ownerBurstRules, triggerBuffs, ...) are merged in.
		Json FixtureModule(const char* id, const char* displayName, const Json& variantFields)
		{
			Json variant{ { "shape", Json::array({ Json::array({ 0, 0 }) }) } };
			for (const auto& field : variantFields.items()) variant[field.key()] = field.value();
			return Json{
				{ "id", id }, { "displayName", displayName }, { "rarity", "Rare" }, { "levelCount", 3 },
				{ "family", "Ability.Defense" }, { "sizeLadder", Json::array({ 1 }) },
				{ "sizeVariants", Json::array({ variant }) }
			};
		}

		// One ShieldBroken burst rule with the fixture defaults (radius 250, cooldown 20, Energy). extra overrides or adds fields.
		Json Burst(const Json& fraction, const Json& extra = Json::object())
		{
			Json rule{ { "trigger", "ShieldBroken" }, { "radius", 250.0 }, { "damageFractionOfMaxHealth", fraction },
				{ "damageType", "Damage.Type.Energy" }, { "cooldownSeconds", 20.0 } };
			for (const auto& field : extra.items()) rule[field.key()] = field.value();
			return rule;
		}

		// A probe that only reports KillConfirmed: its movement buff is active exactly while such an event was raised.
		Json KillProbe()
		{
			return Json::array({ Json{ { "trigger", "KillConfirmed" }, { "durationSeconds", 5.0 }, { "movementSpeedMultiplier", 1.2 }, { "modifiers", Json::array() } } });
		}

		// Shipped modules plus the test-only burst fixtures.
		Json FixtureSet(const Json& shipped)
		{
			Json modules = shipped;
			modules["modules"].push_back(FixtureModule(kBurstId, "Rare owner burst fixture", Json{ { "ownerBurstRules", Json::array({ Burst(0.10) }) } }));
			modules["modules"].push_back(FixtureModule(kLevelsId, "Rare owner burst levels fixture", Json{ { "ownerBurstRules", Json::array({ Burst(Json::array({ 0.10, 0.16, 0.22 })) }) } }));
			modules["modules"].push_back(FixtureModule(kPushId, "Rare owner burst push fixture", Json{ { "ownerBurstRules", Json::array({ Burst(0.10, Json{ { "pushImpulse", 300.0 } }) }) } }));
			modules["modules"].push_back(FixtureModule(kStunId, "Rare owner burst stun fixture", Json{ { "ownerBurstRules", Json::array({ Burst(0.10, Json{ { "pushImpulse", 300.0 }, { "stunSeconds", 1.0 } }) }) } }));
			modules["modules"].push_back(FixtureModule(kEchoId, "Rare owner burst echo fixture", Json{ { "ownerBurstRules", Json::array({ Burst(0.10, Json{ { "echoDelaySeconds", 1.5 }, { "echoFraction", 0.5 } }) }) } }));
			modules["modules"].push_back(FixtureModule(kProbeId, "Rare owner burst kill probe", Json{ { "triggerBuffs", KillProbe() } }));
			return modules;
		}

		void SetShield(SpaceShip& ship, float value)
		{
			ShieldComponent& shield = ship.GetShieldComponent();
			shield.ChangeShield(-shield.GetShield());
			if (value > 0.f) shield.ChangeShield(value);
		}

		float Distance(SpaceShip& from, SpaceShip& to)
		{
			const sf::Vector2f delta = to.GetActorLocation() - from.GetActorLocation();
			return std::sqrt(delta.x * delta.x + delta.y * delta.y);
		}

		void Advance(Fixture& f, float seconds)
		{
			float remaining = seconds;
			while (remaining > 1e-4f)
			{
				const float step = std::min(0.1f, remaining);
				f.level->TickInternal(step);
				remaining -= step;
			}
		}

		// Breaks the owner's shield with one hit from source and checks that the shield actually reached zero.
		void Break(Report& report, Fixture& f, Actor* source, const DamagePayload& payload = Plain())
		{
			SetShield(*f.ship, 10.f);
			Hit(*f.ship, source, 30.f, payload);
			report.Check(f.ship->GetShieldComponent().GetShield() <= 0.f, "owner shield reaches zero in the break",
				Json{ { "shield", f.ship->GetShieldComponent().GetShield() } });
		}

		// Fresh ship with the burst fixture's MaxHealth set. Regenerate keeps the owner alive through the hits.
		void SetOwnerMaxHealth(Fixture& f, float maxHealth)
		{
			f.ship->GetHealthComponent().SetMaxHealth(maxHealth);
			f.ship->GetHealthComponent().Regenerate(10000.f);
		}

		// The shared Enemy() fixture empties the shield but leaves it regenerating, and the 0.1 s setup ticks refill it.
		// That refilled shield absorbs part of the burst, so every burst target gets zero shield capacity right after spawn.
		auto ZeroShieldEnemy(Fixture& f, float distance)
		{
			auto enemy = Enemy(f, distance);
			enemy->GetShieldComponent().SetMaxShield(0.f);
			return enemy;
		}

		void RadiusChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.Install(kBurstId);
			SetOwnerMaxHealth(f, 400.f);
			auto inside = ZeroShieldEnemy(f, 100.f);
			auto edge = ZeroShieldEnemy(f, 240.f);
			auto outside = ZeroShieldEnemy(f, 400.f);
			Advance(f, 0.1f);
			const float insideBefore = inside->GetHealthComponent().GetHealth();
			const float edgeBefore = edge->GetHealthComponent().GetHealth();
			const float outsideBefore = outside->GetHealthComponent().GetHealth();
			Break(report, f, outside.get());
			report.CheckValue("enemy inside 250 takes 10% of owner MaxHealth", 40.f, insideBefore - inside->GetHealthComponent().GetHealth(), "owner burst radius");
			report.CheckValue("enemy at 240 (inside the radius) takes the burst", 40.f, edgeBefore - edge->GetHealthComponent().GetHealth(), "owner burst radius");
			report.CheckValue("enemy at 400 (outside the radius) takes nothing", 0.f, outsideBefore - outside->GetHealthComponent().GetHealth(), "owner burst radius");
		}

		void ScalingChecks(Report& report, Application& application)
		{
			const float maxHealths[] = { 400.f, 800.f };
			for (float maxHealth : maxHealths)
			{
				Fixture f{ report, application };
				f.rules.lookup = FixtureLookup;
				f.Install(kBurstId);
				SetOwnerMaxHealth(f, maxHealth);
				auto enemy = ZeroShieldEnemy(f, 100.f);
				auto attacker = ZeroShieldEnemy(f, 400.f);
				Advance(f, 0.1f);
				const float before = enemy->GetHealthComponent().GetHealth();
				Break(report, f, attacker.get());
				report.CheckValue("burst damage scales with the owner MaxHealth", 0.10f * maxHealth,
					before - enemy->GetHealthComponent().GetHealth(), "owner MaxHealth fraction");
			}
		}

		void CooldownChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.Install(kBurstId);
			SetOwnerMaxHealth(f, 400.f);
			auto enemy = ZeroShieldEnemy(f, 100.f);
			auto attacker = ZeroShieldEnemy(f, 400.f);
			Advance(f, 0.1f);
			const float start = enemy->GetHealthComponent().GetHealth();
			Break(report, f, attacker.get());
			report.CheckValue("first break bursts", 40.f, start - enemy->GetHealthComponent().GetHealth(), "cooldown start");
			Advance(f, 1.f);
			const float afterFirst = enemy->GetHealthComponent().GetHealth();
			Break(report, f, attacker.get());
			report.CheckValue("second break inside the 20 s cooldown does not burst", 0.f, afterFirst - enemy->GetHealthComponent().GetHealth(), "cooldown");
			Advance(f, 19.5f);
			const float beforeThird = enemy->GetHealthComponent().GetHealth();
			Break(report, f, attacker.get());
			report.CheckValue("break after the 20 s cooldown bursts again", 40.f, beforeThird - enemy->GetHealthComponent().GetHealth(), "cooldown");
		}

		void PushChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.Install(kPushId);
			SetOwnerMaxHealth(f, 400.f);
			auto enemy = ZeroShieldEnemy(f, 100.f);
			auto attacker = ZeroShieldEnemy(f, 400.f);
			Advance(f, 0.1f);
			const float before = Distance(*f.ship, *enemy);
			Break(report, f, attacker.get());
			Advance(f, 0.5f);
			const float after = Distance(*f.ship, *enemy);
			report.Check(after > before + 20.f, "owner burst pushes the enemy away from the owner",
				Json{ { "before", before }, { "after", after } });
		}

		void StunChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.Install(kStunId);
			SetOwnerMaxHealth(f, 400.f);
			auto normal = ZeroShieldEnemy(f, 100.f);
			auto elite = ZeroShieldEnemy(f, 120.f);
			auto miniBoss = ZeroShieldEnemy(f, 140.f);
			auto boss = ZeroShieldEnemy(f, 160.f);
			elite->SetControlTargetClass(ControlTargetClass::Elite);
			miniBoss->SetControlTargetClass(ControlTargetClass::MiniBoss);
			boss->SetControlTargetClass(ControlTargetClass::Boss);
			auto attacker = ZeroShieldEnemy(f, 400.f);
			Advance(f, 0.1f);
			Break(report, f, attacker.get());
			Advance(f, 0.1f);
			const auto stunned = GameplayTags::State::Effect::Control::Stunned;
			report.Check(normal->GetAbilitySystemComponent().HasOwnedTag(stunned), "Normal enemy is stunned by the burst");
			report.Check(!elite->GetAbilitySystemComponent().HasOwnedTag(stunned), "Elite enemy is immune to the burst stun");
			report.Check(!miniBoss->GetAbilitySystemComponent().HasOwnedTag(stunned), "MiniBoss enemy is immune to the burst stun");
			report.Check(!boss->GetAbilitySystemComponent().HasOwnedTag(stunned), "Boss enemy is immune to the burst stun");
			Advance(f, 0.5f);
			report.Check(normal->GetAbilitySystemComponent().HasOwnedTag(stunned), "stun lasts through its duration");
			Advance(f, 1.0f);
			report.Check(!normal->GetAbilitySystemComponent().HasOwnedTag(stunned), "stun ends when its duration is over");
		}

		void EchoChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.Install(kEchoId);
			SetOwnerMaxHealth(f, 400.f);
			auto enemy = ZeroShieldEnemy(f, 100.f);
			auto attacker = ZeroShieldEnemy(f, 400.f);
			Advance(f, 0.1f);
			const float start = enemy->GetHealthComponent().GetHealth();
			Break(report, f, attacker.get());
			report.CheckValue("first wave lands at once", 40.f, start - enemy->GetHealthComponent().GetHealth(), "echo first wave");
			report.CheckValue("one echo is scheduled", 1.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstEchoCount()), "echo schedule");
			Advance(f, 1.0f);
			report.CheckValue("echo waits for its delay", 40.f, start - enemy->GetHealthComponent().GetHealth(), "echo delay");
			Advance(f, 0.6f);
			report.CheckValue("echo wave deals half of the first wave", 60.f, start - enemy->GetHealthComponent().GetHealth(), "echo fraction");
			report.CheckValue("echo leaves no pending work", 0.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstEchoCount()), "echo cleanup");
		}

		void ModuleCausedBreakChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.Install(kBurstId);
			SetOwnerMaxHealth(f, 400.f);
			auto enemy = ZeroShieldEnemy(f, 100.f);
			auto attacker = ZeroShieldEnemy(f, 400.f);
			Advance(f, 0.1f);
			const float start = enemy->GetHealthComponent().GetHealth();
			DamagePayload moduleHit = MakeModuleGeneratedDamagePayload();
			Break(report, f, attacker.get(), moduleHit);
			report.CheckValue("a module-caused shield break does not burst", 0.f, start - enemy->GetHealthComponent().GetHealth(), "module-caused break");
			Break(report, f, attacker.get());
			report.CheckValue("the ordinary break after it still bursts (no cooldown was spent)", 40.f, start - enemy->GetHealthComponent().GetHealth(), "module-caused break");
		}

		void OtherModuleChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.Install(kBurstId);
			f.Install(kProbeId);
			SetOwnerMaxHealth(f, 400.f);
			auto nearEnemy = ZeroShieldEnemy(f, 100.f);
			auto attacker = ZeroShieldEnemy(f, 400.f);
			Advance(f, 0.1f);
			// Leave the enemy with 10 health so the burst is the killing blow.
			Hit(*nearEnemy, nullptr, 990.f, Plain());
			ModuleTriggerRuntime& triggers = f.ship->GetModuleTriggerRuntime();
			const std::size_t before = triggers.GetActiveModifierHandleCount();
			Break(report, f, attacker.get());
			report.Check(nearEnemy->GetHealthComponent().GetHealth() <= 0.f, "owner burst kills the nearly dead enemy");
			report.Check(triggers.GetActiveModifierHandleCount() == before, "a burst kill does not trigger another module's KillConfirmed buff",
				Json{ { "before", before }, { "after", triggers.GetActiveModifierHandleCount() } });
			// Positive control: an ordinary kill by the owner does activate the probe.
			f.Kill(f.ship.get(), false);
			report.Check(triggers.GetActiveModifierHandleCount() > before, "positive control: an ordinary kill activates the KillConfirmed probe");
		}

		void RemovalChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			const std::uint32_t instance = f.Install(kBurstId);
			SetOwnerMaxHealth(f, 400.f);
			report.CheckValue("installed owner burst rule is tracked", 1.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstRuleCount()), "removal");
			auto enemy = ZeroShieldEnemy(f, 100.f);
			auto attacker = ZeroShieldEnemy(f, 400.f);
			Advance(f, 0.1f);
			f.Remove(instance);
			report.CheckValue("removed module leaves no owner burst rule", 0.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstRuleCount()), "removal");
			const float start = enemy->GetHealthComponent().GetHealth();
			Break(report, f, attacker.get());
			report.CheckValue("removed module does not burst", 0.f, start - enemy->GetHealthComponent().GetHealth(), "removal");
		}

		void LevelChecks(Report& report, Application& application)
		{
			const float expected[] = { 0.10f, 0.16f, 0.22f };
			for (int level = 1; level <= 3; ++level)
			{
				Fixture f{ report, application };
				f.rules.lookup = FixtureLookup;
				f.Install(kLevelsId, 0, level);
				SetOwnerMaxHealth(f, 400.f);
				auto enemy = ZeroShieldEnemy(f, 100.f);
				auto attacker = ZeroShieldEnemy(f, 400.f);
				Advance(f, 0.1f);
				const float start = enemy->GetHealthComponent().GetHealth();
				Break(report, f, attacker.get());
				report.CheckValue("level " + std::to_string(level) + " burst uses its own fraction", expected[level - 1] * 400.f,
					start - enemy->GetHealthComponent().GetHealth(), "level table 10/16/22%");
			}
		}

		// Each entry is one ownerBurstRules array; the first fields are a valid rule that the case then breaks.
		void LoaderChecks(Report& report, const std::filesystem::path& root, const Json& shipped)
		{
			const auto withRules = [&shipped](const Json& rules)
			{
				Json modules = shipped;
				modules["modules"].push_back(FixtureModule(kRejectId, "Rare owner burst reject fixture", Json{ { "ownerBurstRules", rules } }));
				return modules;
			};
			const auto reject = [&](const char* name, const Json& rules)
			{
				const auto path = WriteJson(root, std::string{ "reject-" } + name + ".json", withRules(rules));
				const auto loaded = content::ModuleLoader::LoadFromFile(path);
				report.Check(!loaded.Succeeded(), std::string{ "ownerBurstRules loader rejects " } + name, loaded.error);
			};
			reject("wrong-trigger", Json::array({ Burst(0.10, Json{ { "trigger", "KillConfirmed" } }) }));
			reject("unknown-trigger", Json::array({ Burst(0.10, Json{ { "trigger", "Nope" } }) }));
			reject("missing-trigger", Json::array({ Burst(0.10, Json{ { "trigger", nullptr } }) }));
			reject("missing-radius", Json::array({ Json{ { "trigger", "ShieldBroken" }, { "damageFractionOfMaxHealth", 0.1 }, { "damageType", "Damage.Type.Energy" }, { "cooldownSeconds", 20.0 } } }));
			reject("zero-radius", Json::array({ Burst(0.10, Json{ { "radius", 0.0 } }) }));
			reject("zero-fraction", Json::array({ Burst(0.0) }));
			reject("fraction-table-with-zero", Json::array({ Burst(Json::array({ 0.10, 0.0, 0.22 })) }));
			reject("short-fraction-table", Json::array({ Burst(Json::array({ 0.10, 0.16 })) }));
			reject("missing-damage-type", Json::array({ Json{ { "trigger", "ShieldBroken" }, { "radius", 250.0 }, { "damageFractionOfMaxHealth", 0.1 }, { "cooldownSeconds", 20.0 } } }));
			reject("missing-cooldown", Json::array({ Json{ { "trigger", "ShieldBroken" }, { "radius", 250.0 }, { "damageFractionOfMaxHealth", 0.1 }, { "damageType", "Damage.Type.Energy" } } }));
			reject("zero-cooldown", Json::array({ Burst(0.10, Json{ { "cooldownSeconds", 0.0 } }) }));
			reject("unknown-field", Json::array({ Burst(0.10, Json{ { "shield", 3 } }) }));
			reject("unknown-class", Json::array({ Burst(0.10, Json{ { "stunImmuneClasses", Json::array({ "Titan" }) } }) }));
			reject("immune-classes-not-array", Json::array({ Burst(0.10, Json{ { "stunImmuneClasses", "Boss" } }) }));
			reject("negative-push", Json::array({ Burst(0.10, Json{ { "pushImpulse", -1.0 } }) }));
			reject("echo-fraction-without-delay", Json::array({ Burst(0.10, Json{ { "echoFraction", 0.5 } }) }));
			reject("echo-delay-without-fraction", Json::array({ Burst(0.10, Json{ { "echoDelaySeconds", 1.5 } }) }));
			reject("echo-fraction-above-one", Json::array({ Burst(0.10, Json{ { "echoDelaySeconds", 1.5 }, { "echoFraction", 1.5 } }) }));
			reject("rules-not-array", Json::object());
			reject("entry-not-object", Json::array({ 1.0 }));

			const auto accepted = [&](const char* name, const Json& rules)
			{
				const auto path = WriteJson(root, std::string{ "accept-" } + name + ".json", withRules(rules));
				const auto loaded = content::ModuleLoader::LoadFromFile(path);
				report.Check(loaded.Succeeded(), std::string{ "ownerBurstRules loader accepts " } + name, loaded.error);
				return loaded;
			};
			const auto defaults = accepted("default-immunity", Json::array({ Burst(0.10, Json{ { "pushImpulse", 300.0 }, { "stunSeconds", 1.0 } }) }));
			if (defaults.Succeeded())
			{
				const auto found = std::find_if(defaults.definitions.begin(), defaults.definitions.end(),
					[](const ModuleDefinition& definition) { return definition.moduleId == sas::ContentId{ kRejectId }; });
				report.Check(found != defaults.definitions.end() && found->sizeVariants.size() == 1 && found->sizeVariants[0].ownerBurstRules.size() == 1 &&
					found->sizeVariants[0].ownerBurstRules[0].stunImmuneClasses.size() == 3,
					"stunImmuneClasses defaults to Elite, MiniBoss and Boss");
			}
			accepted("explicit-empty-immunity", Json::array({ Burst(0.10, Json{ { "stunImmuneClasses", Json::array() }, { "stunSeconds", 1.0 }, { "pushImpulse", 300.0 } }) }));
			accepted("per-level-fraction", Json::array({ Burst(Json::array({ 0.10, 0.16, 0.22 }), Json{ { "echoDelaySeconds", 1.5 }, { "echoFraction", 0.5 } }) }));

			const auto shippedLoaded = content::ModuleLoader::LoadFromFile(
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json");
			report.Check(shippedLoaded.Succeeded(), "shipped modules still load through the ownerBurstRules parser", shippedLoaded.error);
		}
	}
}

int main(int argc, char** argv)
{
	using namespace rare_owner_burst_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare owner burst content registration failed" };
		std::string title = "Rare Owner Burst E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);

		const std::filesystem::path root = std::filesystem::path{ argv[1] }.parent_path() / "rare-owner-burst-fixtures";
		Json shipped;
		{
			std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json" };
			input >> shipped;
		}
		const auto fixturePath = WriteJson(root, "rare-owner-burst-modules.json", FixtureSet(shipped));
		const auto loaded = content::ModuleLoader::LoadFromFile(fixturePath);
		report.Check(loaded.Succeeded(), "rare owner burst fixture modules load through the real ModuleLoader", loaded.error);
		gFixtureDefinitions = loaded.definitions;

		RadiusChecks(report, application);
		ScalingChecks(report, application);
		CooldownChecks(report, application);
		PushChecks(report, application);
		StunChecks(report, application);
		EchoChecks(report, application);
		ModuleCausedBreakChecks(report, application);
		OtherModuleChecks(report, application);
		RemovalChecks(report, application);
		LevelChecks(report, application);
		LoaderChecks(report, root, shipped);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareOwnerBurstE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
