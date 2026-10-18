#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"

#include "attributes/AttributeMath.h"
#include "gameplay/ShieldComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/module/ModuleResolver.h"
#include "presentation/hud/modules/ModuleScreenPresenter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace rare_single_e2e
{
	using namespace uncommon_p4_e2e;
	using Json = common_module_e2e::Json;
	using ShieldMode = ShieldModuleBehavior::RegenMode;

	namespace
	{
		// Shipped content only. Nothing here is copied, reduced or re-parsed from a fixture file.
		constexpr const char* kOverclocker = "Module.Utility.ZoneOverclocker.Rare";
		constexpr const char* kMisfit = "Module.Utility.MisfitWiring.Rare";
		constexpr const char* kMomentum = "Module.Movement.MomentumShield.Rare";
		constexpr const char* kField = "Module.Defense.ContinuousFieldGenerator.Rare";
		constexpr const char* kRedirector = "Module.Defense.ImpactRedirector.Rare";
		// Shipped one-cell Utility module with one Owner modifier; its zone bonus reads back through the ship's attribute.
		constexpr const char* kSubject = "Module.Utility.TargetingComputer.Basic";
		// Shipped Uncommon Utility module, used as the regression probe for an unchanged Uncommon module.
		constexpr const char* kUncommon = "Module.Utility.DoubleFeedCore.Uncommon";

		constexpr std::array<float, 3> kAmplifierBonus{ 0.50f, 0.55f, 0.60f };
		constexpr std::array<float, 3> kMisfitBonus{ 0.25f, 0.30f, 0.35f };
		constexpr std::array<float, 3> kSpeedMultiplier{ 0.5f, 0.7f, 0.9f };
		constexpr std::array<float, 3> kContinuousRate{ 0.30f, 0.45f, 0.55f };
		constexpr std::array<float, 3> kArmorShare{ 0.5f, 0.6f, 0.7f };

		sas::ContentId ContentIdOf(const char* id)
		{
			return sas::ContentId{ std::string{ id } };
		}

		// Level value of the first Owner modifier of a shipped module, read from its own JSON-loaded definition.
		float BaseMagnitude(const ModuleDefinition& definition, int level)
		{
			const ModuleAttributeModifier& entry = definition.sizeVariants[0].attributeModifiers[0];
			return entry.magnitudeByLevel.ValueAtLevel(entry.modifier.magnitude, level);
		}

		sas::AttributeId FirstAttribute(const ModuleDefinition& definition)
		{
			return definition.sizeVariants[0].attributeModifiers[0].modifier.attributeId;
		}

		// Installs through the real loadout on the first legal placement whose covered cells all lie in the zone.
		std::uint32_t InstallInZone(Fixture& f, const char* moduleId, int level, const GameplayTag& zone)
		{
			const ModuleDefinition* definition = f.rules.lookup(moduleId);
			f.report.Check(definition != nullptr, std::string{ "catalog contains " } + moduleId);
			const std::uint32_t instance = f.loadout.AddToInventory(f.rules, *definition, 0, definition->rarity, level, sas::ContentId{ "" });
			f.report.Check(instance != 0, std::string{ "inventory accepts " } + moduleId);
			ModulePlacement candidate = *f.loadout.FindInventoryItem(instance);
			for (int rotation = 0; rotation < 4; ++rotation)
				for (int y = 0; y < f.rules.schematic.height; ++y)
					for (int x = 0; x < f.rules.schematic.width; ++x)
					{
						candidate.origin = { x, y };
						candidate.rotationQuarterTurns = rotation;
						if (ModulePlacementRules::CanPlace(f.rules, f.loadout, candidate) != ModulePlacementFailure::None) continue;
						bool inZone = true;
						for (const sf::Vector2i& cell : ModulePlacementRules::GetCoveredCells(*definition, candidate))
							inZone = inZone && f.rules.schematic.GetZoneAt(cell.x, cell.y) == zone;
						if (!inZone) continue;
						f.report.Check(f.loadout.Install(f.rules, instance, candidate.origin, rotation) == ModulePlacementFailure::None &&
							f.ship->GetModuleApplier().Apply(f.loadout, f.rules), std::string{ "zone install through the real applier: " } + moduleId);
						return instance;
					}
			throw std::runtime_error{ std::string{ "no legal zone placement for " } + moduleId };
		}

		// Adds a copy to the inventory and asks for a placement on a cell that is otherwise legal. The conflict rule
		// must refuse it, so the returned failure is the real loadout's answer for that placement.
		ModulePlacementFailure ProbeConflict(Fixture& f, const char* moduleId)
		{
			const ModuleDefinition* definition = f.rules.lookup(moduleId);
			f.report.Check(definition != nullptr, std::string{ "catalog contains " } + moduleId);
			const std::uint32_t instance = f.loadout.AddToInventory(f.rules, *definition, 0, definition->rarity, 1, sas::ContentId{ "" });
			f.report.Check(instance != 0, std::string{ "inventory accepts " } + moduleId);
			ModulePlacement candidate = *f.loadout.FindInventoryItem(instance);
			for (int rotation = 0; rotation < 4; ++rotation)
				for (int y = 0; y < f.rules.schematic.height; ++y)
					for (int x = 0; x < f.rules.schematic.width; ++x)
					{
						candidate.origin = { x, y };
						candidate.rotationQuarterTurns = rotation;
						if (ModulePlacementRules::CanPlace(f.rules, f.loadout, candidate) == ModulePlacementFailure::ModuleConflict)
							return f.loadout.Install(f.rules, instance, candidate.origin, rotation);
					}
			throw std::runtime_error{ std::string{ "no conflicting placement found for " } + moduleId };
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

		// Expected hull loss for 30 source damage into a 10-point shield with share s: the shield's damage factor is
		// 1 - s * (1 - m), and Armor is applied once to what passes the shield (same model as RareShieldRulesE2E).
		float ExpectedHullLoss(float armor, float share)
		{
			const float m = sas::AttributeMath::GetArmorDamageMultiplier(armor);
			const float factor = 1.f - share * (1.f - m);
			const float absorbedSource = std::min(30.f, 10.f / factor);
			return (30.f - absorbedSource) * m;
		}

		void CatalogChecks(Report& report)
		{
			const auto loaded = content::ModuleLoader::LoadFromFile(
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json");
			report.Check(loaded.Succeeded(), "shipped modules.json loads through the real ModuleLoader", loaded.error);

			struct Subject { const char* id; const char* family; std::size_t cells; };
			const Subject subjects[] = {
				{ kOverclocker, "Ability.Utility", 1 },
				{ kMisfit, "Ability.Utility", 1 },
				{ kMomentum, "Ability.Movement", 2 },
				{ kField, "Ability.Defense", 3 },
				{ kRedirector, "Ability.Defense", 3 },
			};
			for (const Subject& subject : subjects)
			{
				const ModuleDefinition* definition = content::ModuleContentCatalog::FindById(ContentIdOf(subject.id));
				report.Check(definition != nullptr, std::string{ "catalog contains " } + subject.id);
				report.Check(definition->rarity == ModuleRarity::Rare && definition->levelCount == 3 && definition->family.name == subject.family &&
					definition->sizeVariants.size() == 1, std::string{ "Rare, three levels, one size variant: " } + subject.id,
					Json{ { "levelCount", definition->levelCount }, { "variants", definition->sizeVariants.size() } });
				report.Check(definition->sizeVariants[0].shape.cells.size() == subject.cells, std::string{ "cell count matches the design: " } + subject.id,
					Json{ { "cells", definition->sizeVariants[0].shape.cells.size() } });
				report.Check(definition->sideFeatures.empty() && definition->sharedPools.empty(),
					std::string{ "single-size module has no side features or shared pools: " } + subject.id);
				for (int level = 1; level <= 3; ++level)
					report.Check(HasModuleRulePackageLevelCoverage(definition->sizeVariants[0], level),
						std::string{ "level table covers level " } + std::to_string(level) + ": " + subject.id);
				report.Check(!HasModuleRulePackageLevelCoverage(definition->sizeVariants[0], 4), std::string{ "no level beyond three: " } + subject.id);
			}

			const ModuleDefinition* momentum = content::ModuleContentCatalog::FindById(ContentIdOf(kMomentum));
			const ModuleDefinition* field = content::ModuleContentCatalog::FindById(ContentIdOf(kField));
			report.Check(momentum->conflictsWith.size() == 1 && momentum->conflictsWith[0] == ContentIdOf(kField), "Momentum Shield lists Continuous Field Generator as a conflict");
			report.Check(field->conflictsWith.size() == 1 && field->conflictsWith[0] == ContentIdOf(kMomentum), "Continuous Field Generator lists Momentum Shield as a conflict");
		}

		void DescriptionChecks(Report& report)
		{
			struct Text { const char* id; std::array<const char*, 3> expected; };
			const Text texts[] = {
				{ kOverclocker, { "(+50%)", "(+55%)", "(+60%)" } },
				{ kMisfit, { "(+25%", "(+30%", "(+35%" } },
				{ kMomentum, { "0,5x", "0,7x", "0,9x" } },
				{ kField, { "at 30%", "at 45%", "at 55%" } },
				{ kRedirector, { "(50%", "(60%", "(70%" } },
			};
			for (const Text& text : texts)
			{
				const ModuleDefinition* definition = content::ModuleContentCatalog::FindById(ContentIdOf(text.id));
				report.Check(definition != nullptr, std::string{ "catalog contains " } + text.id);
				for (int level = 1; level <= 3; ++level)
				{
					const std::string formatted = FormatModuleDescription(*definition, definition->sizeVariants[0], level);
					const std::string label = std::string{ "description level " } + std::to_string(level) + " is fully formatted: " + text.id;
					report.Check(formatted.find('{') == std::string::npos && formatted.find('}') == std::string::npos, label, Json{ { "text", formatted } });
					report.Check(formatted.find(text.expected[level - 1]) != std::string::npos,
						std::string{ "description shows the level " } + std::to_string(level) + " value: " + text.id, Json{ { "text", formatted } });
				}
			}
		}

		void OverclockerChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const GameplayTag& sensor = GameplayTags::Module::Zone::Sensor;
			const ModuleDefinition* subject = f.rules.lookup(kSubject);
			const sas::AttributeId attribute = FirstAttribute(*subject);
			const float base = BaseMagnitude(*subject, 1);
			const float before = f.Owner(attribute);
			InstallInZone(f, kSubject, 1, sensor);
			report.CheckValue("zone overclocker: a Sensor module keeps the standard 0.25 bonus with no amplifier", base * 1.25f,
				f.Owner(attribute) - before, "Utility probe in Sensor on the real Fighter");
			for (int level = 1; level <= 3; ++level)
			{
				const std::uint32_t amplifier = InstallInZone(f, kOverclocker, level, sensor);
				report.CheckValue("zone overclocker: level " + std::to_string(level) + " raises the other Sensor module",
					base * (1.f + kAmplifierBonus[level - 1]), f.Owner(attribute) - before, "Zone Overclocker in the same zone");
				f.Remove(amplifier);
				report.CheckValue("zone overclocker: removing it returns the Sensor module to 0.25", base * 1.25f,
					f.Owner(attribute) - before, "Zone Overclocker removed through the real loadout");
			}
		}

		void MisfitChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const GameplayTag& wings = GameplayTags::Module::Zone::Wings;
			const GameplayTag& sensor = GameplayTags::Module::Zone::Sensor;
			const ModuleDefinition* subject = f.rules.lookup(kSubject);
			const sas::AttributeId attribute = FirstAttribute(*subject);
			const float base = BaseMagnitude(*subject, 1);
			const float before = f.Owner(attribute);
			InstallInZone(f, kSubject, 1, wings);
			report.CheckValue("misfit: an out-of-family module has no zone bonus without Misfit Wiring", base,
				f.Owner(attribute) - before, "Utility probe in Offense Wings");
			for (int level = 1; level <= 3; ++level)
			{
				const std::uint32_t misfit = InstallInZone(f, kMisfit, level, sensor);
				report.CheckValue("misfit: level " + std::to_string(level) + " gives the out-of-family module its main bonus",
					base * (1.f + kMisfitBonus[level - 1]), f.Owner(attribute) - before, "Misfit Wiring installed elsewhere on the ship");
				f.Remove(misfit);
				report.CheckValue("misfit: removing Misfit Wiring restores the out-of-family module", base,
					f.Owner(attribute) - before, "Misfit Wiring removed through the real loadout");
			}
		}

		void MomentumChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			ShieldComponent& shield = f.ship->GetShieldComponent();
			const float maxShield = shield.GetMaxShield();
			const float normal = NormalRegen(*f.ship);
			report.Check(maxShield > 20.f && normal > 0.f, "precondition: the Fighter shield has capacity and passive regen",
				Json{ { "maxShield", maxShield }, { "normalRegen", normal } });
			// LegacyVelocity resets velocity every tick; ThrustDrift keeps the velocity the test sets.
			f.ship->SetMovementMode(ShipMovementMode::ThrustDrift);

			for (int level = 1; level <= 3; ++level)
			{
				const std::uint32_t instance = f.Install(kMomentum, 0, level);
				const auto& behavior = shield.GetModuleBehavior();
				report.Check(behavior.regenMode == ShieldMode::SpeedScaled, "Momentum Shield switches the live shield to speed-scaled regen");
				report.CheckValue("momentum: level " + std::to_string(level) + " top-speed multiplier", kSpeedMultiplier[level - 1],
					behavior.speedMultiplierAtMax, "shieldRules table through the real applier");

				SetShield(*f.ship, 0.5f * maxShield);
				f.ship->SetVelocity({ 0.f, 0.f });
				float before = shield.GetShield();
				Advance(f, 1.f);
				report.Check(std::abs(shield.GetShield() - before) < 1e-4f, "momentum: a stationary ship gets no passive regen",
					Json{ { "gain", shield.GetShield() - before } });

				f.ship->SetVelocity({ 199.f, 0.f });
				before = shield.GetShield();
				Advance(f, 0.1f);
				report.Check(std::abs(shield.GetShield() - before) < 1e-4f, "momentum: below 200 speed grants no recharge", Json{ { "gain", shield.GetShield() - before } });
				f.ship->SetVelocity({ 400.f, 0.f });
				before = shield.GetShield();
				Advance(f, 0.1f);
				report.Check(shield.GetShield() > before, "momentum: above 200 speed starts recharge", Json{ { "speed", Speed(*f.ship) }, { "gain", shield.GetShield() - before } });

				// Far above the cap the speed ratio clamps to 1, so the gain is the normal rate times the level multiplier.
				SetShield(*f.ship, 0.5f * maxShield);
				f.ship->SetVelocity({ 5000.f, 0.f });
				before = shield.GetShield();
				Advance(f, 1.f);
				const float topGain = shield.GetShield() - before;
				const float expectedTop = normal * kSpeedMultiplier[level - 1];
				report.Check(std::abs(topGain - expectedTop) <= 0.01f * expectedTop + 1e-4f,
					"momentum: top speed regen is the normal rate times the level multiplier",
					Json{ { "gain", topGain }, { "expected", expectedTop } });

				auto enemy = Enemy(f);
				Hit(*f.ship, enemy.get(), 10.f, Plain());
				f.ship->SetVelocity({ 5000.f, 0.f });
				before = shield.GetShield();
				Advance(f, 0.5f);
				report.Check(shield.GetShield() > before && shield.GetRechargeDelayRemaining() == 0.f,
					"momentum: regen runs on the first tick after damage with no recharge delay",
					Json{ { "gain", shield.GetShield() - before }, { "delayRemaining", shield.GetRechargeDelayRemaining() } });
				f.Remove(instance);
				report.Check(shield.GetModuleBehavior().regenMode == ShieldMode::Normal, "removing Momentum Shield restores normal regen");
			}
		}

		void ContinuousChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			ShieldComponent& shield = f.ship->GetShieldComponent();
			const float normal = NormalRegen(*f.ship);
			auto enemy = Enemy(f);

			for (int level = 1; level <= 3; ++level)
			{
				const std::uint32_t instance = f.Install(kField, 0, level);
				const auto& behavior = shield.GetModuleBehavior();
				report.Check(behavior.regenMode == ShieldMode::Continuous && std::abs(behavior.breakRearmFraction - 0.25f) < 1e-4f,
					"Continuous Field Generator switches the live shield to continuous regen with the 25 percent re-arm");
				report.CheckValue("continuous: level " + std::to_string(level) + " rate fraction", kContinuousRate[level - 1],
					behavior.continuousRateFraction, "shieldRules table through the real applier");

				SetShield(*f.ship, 0.5f * shield.GetMaxShield());
				Hit(*f.ship, enemy.get(), 10.f, Plain());
				const float before = shield.GetShield();
				Advance(f, 1.f);
				const float gain = shield.GetShield() - before;
				const float expected = normal * kContinuousRate[level - 1];
				report.Check(normal > 0.f && std::abs(gain - expected) <= 0.01f * expected + 1e-4f && shield.GetRechargeDelayRemaining() == 0.f,
					"continuous: regen runs while damaged at the level rate with no delay",
					Json{ { "gain", gain }, { "expected", expected }, { "delayRemaining", shield.GetRechargeDelayRemaining() } });
				f.Remove(instance);
			}
		}

		void ConflictChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			ShieldComponent& shield = f.ship->GetShieldComponent();

			const std::uint32_t momentum = f.Install(kMomentum, 0, 1);
			report.Check(ProbeConflict(f, kField) == ModulePlacementFailure::ModuleConflict,
				"Continuous Field Generator is refused while Momentum Shield is installed");
			f.Remove(momentum);

			const std::uint32_t field = f.Install(kField, 0, 1);
			report.Check(ProbeConflict(f, kMomentum) == ModulePlacementFailure::ModuleConflict,
				"Momentum Shield is refused while Continuous Field Generator is installed");
			report.Check(shield.GetModuleBehavior().regenMode == ShieldMode::Continuous, "the refused module leaves the installed shield rule alone");
			f.Remove(field);

			f.Install(kMomentum, 0, 1);
			report.Check(shield.GetModuleBehavior().regenMode == ShieldMode::SpeedScaled, "with the conflict removed Momentum Shield installs and applies");
		}

		void ArmorChecks(Report& report, Application& application)
		{
			struct Outcome { float armor = 0.f; float loss = 0.f; float share = 0.f; float shieldLeft = 0.f; };
			const auto armorCase = [&report, &application](int level) -> Outcome
			{
				Fixture f{ report, application };
				if (level > 0) f.Install(kRedirector, 0, level);
				f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 100.f);
				Outcome outcome;
				outcome.armor = f.Owner(OwnerAttributeIds::Armor);
				outcome.share = f.ship->GetShieldComponent().GetModuleBehavior().armorShare;
				f.ship->GetHealthComponent().SetMaxHealth(1000.f);
				f.ship->GetHealthComponent().Regenerate(1000.f);
				SetShield(*f.ship, 10.f);
				auto enemy = Enemy(f);
				const float healthBefore = f.ship->GetHealthComponent().GetHealth();
				Hit(*f.ship, enemy.get(), 30.f, Plain());
				outcome.loss = healthBefore - f.ship->GetHealthComponent().GetHealth();
				outcome.shieldLeft = f.ship->GetShieldComponent().GetShield();
				return outcome;
			};

			const Outcome none = armorCase(0);
			report.Check(none.armor > 0.f, "precondition: the Fighter has positive Armor", Json{ { "armor", none.armor } });
			report.Check(std::abs(none.loss - ExpectedHullLoss(none.armor, 0.f)) < 0.01f, "without Impact Redirector the shield takes no Armor share",
				Json{ { "loss", none.loss } });
			for (int level = 1; level <= 3; ++level)
			{
				const Outcome outcome = armorCase(level);
				report.CheckValue("impact redirector: level " + std::to_string(level) + " armor share", kArmorShare[level - 1], outcome.share,
					"shieldRules table through the real applier");
				report.Check(std::abs(outcome.loss - ExpectedHullLoss(outcome.armor, kArmorShare[level - 1])) < 0.01f && outcome.loss < none.loss,
					"impact redirector: the same hit costs less hull when Armor shares the shield load",
					Json{ { "level", level }, { "loss", outcome.loss }, { "withoutModule", none.loss } });
				report.Check(std::abs(outcome.shieldLeft) < 1e-4f && std::abs(none.shieldLeft) < 1e-4f,
					"impact redirector: both cases spend the whole 10-point shield on 30 damage");
			}
		}

		void HandleChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const std::size_t baselineHandles = f.ship->GetModuleApplier().GetOwnedHandleCount();
			const std::size_t baselineModifiers = f.ship->GetModuleTriggerRuntime().GetActiveModifierHandleCount();
			for (int cycle = 0; cycle < 10; ++cycle)
			{
				const int level = 1 + cycle % 3;
				const std::uint32_t overclocker = f.Install(kOverclocker, 0, level);
				const std::uint32_t misfit = f.Install(kMisfit, 0, level);
				const std::uint32_t redirector = f.Install(kRedirector, 0, level);
				const std::uint32_t shield = f.Install(cycle % 2 == 0 ? kMomentum : kField, 0, level);
				f.Remove(shield);
				f.Remove(redirector);
				f.Remove(misfit);
				f.Remove(overclocker);
				report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == baselineHandles &&
					f.ship->GetModuleTriggerRuntime().GetActiveModifierHandleCount() == baselineModifiers,
					"ten install and remove cycles of all five Rare modules leak no handles", Json{ { "cycle", cycle } });
			}
		}

		void RegressionChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const ModuleDefinition* uncommon = f.rules.lookup(kUncommon);
			const sas::AttributeId attribute = FirstAttribute(*uncommon);
			const float before = f.Owner(attribute);
			const std::uint32_t instance = f.Install(kUncommon, 0, 1);
			const ModulePlacement placement = *f.loadout.FindPlacement(instance);
			const float zoneBonus = ModuleResolver::GetZoneBonus(f.rules, *uncommon, placement);
			report.CheckValue("regression: the shipped Uncommon module scales by its own zone bonus only",
				BaseMagnitude(*uncommon, 1) * (1.f + zoneBonus), f.Owner(attribute) - before, "Double Feed Core, no Rare module installed");
			f.Remove(instance);
			report.CheckValue("regression: removing the Uncommon module restores its attribute", 0.f,
				f.Owner(attribute) - before, "Double Feed Core removed");
		}
	}
}

class AfterburningRegenProbe final : public vega::PlayerSpaceShip
{
public:
	AfterburningRegenProbe(vega::World* world, const ShipDefinition& definition) : PlayerSpaceShip{ world, definition } {}
	bool IsAfterburnerRechargeBlocked() const override { return true; }
};

void AfterburnerMomentumChecks(uncommon_p4_e2e::Report& report, vega::Application& application)
{
	using namespace uncommon_p4_e2e;
	Fixture f{ report, application };
	auto ship = f.level->SpawnActor<AfterburningRegenProbe>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock();
	f.level->TickInternal(0.f);
	ship->SetMovementMode(ShipMovementMode::ThrustDrift);
	ship->SetInvulnerability(false);
	ship->GetShieldComponent().ChangeShield(-ship->GetShieldComponent().GetMaxShield() * 0.5f);
	const float before = ship->GetShieldComponent().GetShield();
	ship->SetVelocity({ 5000.f, 0.f });
	ship->Tick(0.1f);
	report.CheckValue("normal shield recharge remains blocked during Afterburner", before, ship->GetShieldComponent().GetShield(), "actual SpaceShip regeneration with recharge-blocked owner");
	f.Install("Module.Movement.MomentumShield.Rare");
	report.Check(ship->GetModuleApplier().Apply(f.loadout, f.rules), "afterburning ship receives real Momentum Shield content");
	ship->SetVelocity({ 5000.f, 0.f });
	ship->Tick(0.1f);
	report.Check(ship->GetShieldComponent().GetShield() > before, "Momentum Shield recharges while Afterburner blocks normal regeneration", Json{ { "gain", ship->GetShieldComponent().GetShield() - before } });
}

int main(int argc, char** argv)
{
	vega::InstallVegaEngineAssets();
	using namespace rare_single_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare single module content registration failed" };
		std::string title = "Rare Single Modules E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		CatalogChecks(report);
		DescriptionChecks(report);
		OverclockerChecks(report, application);
		MisfitChecks(report, application);
		MomentumChecks(report, application);
		AfterburnerMomentumChecks(report, application);
		ContinuousChecks(report, application);
		ConflictChecks(report, application);
		ArmorChecks(report, application);
		HandleChecks(report, application);
		RegressionChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareSingleModulesE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
