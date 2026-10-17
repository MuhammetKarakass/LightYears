#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"

#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/DamageStatusBalanceCatalog.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/control/ControlResponse.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace rare_status_e2e
{
	using namespace uncommon_p4_e2e;
	using Json = common_module_e2e::Json;

	namespace
	{
		constexpr const char* kEmberId = "Module.Offense.RareEmberFixture.Rare";
		constexpr const char* kProbeId = "Module.Offense.RareStatusProbeFixture.Rare";
		constexpr const char* kKineticId = "Module.Offense.RareKineticFixture.Rare";
		constexpr const char* kHairId = "Module.Offense.RareKineticHairFixture.Rare";
		constexpr const char* kRejectId = "Module.Offense.RareStatusRejectFixture.Rare";

		// Test-only definitions parsed from fixture JSON. They never enter shipped content.
		vega::List<vega::ModuleDefinition> gFixtureDefinitions;

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

		// One single-cell Offense module whose only rule lives under ruleKey (statusRules or damageRules).
		Json FixtureModule(const char* id, const char* ruleKey, Json rules)
		{
			Json variant{ { "shape", Json::array({ Json::array({ 0, 0 }) }) } };
			variant[ruleKey] = std::move(rules);
			return Json{
				{ "id", id }, { "displayName", id }, { "rarity", "Rare" }, { "levelCount", 3 },
				{ "family", "Ability.Offense" }, { "sizeLadder", Json::array({ 1 }) },
				{ "sizeVariants", Json::array({ std::move(variant) }) }
			};
		}

		Json EmberRules()
		{
			return Json{ { "thermalBursts", Json::array({ Json{
				{ "stackThreshold", 4 }, { "damageFraction", Json::array({ 1.30, 1.40, 1.50 }) },
				{ "areaRadius", 200.0 }, { "areaDamageFraction", 0.6 } } }) } };
		}

		Json KineticRules()
		{
			return Json{ { "kineticConsumes", Json::array({ Json{
				{ "stackThreshold", 4 }, { "damageMultiplier", Json::array({ 1.10, 1.15, 1.20 }) },
				{ "rendFraction", 0.25 }, { "rendSeconds", 4.0 } } }) } };
		}

		Json HairRules()
		{
			return Json{ { "kineticConsumes", Json::array({ Json{
				{ "stackThreshold", 4 }, { "normalTargetThreshold", 3 },
				{ "damageMultiplier", Json::array({ 1.10, 1.15, 1.20 }) } } }) } };
		}

		// Shipped modules plus test-only Offense modules. The probe doubles every outgoing hit of its owner
		// (Always), which lets the checks prove that generated burst damage skips outgoing module rules.
		Json FixtureSet(const Json& shipped)
		{
			Json modules = shipped;
			modules["modules"].push_back(FixtureModule(kEmberId, "statusRules", EmberRules()));
			modules["modules"].push_back(FixtureModule(kProbeId, "damageRules", Json::array({ Json{ { "condition", "Always" }, { "damageMultiplier", 2.0 } } })));
			modules["modules"].push_back(FixtureModule(kKineticId, "statusRules", KineticRules()));
			modules["modules"].push_back(FixtureModule(kHairId, "statusRules", HairRules()));
			return modules;
		}

		void Advance(Fixture& f, float seconds)
		{
			f.level->TickInternal(seconds);
		}

		DamagePayload Thermal(int stacks)
		{
			DamagePayload payload = Plain();
			payload.igniteStacks = stacks;
			return payload;
		}

		DamagePayload Kinetic(int stacks)
		{
			DamagePayload payload = Plain();
			payload.kineticStacks = stacks;
			return payload;
		}

		float ThermalHit(SpaceShip& target, Actor* source, float damage = 1.f)
		{
			return Hit(target, source, damage, Thermal(1), {}, {}, DamageDeliveryType::Direct, { DamageTypeSchema::Thermal });
		}

		float KineticHit(SpaceShip& target, Actor* source, int stacks)
		{
			return Hit(target, source, 1.f, Kinetic(stacks), {}, {}, DamageDeliveryType::Direct, { DamageTypeSchema::Kinetic });
		}

		float Health(SpaceShip& ship)
		{
			return ship.GetHealthComponent().GetHealth();
		}

		float Armor(SpaceShip& ship)
		{
			return ship.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::Armor);
		}

		bool HasEffect(SpaceShip& ship, const char* effectId)
		{
			return ship.GetAbilitySystemComponent().FindGameplayEffectById(effectId) != nullptr;
		}

		int StackCount(SpaceShip& ship, const char* effectId)
		{
			const auto* effect = ship.GetAbilitySystemComponent().FindGameplayEffectById(effectId);
			return effect ? effect->stackCount : 0;
		}

		float IgnitePending(SpaceShip& ship)
		{
			const auto* effect = ship.GetAbilitySystemComponent().FindGameplayEffectById(DamageStatusEffectIds::IgniteEffectId);
			if (!effect) return 0.f;
			const auto* pending = sas::FindAttribute(effect->runtimeAttributes, DamageAttributeIds::IgnitePendingDamage);
			return pending ? pending->currentValue : 0.f;
		}

		// Health lost by a 1-damage plain hit of the owner against an unshielded, unarmored target: the owner's outgoing scale.
		float OutgoingScale(Fixture& f)
		{
			const auto gauge = Enemy(f);
			gauge->SetActorLocation({ 30000.f, 30000.f });
			return Hit(*gauge, f.ship.get(), 1.f, Plain());
		}

		struct BurstRun
		{
			float outgoing = 0.f;
			float threeStackDrop = 0.f;
			int stacksAfterThree = 0;
			float pendingBeforeFourth = 0.f;
			float fourthDrop = 0.f;
			int stacksAfterFourth = 0;
			bool igniteRemoved = false;
			float lateDrop = 0.f;
		};

		// Three Thermal hits, a short pause that leaves pending DoT, then the fourth stack. Afterwards one second of time
		// shows whether any Ignite DoT is still delivered.
		BurstRun RunBurst(Report& report, Application& application, int level, bool withProbe)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			BurstRun run;
			run.outgoing = OutgoingScale(f);
			f.Install(kEmberId, 0, level);
			if (withProbe) f.Install(kProbeId, 0, 1);
			report.Check(f.ship->GetCombatRuntime().GetModuleStatusRules().thermalBursts.size() == 1,
				"Ember status rule reaches the live ship through the real applier", Json{ { "level", level } });

			const auto target = Enemy(f);
			const float before = Health(*target);
			for (int stack = 0; stack < 3; ++stack) ThermalHit(*target, f.ship.get());
			run.threeStackDrop = before - Health(*target);
			run.stacksAfterThree = StackCount(*target, DamageStatusEffectIds::IgniteEffectId);
			Advance(f, 0.1f);
			run.pendingBeforeFourth = IgnitePending(*target);
			run.fourthDrop = ThermalHit(*target, f.ship.get());
			run.stacksAfterFourth = StackCount(*target, DamageStatusEffectIds::IgniteEffectId);
			run.igniteRemoved = !HasEffect(*target, DamageStatusEffectIds::IgniteEffectId);
			const float afterBurst = Health(*target);
			Advance(f, 1.f);
			run.lateDrop = afterBurst - Health(*target);
			return run;
		}

		void EmberChecks(Report& report, Application& application)
		{
			const auto& balance = vega::content::DamageStatusBalanceCatalog::Get();
			// Four Thermal stacks tick ThermalDamagePerSecond(4) for the Ignite duration: 5 DPS x 5 s.
			const float totalDoT = balance.ThermalDamagePerSecond(balance.thermal.maxStacks) * balance.thermal.duration;
			report.Check(std::abs(totalDoT - 25.f) < 1e-3f, "precondition: four-stack Thermal DoT is 5 DPS x 5 s from damage_status_balance.json",
				Json{ { "totalDoT", totalDoT } });

			for (int level = 1; level <= 3; ++level)
			{
				const float fraction = level == 1 ? 1.30f : level == 2 ? 1.40f : 1.50f;
				const BurstRun run = RunBurst(report, application, level, false);
				report.Check(std::abs(run.threeStackDrop - 3.f * run.outgoing) < 0.01f, "three Thermal stacks produce no burst, only their hits",
					Json{ { "level", level }, { "drop", run.threeStackDrop }, { "outgoing", run.outgoing } });
				report.Check(run.stacksAfterThree == 3 && run.pendingBeforeFourth > 0.f, "three stacks remain and leave pending DoT for the cancel check",
					Json{ { "level", level }, { "stacks", run.stacksAfterThree }, { "pending", run.pendingBeforeFourth } });
				const float expected = run.outgoing * (1.f + fraction * totalDoT);
				report.Check(std::abs(run.fourthDrop - expected) < 0.05f, "fourth stack bursts once for fraction x total DoT on top of its hit",
					Json{ { "level", level }, { "drop", run.fourthDrop }, { "expected", expected } });
				report.Check(run.stacksAfterFourth == 0 && run.igniteRemoved, "burst clears all Thermal stacks",
					Json{ { "level", level }, { "stacks", run.stacksAfterFourth } });
				report.Check(std::abs(run.lateDrop) < 0.01f, "burst cancels pending Ignite DoT; none is delivered afterwards",
					Json{ { "level", level }, { "lateDrop", run.lateDrop } });
			}
		}

		void ProbeChecks(Report& report, Application& application)
		{
			const float totalDoT = 25.f;
			const BurstRun run = RunBurst(report, application, 1, true);
			report.Check(std::abs(run.threeStackDrop - 6.f * run.outgoing) < 0.01f, "probe doubles the owner's three ordinary hits",
				Json{ { "drop", run.threeStackDrop }, { "outgoing", run.outgoing } });
			// Only the 4th hit carries the probe's x2; the burst itself is generated damage and must not be multiplied.
			const float expected = run.outgoing * (2.f + 1.30f * totalDoT);
			report.Check(std::abs(run.fourthDrop - expected) < 0.05f, "burst damage ignores outgoing module rules of the owner",
				Json{ { "drop", run.fourthDrop }, { "expected", expected } });
		}

		void AreaChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			const float outgoing = OutgoingScale(f);
			f.Install(kEmberId, 0, 1);
			const auto target = Enemy(f);
			const float before = Health(*target);
			for (int stack = 0; stack < 3; ++stack) ThermalHit(*target, f.ship.get());
			Advance(f, 0.1f);
			// Neighbours are spawned after the pause: an undamaged enemy regenerates shield from spawn, so any
			// elapsed time before the burst would let its shield absorb part of the area hit. Spawning them here
			// (Enemy() ticks with dt 0) keeps their shield at zero when the burst lands.
			const auto near = Enemy(f);
			const auto far = Enemy(f);
			near->SetActorLocation(target->GetActorLocation() + sf::Vector2f{ 0.f, 120.f });
			far->SetActorLocation(target->GetActorLocation() + sf::Vector2f{ 0.f, 400.f });
			const float nearBefore = Health(*near);
			const float farBefore = Health(*far);
			ThermalHit(*target, f.ship.get());
			const float nearDrop = nearBefore - Health(*near);
			const float farDrop = farBefore - Health(*far);
			const float expectedNear = outgoing * 0.6f * 1.30f * 25.f;
			report.Check(std::abs(nearDrop - expectedNear) < 0.05f, "burst reaches an enemy inside areaRadius at 60 percent of the burst",
				Json{ { "drop", nearDrop }, { "expected", expectedNear } });
			report.Check(farDrop < 0.01f, "burst leaves an enemy outside areaRadius untouched", Json{ { "drop", farDrop } });
			report.Check(StackCount(*near, DamageStatusEffectIds::IgniteEffectId) == 0 && !HasEffect(*near, DamageStatusEffectIds::IgniteEffectId),
				"area neighbour receives no Thermal stacks or DoT");
			const float targetDrop = before - Health(*target);
			report.Check(targetDrop > 0.f, "primary target still takes the burst and its hits", Json{ { "drop", targetDrop } });
		}

		// Ember without Ignite stacks ticks DoT; the same timeline with the module installed and removed must match exactly.
		float FourStackDoT(Report& report, Application& application, bool installThenRemove, float& outgoing, int& stacks)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			outgoing = OutgoingScale(f);
			if (installThenRemove)
			{
				const auto ember = f.Install(kEmberId, 0, 1);
				f.Remove(ember);
				report.Check(f.ship->GetCombatRuntime().GetModuleStatusRules().Empty(), "removed Ember leaves no status rules");
			}
			const auto target = Enemy(f);
			for (int stack = 0; stack < 4; ++stack) ThermalHit(*target, f.ship.get());
			stacks = StackCount(*target, DamageStatusEffectIds::IgniteEffectId);
			const float before = Health(*target);
			Advance(f, 2.1f);
			return before - Health(*target);
		}

		void RemovalChecks(Report& report, Application& application)
		{
			float outgoingBaseline = 0.f;
			float outgoingRemoved = 0.f;
			int stacksBaseline = 0;
			int stacksRemoved = 0;
			const float baseline = FourStackDoT(report, application, false, outgoingBaseline, stacksBaseline);
			const float removed = FourStackDoT(report, application, true, outgoingRemoved, stacksRemoved);
			report.Check(stacksBaseline == 4 && stacksRemoved == 4, "four Thermal stacks exist with and without the removed module",
				Json{ { "baseline", stacksBaseline }, { "removed", stacksRemoved } });
			report.Check(baseline > 5.f * outgoingBaseline, "without a module four Thermal stacks keep ticking DoT",
				Json{ { "baselineDoT", baseline }, { "outgoing", outgoingBaseline } });
			report.Check(std::abs(baseline - removed) < 0.05f, "after Ember removal the Thermal status behaves exactly like the no-module baseline",
				Json{ { "baseline", baseline }, { "removed", removed } });
		}

		// Kinetic stacks are read back through the ASC effect; Armor Rend is a separate timed effect.
		void KineticChecks(Report& report, Application& application)
		{
			for (int level = 1; level <= 3; ++level)
			{
				const float multiplier = level == 1 ? 1.10f : level == 2 ? 1.15f : 1.20f;
				Fixture f{ report, application };
				f.rules.lookup = FixtureLookup;
				const auto gauge = Enemy(f);
				gauge->SetActorLocation({ 30000.f, 30000.f });
				const float baseline = Hit(*gauge, f.ship.get(), 10.f, Plain());
				f.Install(kKineticId, 0, level);
				const auto target = Enemy(f);
				target->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 100.f);
				report.Check(std::abs(Armor(*target) - 100.f) < 1e-3f, "precondition: Kinetic target has Armor 100", Json{ { "armor", Armor(*target) } });

				for (int hit = 0; hit < 3; ++hit) KineticHit(*target, f.ship.get(), 1);
				report.Check(StackCount(*target, DamageStatusEffectIds::KineticEffectId) == 3, "three Kinetic stacks are present",
					Json{ { "level", level } });
				Hit(*target, f.ship.get(), 1.f, Plain());
				report.Check(StackCount(*target, DamageStatusEffectIds::KineticEffectId) == 3 && !HasEffect(*target, DamageStatusEffectIds::KineticArmorRendEffectId),
					"three Kinetic stacks are not spent by a direct hit", Json{ { "level", level } });
				KineticHit(*target, f.ship.get(), 1);
				report.Check(StackCount(*target, DamageStatusEffectIds::KineticEffectId) == 4, "four Kinetic stacks are present", Json{ { "level", level } });
				Hit(*target, f.ship.get(), 1.f, Kinetic(0), {}, {}, DamageDeliveryType::Periodic, { DamageTypeSchema::Kinetic });
				report.Check(StackCount(*target, DamageStatusEffectIds::KineticEffectId) == 4, "a periodic hit does not spend Kinetic stacks", Json{ { "level", level } });

				const float spend = Hit(*target, f.ship.get(), 10.f, Plain());
				report.Check(std::abs(spend - baseline * multiplier) < 0.05f, "spending direct hit ignores Armor and applies the level multiplier",
					Json{ { "level", level }, { "spend", spend }, { "expected", baseline * multiplier } });
				report.Check(!HasEffect(*target, DamageStatusEffectIds::KineticEffectId), "spend clears all Kinetic stacks", Json{ { "level", level } });
				report.Check(HasEffect(*target, DamageStatusEffectIds::KineticArmorRendEffectId) && std::abs(Armor(*target) - 75.f) < 0.01f,
					"Armor Rend lowers Armor by 25 percent right after the spend", Json{ { "armor", Armor(*target) } });
				Advance(f, 3.5f);
				report.Check(std::abs(Armor(*target) - 75.f) < 0.01f, "Armor Rend still applies at 3.5 seconds", Json{ { "armor", Armor(*target) } });
				Advance(f, 0.7f);
				report.Check(std::abs(Armor(*target) - 100.f) < 0.01f && !HasEffect(*target, DamageStatusEffectIds::KineticArmorRendEffectId),
					"Armor Rend ends after 4 seconds", Json{ { "armor", Armor(*target) } });
			}

			// Hair Trigger: a Normal-class target spends at 3 stacks.
			{
				Fixture f{ report, application };
				f.rules.lookup = FixtureLookup;
				const auto gauge = Enemy(f);
				gauge->SetActorLocation({ 30000.f, 30000.f });
				const float baseline = Hit(*gauge, f.ship.get(), 10.f, Plain());
				f.Install(kHairId, 0, 1);
				const auto target = Enemy(f);
				target->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 100.f);
				KineticHit(*target, f.ship.get(), 1);
				KineticHit(*target, f.ship.get(), 1);
				Hit(*target, f.ship.get(), 1.f, Plain());
				report.Check(StackCount(*target, DamageStatusEffectIds::KineticEffectId) == 2, "Normal target keeps two stacks below normalTargetThreshold");
				KineticHit(*target, f.ship.get(), 1);
				const float spend = Hit(*target, f.ship.get(), 10.f, Plain());
				report.Check(std::abs(spend - baseline * 1.10f) < 0.05f && !HasEffect(*target, DamageStatusEffectIds::KineticEffectId),
					"Normal target spends at normalTargetThreshold 3", Json{ { "spend", spend }, { "expected", baseline * 1.10f } });
			}

			// Hair Trigger on an Elite target keeps the full threshold of 4.
			{
				Fixture f{ report, application };
				f.rules.lookup = FixtureLookup;
				const auto gauge = Enemy(f);
				gauge->SetActorLocation({ 30000.f, 30000.f });
				const float baseline = Hit(*gauge, f.ship.get(), 10.f, Plain());
				f.Install(kHairId, 0, 1);
				const auto target = Enemy(f);
				target->SetControlTargetClass(ControlTargetClass::Elite);
				target->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 100.f);
				for (int hit = 0; hit < 3; ++hit) KineticHit(*target, f.ship.get(), 1);
				Hit(*target, f.ship.get(), 1.f, Plain());
				report.Check(StackCount(*target, DamageStatusEffectIds::KineticEffectId) == 3, "Elite target does not spend at three stacks");
				KineticHit(*target, f.ship.get(), 1);
				const float spend = Hit(*target, f.ship.get(), 10.f, Plain());
				report.Check(std::abs(spend - baseline * 1.10f) < 0.05f && !HasEffect(*target, DamageStatusEffectIds::KineticEffectId),
					"Elite target spends at the full threshold 4", Json{ { "spend", spend }, { "expected", baseline * 1.10f } });
			}

			// Removing the module restores ordinary Armor shred: the same spend-shaped hit is reduced by Armor.
			{
				Fixture f{ report, application };
				f.rules.lookup = FixtureLookup;
				const auto gauge = Enemy(f);
				gauge->SetActorLocation({ 30000.f, 30000.f });
				const float baseline = Hit(*gauge, f.ship.get(), 10.f, Plain());
				const auto kinetic = f.Install(kKineticId, 0, 1);
				f.Remove(kinetic);
				report.Check(f.ship->GetCombatRuntime().GetModuleStatusRules().Empty(), "removed Kinetic Breaker leaves no status rules");
				const auto target = Enemy(f);
				target->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 100.f);
				for (int hit = 0; hit < 4; ++hit) KineticHit(*target, f.ship.get(), 1);
				const float plain = Hit(*target, f.ship.get(), 10.f, Plain());
				report.Check(StackCount(*target, DamageStatusEffectIds::KineticEffectId) == 4 && !HasEffect(*target, DamageStatusEffectIds::KineticArmorRendEffectId),
					"without the module four Kinetic stacks stay and no Armor Rend is created");
				report.Check(plain < baseline - 0.5f, "without the module Kinetic shred reduces the hit by Armor again",
					Json{ { "plain", plain }, { "baseline", baseline } });
			}
		}

		void LoaderChecks(Report& report, const std::filesystem::path& root, const Json& shipped)
		{
			const auto reject = [&](const char* name, const Json& statusRules)
			{
				Json modules = shipped;
				modules["modules"].push_back(FixtureModule(kRejectId, "statusRules", statusRules));
				const auto path = WriteJson(root, std::string{ "reject-" } + name + ".json", modules);
				const auto loaded = content::ModuleLoader::LoadFromFile(path);
				report.Check(!loaded.Succeeded(), std::string{ "status rule loader rejects " } + name, loaded.error);
			};
			const auto thermal = [](Json entry) { return Json{ { "thermalBursts", Json::array({ std::move(entry) }) } }; };
			const auto kinetic = [](Json entry) { return Json{ { "kineticConsumes", Json::array({ std::move(entry) }) } }; };
			reject("thermal-threshold-zero", thermal(Json{ { "stackThreshold", 0 }, { "damageFraction", 1.3 } }));
			reject("thermal-threshold-five", thermal(Json{ { "stackThreshold", 5 }, { "damageFraction", 1.3 } }));
			reject("thermal-missing-threshold", thermal(Json{ { "damageFraction", 1.3 } }));
			reject("thermal-negative-fraction", thermal(Json{ { "stackThreshold", 4 }, { "damageFraction", -1.0 } }));
			reject("thermal-unknown-field", thermal(Json{ { "stackThreshold", 4 }, { "damageFraction", 1.3 }, { "damage", 2 } }));
			reject("thermal-area-fraction-above-one", thermal(Json{ { "stackThreshold", 4 }, { "damageFraction", 1.3 }, { "areaRadius", 200.0 }, { "areaDamageFraction", 1.5 } }));
			reject("thermal-area-without-fraction", thermal(Json{ { "stackThreshold", 4 }, { "damageFraction", 1.3 }, { "areaRadius", 200.0 } }));
			reject("thermal-short-level-table", thermal(Json{ { "stackThreshold", 4 }, { "damageFraction", Json::array({ 1.3, 1.4 }) } }));
			reject("thermal-not-array", Json{ { "thermalBursts", Json::object() } });
			reject("kinetic-threshold-five", kinetic(Json{ { "stackThreshold", 5 }, { "damageMultiplier", 1.1 } }));
			reject("kinetic-normal-threshold-five", kinetic(Json{ { "stackThreshold", 4 }, { "normalTargetThreshold", 5 }, { "damageMultiplier", 1.1 } }));
			reject("kinetic-zero-multiplier", kinetic(Json{ { "stackThreshold", 4 }, { "damageMultiplier", 0.0 } }));
			reject("kinetic-rend-without-seconds", kinetic(Json{ { "stackThreshold", 4 }, { "damageMultiplier", 1.1 }, { "rendFraction", 0.25 } }));
			reject("kinetic-rend-above-one", kinetic(Json{ { "stackThreshold", 4 }, { "damageMultiplier", 1.1 }, { "rendFraction", 1.5 }, { "rendSeconds", 4.0 } }));
			reject("kinetic-negative-rend-seconds", kinetic(Json{ { "stackThreshold", 4 }, { "damageMultiplier", 1.1 }, { "rendFraction", 0.25 }, { "rendSeconds", -1.0 } }));
			reject("kinetic-short-level-table", kinetic(Json{ { "stackThreshold", 4 }, { "damageMultiplier", Json::array({ 1.1, 1.15 }) } }));
			reject("unknown-status-key", Json{ { "thermalBurst", Json::array() } });
			const auto shippedLoaded = content::ModuleLoader::LoadFromFile(
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json");
			report.Check(shippedLoaded.Succeeded(), "shipped modules still load through the status rule parsers", shippedLoaded.error);
		}
	}
}

int main(int argc, char** argv)
{
	vega::InstallVegaEngineAssets();
	using namespace rare_status_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare status content registration failed" };
		std::string title = "Rare Status Rules E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);

		const std::filesystem::path root = std::filesystem::path{ argv[1] }.parent_path() / "rare-status-fixtures";
		Json shipped;
		{
			std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json" };
			input >> shipped;
		}
		const auto fixturePath = WriteJson(root, "rare-status-modules.json", FixtureSet(shipped));
		const auto loaded = content::ModuleLoader::LoadFromFile(fixturePath);
		report.Check(loaded.Succeeded(), "rare status fixture modules load through the real ModuleLoader", loaded.error);
		gFixtureDefinitions = loaded.definitions;

		EmberChecks(report, application);
		ProbeChecks(report, application);
		AreaChecks(report, application);
		RemovalChecks(report, application);
		KineticChecks(report, application);
		LoaderChecks(report, root, shipped);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareStatusRulesE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
