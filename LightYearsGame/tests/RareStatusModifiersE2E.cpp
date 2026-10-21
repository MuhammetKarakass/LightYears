#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"

#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/DamageStatusBalanceCatalog.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"
#include "gameplay/control/ControlResponse.h"
#include "gameplay/module/ModuleSideFeature.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace rare_status_modifiers_e2e
{
	using namespace vega;
	using namespace uncommon_p4_e2e;
	using Json = common_module_e2e::Json;

	namespace
	{
		constexpr const char* kEmberHost = "Module.Offense.RareModEmberFixture.Rare";
		constexpr const char* kKineticHost = "Module.Offense.RareModKineticFixture.Rare";
		constexpr const char* kSustainHost = "Module.Movement.RareModSustainFixture.Rare";
		constexpr const char* kNoMainHost = "Module.Offense.RareModNoMainFixture.Rare";
		constexpr const char* kRejectHost = "Module.Offense.RareModRejectFixture.Rare";
		constexpr const char* kAreaSide = "Module.SideFeature.RareModTest.Area";
		constexpr const char* kSmolderSide = "Module.SideFeature.RareModTest.Smolder";
		constexpr const char* kRendSide = "Module.SideFeature.RareModTest.Rend";
		constexpr const char* kHairSide = "Module.SideFeature.RareModTest.Hair";
		constexpr const char* kAfterglowSide = "Module.SideFeature.RareModTest.Afterglow";
		// Ember main burst at level 1 (1.30 x a four-stack Thermal DoT of 5 DPS x 5 s).
		constexpr float kFourStackDoT = 25.f;

		// Test-only definitions parsed from fixture JSON. They never enter shipped content.
		List<ModuleDefinition> gFixtureDefinitions;
		List<ModuleSideFeatureDefinition> gFixtureSideFeatures;

		const ModuleDefinition* FixtureLookup(const sas::ContentId& id)
		{
			for (const ModuleDefinition& definition : gFixtureDefinitions)
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
			f.rules.lookup = FixtureLookup;
			f.rules.sideFeatureLookup = FixtureSideFeature;
		}

		std::filesystem::path WriteJson(const std::filesystem::path& root, const std::string& name, const Json& value)
		{
			std::filesystem::create_directories(root);
			const std::filesystem::path path = root / name;
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			output << value.dump(2) << '\n';
			return path;
		}

		// Two size variants (1 and 2 cells): a side feature applies only on the largest variant. variantRules is the main package.
		Json Host(const char* id, const char* family, const Json& variantRules, const std::vector<const char*>& sides)
		{
			Json sideIds = Json::array();
			for (const char* side : sides) sideIds.push_back(side);
			Json small{ { "shape", Json::array({ Json::array({ 0, 0 }) }) } };
			Json large{ { "shape", Json::array({ Json::array({ 0, 0 }), Json::array({ 1, 0 }) }) } };
			for (const auto& entry : variantRules.items())
			{
				small[entry.key()] = entry.value();
				large[entry.key()] = entry.value();
			}
			return Json{
				{ "id", id }, { "displayName", id }, { "rarity", "Rare" }, { "levelCount", 3 }, { "family", family },
				{ "sizeLadder", Json::array({ 1, 2 }) }, { "sizeVariants", Json::array({ std::move(small), std::move(large) }) },
				{ "sideFeatures", std::move(sideIds) }
			};
		}

		Json EmberMain()
		{
			return Json{ { "statusRules", Json{ { "thermalBursts", Json::array({ Json{ { "stackThreshold", 4 }, { "damageFraction", Json::array({ 1.30, 1.40, 1.50 }) } } }) } } } };
		}

		Json KineticMain()
		{
			return Json{ { "statusRules", Json{ { "kineticConsumes", Json::array({ Json{ { "stackThreshold", 4 }, { "damageMultiplier", Json::array({ 1.10, 1.15, 1.20 }) } } }) } } } };
		}

		Json SustainMain()
		{
			return Json{ { "sustainedSpeedRules", Json::array({ Json{ { "speedFraction", 0.9 }, { "holdSeconds", 2.0 }, { "primarySpeedMultiplier", 2.0 },
				{ "primaryPierceBonus", 1.0 }, { "primaryDamageMultiplier", Json::array({ 1.10, 1.15, 1.20 }) } } }) } };
		}

		// A main package with no thermal or kinetic rule: a side modifier merged into it must add nothing.
		Json NoMainRules()
		{
			return Json{ { "damageRules", Json::array({ Json{ { "condition", "Always" }, { "damageMultiplier", 2.0 } } }) } };
		}

		Json FixtureSet(const Json& shipped)
		{
			Json modules = shipped;
			modules["modules"].push_back(Host(kEmberHost, "Ability.Offense", EmberMain(), { kAreaSide, kSmolderSide }));
			modules["modules"].push_back(Host(kKineticHost, "Ability.Offense", KineticMain(), { kRendSide, kHairSide }));
			modules["modules"].push_back(Host(kSustainHost, "Ability.Movement", SustainMain(), { kAfterglowSide }));
			modules["modules"].push_back(Host(kNoMainHost, "Ability.Offense", NoMainRules(), { kAreaSide }));
			return modules;
		}

		// Modifier-only side entries (no damage of their own). Smolder stacks default to one per second.
		Json SideFeature(const char* id, Json rules)
		{
			return Json{ { "id", id }, { "displayName", id }, { "levelCount", 3 }, { "rules", std::move(rules) } };
		}

		Json ValidSideFeatures()
		{
			return Json::array({
				SideFeature(kAreaSide, Json{ { "statusRules", Json{ { "thermalBursts", Json::array({ Json{ { "areaRadius", 200.0 }, { "areaDamageFraction", 0.6 } } }) } } } }),
				SideFeature(kSmolderSide, Json{ { "statusRules", Json{ { "thermalBursts", Json::array({ Json{ { "smolderSeconds", 3.0 }, { "smolderRadius", 150.0 } } }) } } } }),
				SideFeature(kRendSide, Json{ { "statusRules", Json{ { "kineticConsumes", Json::array({ Json{ { "rendFraction", 0.25 }, { "rendSeconds", 4.0 } } }) } } } }),
				SideFeature(kHairSide, Json{ { "statusRules", Json{ { "kineticConsumes", Json::array({ Json{ { "normalTargetThreshold", 3 } } }) } } } }),
				SideFeature(kAfterglowSide, Json{ { "sustainedSpeedRules", Json::array({ Json{ { "lingerSeconds", 1.5 } } }) } })
			});
		}

		Json SideDocument(Json features)
		{
			return Json{ { "schemaVersion", 1 }, { "sharedPools", Json::array() }, { "sideFeatures", std::move(features) } };
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

		const ModuleStatusRules& RuleStatusOf(Fixture& f)
		{
			return f.ship->GetCombatRuntime().GetModuleStatusRules();
		}

		// Health lost by a 1-damage plain hit of the owner against a distant, unshielded target.
		float OutgoingScale(Fixture& f)
		{
			const auto gauge = Enemy(f);
			gauge->SetActorLocation({ 30000.f, 30000.f });
			return Hit(*gauge, f.ship.get(), 1.f, Plain());
		}

		void PrecheckDoT(Report& report)
		{
			const auto& balance = vega::content::DamageStatusBalanceCatalog::Get();
			const float totalDoT = balance.ThermalDamagePerSecond(balance.thermal.maxStacks) * balance.thermal.duration;
			report.Check(std::abs(totalDoT - kFourStackDoT) < 1e-3f, "precondition: four-stack Thermal DoT is 5 DPS x 5 s from damage_status_balance.json",
				Json{ { "totalDoT", totalDoT } });
		}

		// Area Blast: the side area widens the one Ember burst rule. The burst is still a single burst on the target and a
		// fraction of it on the neighbour inside the area; the neighbour outside the area takes nothing.
		void AreaChecks(Report& report, Application& application)
		{
			for (int level = 1; level <= 3; ++level)
			{
				const float fraction = level == 1 ? 1.30f : level == 2 ? 1.40f : 1.50f;
				Fixture f{ report, application };
				Wire(f);
				const float outgoing = OutgoingScale(f);
				f.Install(kEmberHost, 1, level, kAreaSide);
				const auto& rules = RuleStatusOf(f).thermalBursts;
				report.Check(rules.size() == 1 && std::abs(rules[0].areaRadius - 200.f) < 1e-3f && std::abs(rules[0].areaDamageFraction - 0.6f) < 1e-4f &&
					rules[0].smolderSeconds == 0.f, "Area Blast merges into the single Ember burst rule", Json{ { "level", level }, { "rules", rules.size() } });

				const auto target = Enemy(f);
				for (int stack = 0; stack < 3; ++stack) ThermalHit(*target, f.ship.get());
				Advance(f, 0.1f);
				// Neighbours are spawned after the pause so their shields are empty when the burst lands.
				const auto near = Enemy(f);
				const auto far = Enemy(f);
				near->SetActorLocation(target->GetActorLocation() + sf::Vector2f{ 0.f, 120.f });
				far->SetActorLocation(target->GetActorLocation() + sf::Vector2f{ 0.f, 400.f });
				const float nearBefore = Health(*near);
				const float farBefore = Health(*far);
				const float fourthDrop = ThermalHit(*target, f.ship.get());
				const float nearDrop = nearBefore - Health(*near);
				const float farDrop = farBefore - Health(*far);
				const float expectedTarget = outgoing * (1.f + fraction * kFourStackDoT);
				const float expectedNear = outgoing * 0.6f * fraction * kFourStackDoT;
				report.Check(std::abs(fourthDrop - expectedTarget) < 0.05f, "target takes its hit plus exactly one burst (no second burst from the side)",
					Json{ { "level", level }, { "drop", fourthDrop }, { "expected", expectedTarget } });
				report.Check(std::abs(nearDrop - expectedNear) < 0.05f, "neighbour inside the merged area takes 60 percent of the burst",
					Json{ { "level", level }, { "drop", nearDrop }, { "expected", expectedNear } });
				report.Check(farDrop < 0.01f, "neighbour outside the merged area is untouched", Json{ { "drop", farDrop } });
				report.Check(StackCount(*near, DamageStatusEffectIds::IgniteEffectId) == 0, "area neighbour gets no Thermal stacks");
			}

			// Without the side feature the same timeline has no area at all.
			Fixture plain{ report, application };
			Wire(plain);
			plain.Install(kEmberHost, 0, 1);
			report.Check(RuleStatusOf(plain).thermalBursts.size() == 1 && RuleStatusOf(plain).thermalBursts[0].areaRadius == 0.f, "without Area Blast the Ember rule keeps no area");
			const auto target = Enemy(plain);
			for (int stack = 0; stack < 3; ++stack) ThermalHit(*target, plain.ship.get());
			Advance(plain, 0.1f);
			const auto near = Enemy(plain);
			near->SetActorLocation(target->GetActorLocation() + sf::Vector2f{ 0.f, 120.f });
			const float nearBefore = Health(*near);
			ThermalHit(*target, plain.ship.get());
			report.Check(std::abs(nearBefore - Health(*near)) < 0.01f, "without the side feature the neighbour takes nothing from the burst");

			// A side area on a main package with no burst rule adds no rule.
			Fixture noMain{ report, application };
			Wire(noMain);
			noMain.Install(kNoMainHost, 1, 1, kAreaSide);
			report.Check(RuleStatusOf(noMain).thermalBursts.empty(), "a side area without a main burst rule adds no rule");
		}

		// Smoldering Remains: the burst leaves a field for three seconds that adds one Thermal stack per second inside its radius.
		// The field adds no damage to the burst, pulses only while it lives, and is cancelled with its owner's module.
		void SmolderChecks(Report& report, Application& application)
		{
			{
				Fixture f{ report, application };
				Wire(f);
				const float outgoing = OutgoingScale(f);
				f.Install(kEmberHost, 1, 1, kSmolderSide);
				const auto& rules = RuleStatusOf(f).thermalBursts;
				report.Check(rules.size() == 1 && rules[0].smolderSeconds == 3.f && rules[0].smolderRadius == 150.f && rules[0].smolderStacksPerSecond == 1 &&
					rules[0].areaRadius == 0.f, "Smoldering Remains merges its fields into the single Ember burst rule", Json{ { "rules", rules.size() } });
				const auto target = Enemy(f);
				for (int stack = 0; stack < 3; ++stack) ThermalHit(*target, f.ship.get());
				Advance(f, 0.1f);
				const auto near = Enemy(f);
				const auto far = Enemy(f);
				near->SetActorLocation(target->GetActorLocation() + sf::Vector2f{ 0.f, 100.f });
				far->SetActorLocation(target->GetActorLocation() + sf::Vector2f{ 0.f, 400.f });
				const float fourthDrop = ThermalHit(*target, f.ship.get());
				report.Check(std::abs(fourthDrop - outgoing * (1.f + 1.30f * kFourStackDoT)) < 0.05f, "the field adds no damage to the burst itself",
					Json{ { "drop", fourthDrop } });
				report.Check(f.ship->GetModuleApplier().GetSmolderFieldCount() == 1, "the burst leaves exactly one smoldering field");

				Advance(f, 0.5f);
				report.Check(StackCount(*near, DamageStatusEffectIds::IgniteEffectId) == 0, "no pulse before the first full second");
				Advance(f, 0.6f);
				report.Check(StackCount(*near, DamageStatusEffectIds::IgniteEffectId) == 1 && StackCount(*far, DamageStatusEffectIds::IgniteEffectId) == 0,
					"first pulse adds one Thermal stack inside the radius only", Json{ { "near", StackCount(*near, DamageStatusEffectIds::IgniteEffectId) } });
				Advance(f, 1.0f);
				report.Check(StackCount(*near, DamageStatusEffectIds::IgniteEffectId) == 2, "second pulse adds the second stack");
				Advance(f, 1.0f);
				report.Check(StackCount(*near, DamageStatusEffectIds::IgniteEffectId) == 3 && f.ship->GetModuleApplier().GetSmolderFieldCount() == 0,
					"third pulse lands on the last frame and the field ends after three seconds",
					Json{ { "near", StackCount(*near, DamageStatusEffectIds::IgniteEffectId) } });
				Advance(f, 1.0f);
				report.Check(StackCount(*near, DamageStatusEffectIds::IgniteEffectId) == 3, "an ended field pulses no more");
				report.Check(StackCount(*far, DamageStatusEffectIds::IgniteEffectId) == 0, "the far neighbour never receives a stack");
			}

			{
				Fixture f{ report, application };
				Wire(f);
				f.Install(kEmberHost, 1, 1);
				const auto target = Enemy(f);
				for (int stack = 0; stack < 3; ++stack) ThermalHit(*target, f.ship.get());
				Advance(f, 0.1f);
				const auto near = Enemy(f);
				near->SetActorLocation(target->GetActorLocation() + sf::Vector2f{ 0.f, 100.f });
				ThermalHit(*target, f.ship.get());
				report.Check(f.ship->GetModuleApplier().GetSmolderFieldCount() == 0, "without the side feature a burst leaves no field");
				Advance(f, 3.5f);
				report.Check(StackCount(*near, DamageStatusEffectIds::IgniteEffectId) == 0, "without the side feature nothing pulses");
			}

			{
				Fixture f{ report, application };
				Wire(f);
				const auto ember = f.Install(kEmberHost, 1, 1, kSmolderSide);
				const auto target = Enemy(f);
				for (int stack = 0; stack < 3; ++stack) ThermalHit(*target, f.ship.get());
				Advance(f, 0.1f);
				const auto near = Enemy(f);
				near->SetActorLocation(target->GetActorLocation() + sf::Vector2f{ 0.f, 100.f });
				ThermalHit(*target, f.ship.get());
				Advance(f, 1.2f);
				report.Check(StackCount(*near, DamageStatusEffectIds::IgniteEffectId) == 1 && f.ship->GetModuleApplier().GetSmolderFieldCount() == 1,
					"one pulse landed before the module is removed");
				f.Remove(ember);
				report.Check(f.ship->GetModuleApplier().GetSmolderFieldCount() == 0, "removing the Ember module cancels its field through the lease");
				Advance(f, 2.0f);
				report.Check(StackCount(*near, DamageStatusEffectIds::IgniteEffectId) == 1, "a cancelled field pulses no more",
					Json{ { "near", StackCount(*near, DamageStatusEffectIds::IgniteEffectId) } });
			}
		}

		// Armor Rend: the side modifier lowers the target's Armor for four seconds after the Kinetic Breaker spend.
		void RendChecks(Report& report, Application& application)
		{
			for (int level = 1; level <= 3; ++level)
			{
				const float multiplier = level == 1 ? 1.10f : level == 2 ? 1.15f : 1.20f;
				Fixture f{ report, application };
				Wire(f);
				const auto gauge = Enemy(f);
				gauge->SetActorLocation({ 30000.f, 30000.f });
				const float baseline = Hit(*gauge, f.ship.get(), 10.f, Plain());
				f.Install(kKineticHost, 1, level, kRendSide);
				const auto& consume = RuleStatusOf(f).kineticConsumes;
				report.Check(consume.size() == 1 && std::abs(consume[0].rendFraction - 0.25f) < 1e-4f && std::abs(consume[0].rendSeconds - 4.f) < 1e-4f &&
					consume[0].stackThreshold == 4, "Armor Rend merges into the single Kinetic consume rule", Json{ { "level", level } });
				const auto target = Enemy(f);
				target->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 100.f);
				for (int hit = 0; hit < 3; ++hit) KineticHit(*target, f.ship.get(), 1);
				Hit(*target, f.ship.get(), 1.f, Plain());
				KineticHit(*target, f.ship.get(), 1);
				const float spend = Hit(*target, f.ship.get(), 10.f, Plain());
				report.Check(std::abs(spend - baseline * multiplier) < 0.05f && !HasEffect(*target, DamageStatusEffectIds::KineticEffectId),
					"spend applies the level multiplier and clears the stacks", Json{ { "level", level }, { "spend", spend } });
				report.Check(HasEffect(*target, DamageStatusEffectIds::KineticArmorRendEffectId) && std::abs(Armor(*target) - 75.f) < 0.01f,
					"Armor Rend lowers Armor by 25 percent right after the spend", Json{ { "armor", Armor(*target) } });
				Advance(f, 3.5f);
				report.Check(std::abs(Armor(*target) - 75.f) < 0.01f, "Armor Rend still applies at 3.5 seconds", Json{ { "armor", Armor(*target) } });
				Advance(f, 0.7f);
				report.Check(std::abs(Armor(*target) - 100.f) < 0.01f && !HasEffect(*target, DamageStatusEffectIds::KineticArmorRendEffectId),
					"Armor Rend ends after 4 seconds", Json{ { "armor", Armor(*target) } });
			}

			Fixture plain{ report, application };
			Wire(plain);
			plain.Install(kKineticHost, 1, 1);
			const auto target = Enemy(plain);
			target->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 100.f);
			for (int hit = 0; hit < 3; ++hit) KineticHit(*target, plain.ship.get(), 1);
			Hit(*target, plain.ship.get(), 1.f, Plain());
			KineticHit(*target, plain.ship.get(), 1);
			Hit(*target, plain.ship.get(), 10.f, Plain());
			report.Check(!HasEffect(*target, DamageStatusEffectIds::KineticArmorRendEffectId) && std::abs(Armor(*target) - 100.f) < 0.01f,
				"without Armor Rend the spend leaves Armor untouched");
		}

		// Hair Trigger: the side threshold applies to Normal targets only; Elite and boss targets keep the main threshold of four.
		void HairChecks(Report& report, Application& application)
		{
			{
				Fixture f{ report, application };
				Wire(f);
				const auto gauge = Enemy(f);
				gauge->SetActorLocation({ 30000.f, 30000.f });
				const float baseline = Hit(*gauge, f.ship.get(), 10.f, Plain());
				f.Install(kKineticHost, 1, 1, kHairSide);
				const auto& consume = RuleStatusOf(f).kineticConsumes;
				report.Check(consume.size() == 1 && consume[0].normalTargetThreshold == 3 && consume[0].stackThreshold == 4,
					"Hair Trigger merges into the single Kinetic consume rule");
				const auto target = Enemy(f);
				target->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 100.f);
				KineticHit(*target, f.ship.get(), 1);
				KineticHit(*target, f.ship.get(), 1);
				Hit(*target, f.ship.get(), 1.f, Plain());
				report.Check(StackCount(*target, DamageStatusEffectIds::KineticEffectId) == 2, "a Normal target keeps two stacks below the side threshold");
				KineticHit(*target, f.ship.get(), 1);
				const float spend = Hit(*target, f.ship.get(), 10.f, Plain());
				report.Check(std::abs(spend - baseline * 1.10f) < 0.05f && !HasEffect(*target, DamageStatusEffectIds::KineticEffectId),
					"a Normal target spends at three stacks", Json{ { "spend", spend }, { "expected", baseline * 1.10f } });
			}
			{
				Fixture f{ report, application };
				Wire(f);
				const auto gauge = Enemy(f);
				gauge->SetActorLocation({ 30000.f, 30000.f });
				const float baseline = Hit(*gauge, f.ship.get(), 10.f, Plain());
				f.Install(kKineticHost, 1, 1, kHairSide);
				const auto target = Enemy(f);
				target->SetControlTargetClass(ControlTargetClass::Elite);
				target->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 100.f);
				for (int hit = 0; hit < 3; ++hit) KineticHit(*target, f.ship.get(), 1);
				Hit(*target, f.ship.get(), 1.f, Plain());
				report.Check(StackCount(*target, DamageStatusEffectIds::KineticEffectId) == 3, "an Elite target does not spend at three stacks");
				KineticHit(*target, f.ship.get(), 1);
				const float spend = Hit(*target, f.ship.get(), 10.f, Plain());
				report.Check(std::abs(spend - baseline * 1.10f) < 0.05f && !HasEffect(*target, DamageStatusEffectIds::KineticEffectId),
					"an Elite target spends at the main threshold of four", Json{ { "spend", spend } });
			}
		}

		// Afterglow: the side linger merges into the one sonic rule; without it the rule keeps no linger.
		void AfterglowChecks(Report& report, Application& application)
		{
			{
				Fixture f{ report, application };
				Wire(f);
				f.Install(kSustainHost, 1, 1);
				const auto& rules = f.ship->GetModuleApplier().GetResolvedEffects().sustainedSpeedRules;
				report.Check(rules.size() == 1 && rules[0].rule.lingerSeconds == 0.f, "without Afterglow the sonic rule has no linger", Json{ { "rules", rules.size() } });
			}
			{
				Fixture f{ report, application };
				Wire(f);
				f.Install(kSustainHost, 1, 1, kAfterglowSide);
				const auto& rules = f.ship->GetModuleApplier().GetResolvedEffects().sustainedSpeedRules;
				const bool merged = rules.size() == 1 && std::abs(rules[0].rule.lingerSeconds - 1.5f) < 1e-4f && std::abs(rules[0].rule.speedFraction - 0.9f) < 1e-4f &&
					std::abs(rules[0].rule.primarySpeedMultiplier - 2.f) < 1e-4f && std::abs(rules[0].rule.primaryPierceBonus - 1.f) < 1e-4f;
				report.Check(merged, "Afterglow merges lingerSeconds into the one sonic rule without a second rule", Json{ { "rules", rules.size() } });
			}
		}

		// Side-feature loader: the modifier-only forms load, every damage-carrying, empty or zero form is rejected.
		void LoaderChecks(Report& report, const std::filesystem::path& root, const Json& shipped)
		{
			const auto rejectSide = [&](const char* name, Json rules)
			{
				const auto path = WriteJson(root, std::string{ "side-reject-" } + name + ".json",
					SideDocument(Json::array({ SideFeature("Module.SideFeature.RareModTest.Reject", std::move(rules)) })));
				const auto loaded = content::ModuleSideFeatureLoader::LoadFromFile(path);
				report.Check(!loaded.Succeeded(), std::string{ "side modifier loader rejects " } + name, loaded.error);
			};
			rejectSide("thermal-damage", Json{ { "statusRules", Json{ { "thermalBursts", Json::array({ Json{ { "stackThreshold", 4 }, { "damageFraction", 1.3 } } }) } } } });
			rejectSide("thermal-empty", Json{ { "statusRules", Json{ { "thermalBursts", Json::array({ Json::object() }) } } } });
			rejectSide("smolder-without-radius", Json{ { "statusRules", Json{ { "thermalBursts", Json::array({ Json{ { "smolderSeconds", 3.0 } } }) } } } });
			rejectSide("smolder-stacks-above-cap", Json{ { "statusRules", Json{ { "thermalBursts", Json::array({ Json{ { "smolderSeconds", 3.0 }, { "smolderRadius", 150.0 }, { "smolderStacksPerSecond", 5 } } }) } } } });
			rejectSide("stacks-without-seconds", Json{ { "statusRules", Json{ { "thermalBursts", Json::array({ Json{ { "areaRadius", 200.0 }, { "areaDamageFraction", 0.6 }, { "smolderStacksPerSecond", 1 } } }) } } } });
			rejectSide("kinetic-multiplier", Json{ { "statusRules", Json{ { "kineticConsumes", Json::array({ Json{ { "damageMultiplier", 1.1 } } }) } } } });
			rejectSide("kinetic-empty", Json{ { "statusRules", Json{ { "kineticConsumes", Json::array({ Json::object() }) } } } });
			rejectSide("sustained-speed", Json{ { "sustainedSpeedRules", Json::array({ Json{ { "speedFraction", 0.9 }, { "holdSeconds", 2.0 }, { "primarySpeedMultiplier", 2.0 } } }) } });
			rejectSide("sustained-zero-linger", Json{ { "sustainedSpeedRules", Json::array({ Json{ { "lingerSeconds", 0.0 } } }) } });

			// The main package keeps its strict rules: a modifier-only entry is not a burst, a Armor Rend or a sonic state.
			const auto rejectModule = [&](const char* name, const Json& variantRules)
			{
				Json modules = shipped;
				modules["modules"].push_back(Host(kRejectHost, "Ability.Offense", variantRules, {}));
				const auto path = WriteJson(root, std::string{ "module-reject-" } + name + ".json", modules);
				const auto loaded = content::ModuleLoader::LoadFromFile(path);
				report.Check(!loaded.Succeeded(), std::string{ "main rule loader still rejects " } + name, loaded.error);
			};
			rejectModule("thermal-area-only", Json{ { "statusRules", Json{ { "thermalBursts", Json::array({ Json{ { "areaRadius", 200.0 }, { "areaDamageFraction", 0.6 } } }) } } } });
			rejectModule("kinetic-threshold-only", Json{ { "statusRules", Json{ { "kineticConsumes", Json::array({ Json{ { "normalTargetThreshold", 3 } } }) } } } });
			rejectModule("sustained-linger-only", Json{ { "sustainedSpeedRules", Json::array({ Json{ { "lingerSeconds", 1.5 } } }) } });
		}
	}
}

int main(int argc, char** argv)
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	using namespace rare_status_modifiers_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare status modifier content registration failed" };
		std::string title = "Rare Status Modifiers E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);

		const std::filesystem::path root = std::filesystem::path{ argv[1] }.parent_path() / "rare-status-modifier-fixtures";
		Json shipped;
		{
			std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json" };
			input >> shipped;
		}
		const auto sidePath = WriteJson(root, "rare-status-modifier-side-features.json", SideDocument(ValidSideFeatures()));
		const auto sideLoaded = content::ModuleSideFeatureLoader::LoadFromFile(sidePath);
		report.Check(sideLoaded.Succeeded() && sideLoaded.definitions.size() == 5, "fixture side modifiers load through the real side-feature loader", sideLoaded.error);
		gFixtureSideFeatures = sideLoaded.definitions;
		const auto fixturePath = WriteJson(root, "rare-status-modifier-modules.json", FixtureSet(shipped));
		const auto loaded = content::ModuleLoader::LoadFromFile(fixturePath);
		report.Check(loaded.Succeeded(), "fixture hosts load through the real ModuleLoader", loaded.error);
		gFixtureDefinitions = loaded.definitions;

		PrecheckDoT(report);
		AreaChecks(report, application);
		SmolderChecks(report, application);
		RendChecks(report, application);
		HairChecks(report, application);
		AfterglowChecks(report, application);
		LoaderChecks(report, root, shipped);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareStatusModifiersE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
