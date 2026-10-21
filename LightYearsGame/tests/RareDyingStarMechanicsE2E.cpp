#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"

#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/ShieldComponent.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"
#include "gameplay/control/ControlResponse.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "gameplay/tags/GameplayTags.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace rare_dying_star_e2e
{
	using namespace uncommon_p4_e2e;
	using Json = common_module_e2e::Json;

	namespace
	{
		constexpr const char* kDyingId = "Module.Offense.RareDyingStarFixture.Rare";
		constexpr const char* kShatterId = "Module.Defense.RareShatterFixture.Rare";
		constexpr const char* kCoreId = "Module.SideFeature.Fixture.CoreCollapse";
		constexpr const char* kRedId = "Module.SideFeature.Fixture.RedGiant";
		constexpr const char* kStaggerId = "Module.SideFeature.Fixture.Stagger";
		constexpr const char* kAfterId = "Module.SideFeature.Fixture.Aftershock";
		constexpr const char* kRejectId = "Module.Offense.RareDyingStarReject.Rare";

		// Test-only definitions parsed from fixture JSON. They never enter shipped content.
		vega::List<vega::ModuleDefinition> gFixtureDefinitions;
		vega::List<vega::ModuleSideFeatureDefinition> gFixtureSideFeatures;

		const ModuleDefinition* FixtureLookup(const sas::ContentId& id)
		{
			for (const ModuleDefinition& definition : gFixtureDefinitions)
				if (definition.moduleId == id) return &definition;
			return content::ModuleContentCatalog::FindById(id);
		}

		const ModuleSideFeatureDefinition* FixtureSideLookup(const sas::ContentId& id)
		{
			for (const ModuleSideFeatureDefinition& definition : gFixtureSideFeatures)
				if (definition.sideFeatureId == id) return &definition;
			return content::ModuleSideFeatureCatalog::FindById(id);
		}

		std::filesystem::path WriteJson(const std::filesystem::path& root, const std::string& name, const Json& value)
		{
			std::filesystem::create_directories(root);
			const std::filesystem::path path = root / name;
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			output << value.dump(2) << '\n';
			return path;
		}

		// Dying Star: a charge-gated Thermal burst. Ten owner kills within 300 units fire it for 60 flat damage at level 1,
		// +15% of that base per level, with 4 Ignite stacks per hit. Radius 350 is the design suggestion; the damage base is a test fixture value.
		Json DyingRule()
		{
			return Json{ { "trigger", "KillConfirmed" }, { "radius", 350.0 }, { "damage", Json::array({ 60.0, 69.0, 78.0 }) },
				{ "damageType", "Damage.Type.Thermal" }, { "igniteStacks", 4 }, { "chargeKills", 10 }, { "chargeRadius", 300.0 } };
		}

		// Shield-break burst with a push: 10% of MaxHealth in 250 units, 20 sec cooldown, 300 push.
		Json ShatterRule()
		{
			return Json{ { "trigger", "ShieldBroken" }, { "radius", 250.0 }, { "damageFractionOfMaxHealth", 0.10 },
				{ "damageType", "Damage.Type.Energy" }, { "cooldownSeconds", 20.0 }, { "pushImpulse", 300.0 } };
		}

		// A module with a one-cell and a three-cell variant, so side features can attach to the largest one.
		Json FixtureModule(const char* id, const char* displayName, const char* family, const Json& rule, const Json& sideFeatures)
		{
			return Json{
				{ "id", id }, { "displayName", displayName }, { "rarity", "Rare" }, { "levelCount", 3 },
				{ "family", family }, { "sizeLadder", Json::array({ 1, 3 }) },
				{ "sizeVariants", Json::array({
					Json{ { "shape", Json::array({ Json::array({ 0, 0 }) }) }, { "ownerBurstRules", Json::array({ rule }) } },
					Json{ { "shape", Json::array({ Json::array({ 0, 0 }), Json::array({ 1, 0 }), Json::array({ 2, 0 }) }) }, { "ownerBurstRules", Json::array({ rule }) } } }) },
				{ "sideFeatures", sideFeatures }
			};
		}

		// Test-only side features: each one is a modifier on the main burst rule, and none of them has its own rule.
		Json SideFeature(const char* id, const Json& modifiers)
		{
			return Json{ { "id", id }, { "displayName", id }, { "description", "fixture" }, { "levelCount", 3 },
				{ "rules", Json{ { "ownerBurstModifiers", modifiers } } } };
		}

		Json SideFile(const Json& features)
		{
			return Json{ { "schemaVersion", 1 }, { "sharedPools", Json::array() }, { "sideFeatures", features } };
		}

		Json FixtureSideFeatures()
		{
			return SideFile(Json::array({
				SideFeature(kCoreId, Json::array({ Json{ { "pullImpulse", 400.0 }, { "pullLeadSeconds", 1.0 } } })),
				SideFeature(kRedId, Json::array({ Json{ { "chargeRadius", 450.0 } } })),
				SideFeature(kStaggerId, Json::array({ Json{ { "stunSeconds", 1.0 } } })),
				SideFeature(kAfterId, Json::array({ Json{ { "echoDelaySeconds", 1.5 }, { "echoFraction", 0.0 }, { "echoPushFraction", 0.5 } } }))
			}));
		}

		Json FixtureSet(const Json& shipped)
		{
			Json modules = shipped;
			modules["modules"].push_back(FixtureModule(kDyingId, "Rare dying star fixture", "Ability.Offense", DyingRule(),
				Json::array({ kCoreId, kRedId })));
			modules["modules"].push_back(FixtureModule(kShatterId, "Rare shatter shock fixture", "Ability.Defense", ShatterRule(),
				Json::array({ kStaggerId, kAfterId })));
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

		void SetOwnerMaxHealth(Fixture& f, float maxHealth)
		{
			f.ship->GetHealthComponent().SetMaxHealth(maxHealth);
			f.ship->GetHealthComponent().Regenerate(10000.f);
		}

		// The shared Enemy() fixture refills its shield over setup ticks; zero capacity keeps damage on health.
		auto ZeroShieldEnemy(Fixture& f, float distance)
		{
			auto enemy = Enemy(f, distance);
			enemy->GetShieldComponent().SetMaxShield(0.f);
			return enemy;
		}

		// One enemy spawned at the given distance and killed by the owner's own hit (or by the given payload).
		void KillAt(Fixture& f, float distance, const DamagePayload& payload = Plain())
		{
			auto enemy = ZeroShieldEnemy(f, distance);
			Hit(*enemy, f.ship.get(), 100000.f, payload);
		}

		void DyingChargeChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.rules.sideFeatureLookup = FixtureSideLookup;
			f.Install(kDyingId, 0, 1);
			auto bystander = ZeroShieldEnemy(f, 340.f);
			Advance(f, 0.1f);
			const float start = bystander->GetHealthComponent().GetHealth();
			for (int kill = 0; kill < 9; ++kill) KillAt(f, 100.f);
			report.CheckValue("nine charging kills do not burst", 0.f, start - bystander->GetHealthComponent().GetHealth(), "charge count below 10");
			KillAt(f, 100.f);
			report.CheckValue("the tenth charging kill bursts for 60 Thermal", 60.f, start - bystander->GetHealthComponent().GetHealth(), "charge threshold and flat damage");
			const auto* ignite = bystander->GetAbilitySystemComponent().FindGameplayEffectById(DamageStatusEffectIds::IgniteEffectId);
			report.CheckValue("each burst hit gets 4 Ignite stacks", 4.f, ignite ? static_cast<float>(ignite->stackCount) : 0.f, "igniteStacks");
			report.CheckValue("the burst leaves no pending wave", 0.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstEchoCount()), "no detonation delay");
			const float afterBurst = bystander->GetHealthComponent().GetHealth();
			for (int kill = 0; kill < 9; ++kill) KillAt(f, 100.f);
			report.CheckValue("charges reset: nine kills after the burst do not burst again", 0.f, afterBurst - bystander->GetHealthComponent().GetHealth(), "charge reset");
			KillAt(f, 100.f);
			report.CheckValue("the tenth kill after the reset bursts again for 60", 60.f, afterBurst - bystander->GetHealthComponent().GetHealth(), "charge reset");
		}

		void DyingRadiusChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.Install(kDyingId, 0, 1);
			auto bystander = ZeroShieldEnemy(f, 340.f);
			Advance(f, 0.1f);
			const float start = bystander->GetHealthComponent().GetHealth();
			for (int kill = 0; kill < 10; ++kill) KillAt(f, 310.f);
			report.CheckValue("ten kills at 310 units (outside the 300 charge radius) do not charge", 0.f, start - bystander->GetHealthComponent().GetHealth(), "charge radius");
			for (int kill = 0; kill < 10; ++kill) KillAt(f, 300.f);
			report.CheckValue("ten kills at exactly 300 units charge and burst", 60.f, start - bystander->GetHealthComponent().GetHealth(), "charge radius boundary");
		}

		void DyingSelfKillChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.Install(kDyingId, 0, 1);
			auto nearlyDead = ZeroShieldEnemy(f, 340.f);
			Advance(f, 0.1f);
			Hit(*nearlyDead, nullptr, 990.f, Plain());
			for (int kill = 0; kill < 10; ++kill) KillAt(f, 100.f);
			report.Check(nearlyDead->GetHealthComponent().GetHealth() <= 0.f, "the burst kills a nearly dead enemy");
			auto fresh = ZeroShieldEnemy(f, 340.f);
			const float start = fresh->GetHealthComponent().GetHealth();
			for (int kill = 0; kill < 9; ++kill) KillAt(f, 100.f);
			report.CheckValue("kills caused by the burst do not charge: nine kills after it do not burst", 0.f, start - fresh->GetHealthComponent().GetHealth(), "burst kills are not charges");
			KillAt(f, 100.f);
			report.CheckValue("the tenth owner kill after the burst bursts again", 60.f, start - fresh->GetHealthComponent().GetHealth(), "burst kills are not charges");
		}

		void DyingModuleKillChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.Install(kDyingId, 0, 1);
			auto bystander = ZeroShieldEnemy(f, 340.f);
			Advance(f, 0.1f);
			const float start = bystander->GetHealthComponent().GetHealth();
			const DamagePayload generated = MakeModuleGeneratedDamagePayload();
			for (int kill = 0; kill < 10; ++kill) KillAt(f, 100.f, generated);
			report.CheckValue("ten module-generated kills do not charge", 0.f, start - bystander->GetHealthComponent().GetHealth(), "generated kills excluded");
			for (int kill = 0; kill < 10; ++kill) KillAt(f, 100.f);
			report.CheckValue("ten ordinary kills still charge after generated kills", 60.f, start - bystander->GetHealthComponent().GetHealth(), "generated kills excluded");
		}

		void DyingShieldBreakChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.Install(kDyingId, 0, 1);
			SetOwnerMaxHealth(f, 400.f);
			auto bystander = ZeroShieldEnemy(f, 100.f);
			auto attacker = ZeroShieldEnemy(f, 400.f);
			Advance(f, 0.1f);
			const float start = bystander->GetHealthComponent().GetHealth();
			Break(report, f, attacker.get());
			report.CheckValue("a shield break does not fire the kill-charged rule", 0.f, start - bystander->GetHealthComponent().GetHealth(), "trigger is KillConfirmed only");
		}

		void DyingLevelChecks(Report& report, Application& application)
		{
			const float expected[] = { 60.f, 69.f, 78.f };
			for (int level = 1; level <= 3; ++level)
			{
				Fixture f{ report, application };
				f.rules.lookup = FixtureLookup;
				f.Install(kDyingId, 0, level);
				auto bystander = ZeroShieldEnemy(f, 340.f);
				Advance(f, 0.1f);
				const float start = bystander->GetHealthComponent().GetHealth();
				for (int kill = 0; kill < 10; ++kill) KillAt(f, 100.f);
				report.CheckValue("level " + std::to_string(level) + " burst deals its flat base (+15% per level)", expected[level - 1],
					start - bystander->GetHealthComponent().GetHealth(), "level table 60/69/78");
			}
		}

		void DyingRemovalChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			const std::uint32_t instance = f.Install(kDyingId, 0, 1);
			auto bystander = ZeroShieldEnemy(f, 340.f);
			Advance(f, 0.1f);
			const float start = bystander->GetHealthComponent().GetHealth();
			for (int kill = 0; kill < 9; ++kill) KillAt(f, 100.f);
			f.Remove(instance);
			f.Install(kDyingId, 0, 1);
			for (int kill = 0; kill < 9; ++kill) KillAt(f, 100.f);
			report.CheckValue("a reinstalled module starts with zero charges", 0.f, start - bystander->GetHealthComponent().GetHealth(), "charges are per instance");
			KillAt(f, 100.f);
			report.CheckValue("the tenth charge of the new instance bursts", 60.f, start - bystander->GetHealthComponent().GetHealth(), "charges are per instance");
		}

		void CorePullChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.rules.sideFeatureLookup = FixtureSideLookup;
			f.Install(kDyingId, 1, 1, kCoreId);
			auto normal = ZeroShieldEnemy(f, 250.f);
			auto elite = ZeroShieldEnemy(f, 230.f);
			auto boss = ZeroShieldEnemy(f, 210.f);
			elite->SetControlTargetClass(ControlTargetClass::Elite);
			boss->SetControlTargetClass(ControlTargetClass::Boss);
			Advance(f, 0.1f);
			const float normalStart = normal->GetHealthComponent().GetHealth();
			const float eliteStart = elite->GetHealthComponent().GetHealth();
			const float bossStart = boss->GetHealthComponent().GetHealth();
			const float normalDistance = Distance(*f.ship, *normal);
			const float eliteDistance = Distance(*f.ship, *elite);
			const float bossDistance = Distance(*f.ship, *boss);
			for (int kill = 0; kill < 10; ++kill) KillAt(f, 100.f);
			report.CheckValue("the charged burst waits 1 s: one detonation is pending", 1.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstEchoCount()), "pull lead");
			Advance(f, 0.2f);
			report.Check(Distance(*f.ship, *normal) < normalDistance - 10.f, "Core Collapse pulls a normal enemy toward the owner before the detonation",
				Json{ { "before", normalDistance }, { "after", Distance(*f.ship, *normal) } });
			report.Check(Distance(*f.ship, *elite) < eliteDistance - 10.f, "Core Collapse pulls an elite enemy toward the owner");
			report.Check(std::abs(Distance(*f.ship, *boss) - bossDistance) < 1.f, "Core Collapse does not pull a boss",
				Json{ { "before", bossDistance }, { "after", Distance(*f.ship, *boss) } });
			report.CheckValue("no damage before the 1 s lead has passed", 0.f, normalStart - normal->GetHealthComponent().GetHealth(), "pull lead");
			Advance(f, 0.6f);
			report.CheckValue("still no damage at 0.8 s", 0.f, normalStart - normal->GetHealthComponent().GetHealth(), "pull lead");
			// Read the flat burst on the step it lands. Its Ignite stacks deal 5 DPS each and first tick only after 0.25 s, so a
			// later sample would also contain Ignite damage (the old 1.2 s sample read 61.5 = 60 + 4 stacks x 5 DPS x 0.3 s).
			for (int step = 0; step < 5 && f.ship->GetModuleApplier().GetOwnerBurstEchoCount() > 0; ++step) Advance(f, 0.1f);
			report.CheckValue("the detonation has fired once its lead has run out", 0.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstEchoCount()), "pull lead");
			report.CheckValue("the detonation lands after the lead, 60 on a normal enemy", 60.f, normalStart - normal->GetHealthComponent().GetHealth(), "pull lead then burst");
			report.CheckValue("the detonation hits an elite enemy too", 60.f, eliteStart - elite->GetHealthComponent().GetHealth(), "elite is pulled, not immune to damage");
			report.CheckValue("the detonation still damages a boss; only the pull skips it", 60.f, bossStart - boss->GetHealthComponent().GetHealth(), "boss pull immunity only");
			report.CheckValue("no detonation left pending", 0.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstEchoCount()), "pull cleanup");
		}

		void RedGiantChecks(Report& report, Application& application)
		{
			{
				Fixture f{ report, application };
				f.rules.lookup = FixtureLookup;
				f.rules.sideFeatureLookup = FixtureSideLookup;
				f.Install(kDyingId, 1, 1, kRedId);
				auto bystander = ZeroShieldEnemy(f, 340.f);
				Advance(f, 0.1f);
				const float start = bystander->GetHealthComponent().GetHealth();
				for (int kill = 0; kill < 10; ++kill) KillAt(f, 400.f);
				report.CheckValue("Red Giant: ten kills at 400 units charge (radius 450)", 60.f, start - bystander->GetHealthComponent().GetHealth(), "Red Giant chargeRadius");
			}
			{
				Fixture f{ report, application };
				f.rules.lookup = FixtureLookup;
				f.rules.sideFeatureLookup = FixtureSideLookup;
				f.Install(kDyingId, 0, 1);
				auto bystander = ZeroShieldEnemy(f, 340.f);
				Advance(f, 0.1f);
				const float start = bystander->GetHealthComponent().GetHealth();
				for (int kill = 0; kill < 10; ++kill) KillAt(f, 400.f);
				report.CheckValue("without Red Giant, ten kills at 400 units do not charge (radius 300)", 0.f, start - bystander->GetHealthComponent().GetHealth(), "base chargeRadius");
			}
		}

		void StaggerChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.rules.sideFeatureLookup = FixtureSideLookup;
			f.Install(kShatterId, 1, 1, kStaggerId);
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
			report.CheckValue("the Stagger side feature adds no burst rule", 1.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstRuleCount()), "modifier, not a rule");
			Break(report, f, attacker.get());
			Advance(f, 0.1f);
			const auto stunned = GameplayTags::State::Effect::Control::Stunned;
			report.Check(normal->GetAbilitySystemComponent().HasOwnedTag(stunned), "Stagger stuns a normal enemy");
			report.Check(!elite->GetAbilitySystemComponent().HasOwnedTag(stunned), "Stagger leaves an elite enemy unstunned");
			report.Check(!miniBoss->GetAbilitySystemComponent().HasOwnedTag(stunned), "Stagger leaves a miniboss unstunned");
			report.Check(!boss->GetAbilitySystemComponent().HasOwnedTag(stunned), "Stagger leaves a boss unstunned");
			Advance(f, 1.0f);
			report.Check(!normal->GetAbilitySystemComponent().HasOwnedTag(stunned), "the Stagger stun ends after one second");
		}

		void AftershockChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.rules.sideFeatureLookup = FixtureSideLookup;
			f.Install(kShatterId, 1, 1, kAfterId);
			SetOwnerMaxHealth(f, 400.f);
			auto enemy = ZeroShieldEnemy(f, 100.f);
			auto attacker = ZeroShieldEnemy(f, 400.f);
			Advance(f, 0.1f);
			const float start = enemy->GetHealthComponent().GetHealth();
			Break(report, f, attacker.get());
			report.CheckValue("the first wave deals 10% of MaxHealth", 40.f, start - enemy->GetHealthComponent().GetHealth(), "main rule");
			report.CheckValue("Aftershock adds no burst rule", 1.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstRuleCount()), "modifier, not a rule");
			report.CheckValue("one Aftershock wave is scheduled", 1.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstEchoCount()), "echo schedule");
			Advance(f, 1.0f);
			report.CheckValue("the Aftershock waits 1.5 s", 1.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstEchoCount()), "echo delay");
			Advance(f, 0.6f);
			report.CheckValue("the Aftershock wave has fired", 0.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstEchoCount()), "echo fired");
			report.CheckValue("the Aftershock wave deals no damage (only the first 40)", 40.f, start - enemy->GetHealthComponent().GetHealth(), "damage-less side entry");
		}

		// Each entry is one ownerBurstRules array on a one-cell fixture module; the case then loads and expects a failure.
		void LoaderChecks(Report& report, const std::filesystem::path& root, const Json& shipped)
		{
			const auto withRules = [&shipped](const Json& rules)
			{
				Json modules = shipped;
				modules["modules"].push_back(FixtureModule(kRejectId, "Rare dying star reject fixture", "Ability.Offense", DyingRule(), Json::array()));
				modules["modules"].back()["sizeVariants"] = Json::array({ Json{ { "shape", Json::array({ Json::array({ 0, 0 }) }) }, { "ownerBurstRules", rules } } });
				modules["modules"].back()["sizeLadder"] = Json::array({ 1 });
				return modules;
			};
			const auto reject = [&](const char* name, const Json& rules)
			{
				const auto path = WriteJson(root, std::string{ "reject-" } + name + ".json", withRules(rules));
				const auto loaded = content::ModuleLoader::LoadFromFile(path);
				report.Check(!loaded.Succeeded(), std::string{ "dying star loader rejects " } + name, loaded.error);
			};
			const auto accept = [&](const char* name, const Json& rules)
			{
				const auto path = WriteJson(root, std::string{ "accept-" } + name + ".json", withRules(rules));
				const auto loaded = content::ModuleLoader::LoadFromFile(path);
				report.Check(loaded.Succeeded(), std::string{ "dying star loader accepts " } + name, loaded.error);
				return loaded;
			};
			const auto without = [](Json rule, const char* key) { rule.erase(key); return rule; };
			Json shieldCharged = ShatterRule();
			shieldCharged["chargeKills"] = 10;
			shieldCharged["chargeRadius"] = 300.0;
			reject("kill-without-charge", Json::array({ without(DyingRule(), "chargeKills") }));
			reject("charge-on-shield-break", Json::array({ shieldCharged }));
			reject("charge-radius-without-charge", Json::array({ [] { Json rule = ShatterRule(); rule["chargeRadius"] = 300.0; return rule; }() }));
			reject("charge-without-radius", Json::array({ without(DyingRule(), "chargeRadius") }));
			reject("zero-charge-kills", Json::array({ [] { Json rule = DyingRule(); rule["chargeKills"] = 0; return rule; }() }));
			reject("fractional-charge-kills", Json::array({ [] { Json rule = DyingRule(); rule["chargeKills"] = 2.5; return rule; }() }));
			reject("both-damage-forms", Json::array({ [] { Json rule = DyingRule(); rule["damageFractionOfMaxHealth"] = 0.1; return rule; }() }));
			reject("no-damage-form", Json::array({ without(DyingRule(), "damage") }));
			reject("zero-flat-damage", Json::array({ [] { Json rule = DyingRule(); rule["damage"] = Json::array({ 0.0, 69.0, 78.0 }); return rule; }() }));
			reject("ignite-without-thermal", Json::array({ [] { Json rule = DyingRule(); rule["damageType"] = "Damage.Type.Energy"; return rule; }() }));
			reject("negative-ignite", Json::array({ [] { Json rule = DyingRule(); rule["igniteStacks"] = -1; return rule; }() }));
			reject("charge-negative-cooldown", Json::array({ [] { Json rule = DyingRule(); rule["cooldownSeconds"] = -1.0; return rule; }() }));
			reject("shield-break-without-cooldown", Json::array({ without(ShatterRule(), "cooldownSeconds") }));

			const auto accepted = accept("charge-without-cooldown", Json::array({ DyingRule() }));
			if (accepted.Succeeded())
			{
				const auto found = std::find_if(accepted.definitions.begin(), accepted.definitions.end(),
					[](const ModuleDefinition& definition) { return definition.moduleId == sas::ContentId{ kRejectId }; });
				const bool shaped = found != accepted.definitions.end() && found->sizeVariants.size() == 1 && found->sizeVariants[0].ownerBurstRules.size() == 1;
				const ModuleOwnerBurstRule* rule = shaped ? &found->sizeVariants[0].ownerBurstRules[0] : nullptr;
				report.Check(rule && rule->trigger == ModuleTriggerKind::KillConfirmed && rule->chargeKills == 10 && rule->igniteStacks == 4 &&
					rule->damageByLevel.values.size() == 3 && rule->cooldownSeconds == 0.f && rule->damageFractionOfMaxHealth == 0.f,
					"a charge rule parses with flat level damage, Ignite stacks and no cooldown");
			}
		}

		void SideLoaderChecks(Report& report, const std::filesystem::path& root)
		{
			const auto reject = [&](const char* name, const Json& rules)
			{
				Json features = Json::array({ Json{ { "id", "Module.SideFeature.Fixture.Probe" }, { "displayName", "Probe" }, { "description", "probe" },
					{ "levelCount", 3 }, { "rules", rules } } });
				const auto path = WriteJson(root, std::string{ "side-reject-" } + name + ".json", SideFile(features));
				const auto loaded = content::ModuleSideFeatureLoader::LoadFromFile(path);
				report.Check(!loaded.Succeeded(), std::string{ "side feature loader rejects " } + name, loaded.error);
			};
			const auto accept = [&](const char* name, const Json& rules)
			{
				Json features = Json::array({ Json{ { "id", "Module.SideFeature.Fixture.Probe" }, { "displayName", "Probe" }, { "description", "probe" },
					{ "levelCount", 3 }, { "rules", rules } } });
				const auto path = WriteJson(root, std::string{ "side-accept-" } + name + ".json", SideFile(features));
				const auto loaded = content::ModuleSideFeatureLoader::LoadFromFile(path);
				report.Check(loaded.Succeeded(), std::string{ "side feature loader accepts " } + name, loaded.error);
			};
			reject("owner-burst-rule", Json{ { "ownerBurstRules", Json::array({ DyingRule() }) } });
			reject("modifier-unknown-field", Json{ { "ownerBurstModifiers", Json::array({ Json{ { "shield", 3 } } }) } });
			reject("modifier-empty", Json{ { "ownerBurstModifiers", Json::array({ Json::object() }) } });
			reject("echo-without-fraction", Json{ { "ownerBurstModifiers", Json::array({ Json{ { "echoDelaySeconds", 1.5 } } }) } });
			reject("echo-push-without-echo", Json{ { "ownerBurstModifiers", Json::array({ Json{ { "echoPushFraction", 0.5 } } }) } });
			reject("echo-fraction-above-one", Json{ { "ownerBurstModifiers", Json::array({ Json{ { "echoDelaySeconds", 1.5 }, { "echoFraction", 1.5 } } }) } });
			reject("pull-without-lead", Json{ { "ownerBurstModifiers", Json::array({ Json{ { "pullImpulse", 400.0 } } }) } });
			reject("lead-without-pull", Json{ { "ownerBurstModifiers", Json::array({ Json{ { "pullLeadSeconds", 1.0 } } }) } });
			reject("unknown-pull-immunity", Json{ { "ownerBurstModifiers", Json::array({ Json{ { "pullImpulse", 400.0 }, { "pullLeadSeconds", 1.0 }, { "pullImmuneClasses", Json::array({ "Titan" }) } } }) } });
			reject("negative-charge-radius", Json{ { "ownerBurstModifiers", Json::array({ Json{ { "chargeRadius", -1.0 } } }) } });
			accept("modifier-only", Json{ { "ownerBurstModifiers", Json::array({ Json{ { "chargeRadius", 450.0 } } }) } });
			accept("echo-without-damage", Json{ { "ownerBurstModifiers", Json::array({ Json{ { "echoDelaySeconds", 1.5 }, { "echoFraction", 0.0 }, { "echoPushFraction", 0.5 } } }) } });

			const auto shippedSide = content::ModuleSideFeatureLoader::LoadFromFile(
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/module_side_features.json");
			report.Check(shippedSide.Succeeded(), "shipped side features still load through the modifier-aware loader", shippedSide.error);
		}
	}
}

int main(int argc, char** argv)
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	using namespace rare_dying_star_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare dying star content registration failed" };
		std::string title = "Rare Dying Star E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);

		const std::filesystem::path root = std::filesystem::path{ argv[1] }.parent_path() / "rare-dying-star-fixtures";
		Json shipped;
		{
			std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json" };
			input >> shipped;
		}
		const auto fixturePath = WriteJson(root, "rare-dying-star-modules.json", FixtureSet(shipped));
		const auto loaded = content::ModuleLoader::LoadFromFile(fixturePath);
		report.Check(loaded.Succeeded(), "dying star fixture modules load through the real ModuleLoader", loaded.error);
		gFixtureDefinitions = loaded.definitions;
		const auto sidePath = WriteJson(root, "rare-dying-star-side-features.json", FixtureSideFeatures());
		const auto sideLoaded = content::ModuleSideFeatureLoader::LoadFromFile(sidePath);
		report.Check(sideLoaded.Succeeded(), "dying star fixture side features load through the real side feature loader", sideLoaded.error);
		gFixtureSideFeatures = sideLoaded.definitions;

		DyingChargeChecks(report, application);
		DyingRadiusChecks(report, application);
		DyingSelfKillChecks(report, application);
		DyingModuleKillChecks(report, application);
		DyingShieldBreakChecks(report, application);
		DyingLevelChecks(report, application);
		DyingRemovalChecks(report, application);
		CorePullChecks(report, application);
		RedGiantChecks(report, application);
		StaggerChecks(report, application);
		AftershockChecks(report, application);
		LoaderChecks(report, root, shipped);
		SideLoaderChecks(report, root);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareDyingStarMechanicsE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
