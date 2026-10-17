#include "gameConfigs/CollisionLayers.h"
#include "gameFramework/LightYearsAssets.h"
#include "CommonModuleE2ETestSupport.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"

#include <functional>
#include <string>

namespace
{
	using namespace ly;
	using namespace common_module_e2e;

	constexpr float kHuntClockLevelOneReduction = .05f;

	struct Fixture
	{
		Report& report;
		shared_ptr<HeadlessModuleLevel> level;
		shared_ptr<PlayerSpaceShip> ship;
		ModuleRulesContext rules;
		ModuleLoadout loadout;
		Fixture(Report& result, Application& application) : report{ result }, level{ std::make_shared<HeadlessModuleLevel>(&application) }, rules{ *content::ShipContentCatalog::GetPlayerFighterDefinition().schematic, [](const sas::ContentId& id) { return content::ModuleContentCatalog::FindById(id); }, content::ModuleContentCatalog::GetZoneTable(), [](const sas::ContentId& id) { return content::ModuleSideFeatureCatalog::FindById(id); } }
		{
			level->BeginPlayInternal();
			const auto& definition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
			ship = level->SpawnActor<PlayerSpaceShip>(definition).lock();
			report.Check(ship != nullptr, "charge test spawns a real player ship");
			level->TickInternal(0.f);
			ship->SetInvulnerability(false);
			ship->SetUseScreenClamp(false);
			ship->SetActorLocation({ 10000.f, 10000.f });
			report.Check(loadout.SeedDefaults(rules) && ship->GetModuleApplier().Apply(loadout, rules), "charge test uses canonical default modules and runtime applier");
		}
		std::uint32_t Install(const char* id, int variant, int levelValue)
		{
			const auto* definition = rules.lookup(id);
			report.Check(definition != nullptr, std::string{ "charge test shipped module: " } + id);
			const auto instance = loadout.AddToInventory(rules, *definition, variant, definition->rarity, levelValue, sas::ContentId{ "" });
			report.Check(instance != 0, "charge test inventory validates variant and level");
			ModulePlacement candidate = *loadout.FindInventoryItem(instance);
			for (int rotation = 0; rotation < 4; ++rotation)
				for (int y = 0; y < rules.schematic.height; ++y)
					for (int x = 0; x < rules.schematic.width; ++x)
					{
						candidate.origin = { x, y };
						candidate.rotationQuarterTurns = rotation;
						if (ModulePlacementRules::CanPlace(rules, loadout, candidate) != ModulePlacementFailure::None) continue;
						report.Check(loadout.Install(rules, instance, candidate.origin, rotation) == ModulePlacementFailure::None && ship->GetModuleApplier().Apply(loadout, rules), "charge test installs through real placement and applier");
						return instance;
					}
			throw std::runtime_error{ "No legal charge test placement" };
		}
		void Kill()
		{
			const auto& definition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
			const auto enemy = level->SpawnActor<SpaceShip>(definition).lock();
			report.Check(enemy != nullptr, "charge test creates a real enemy combatant");
			level->TickInternal(0.f);
			enemy->SetCollisionLayer(ly::GameCollision::Enemy);
			enemy->SetActorLocation({ 20000.f, 20000.f });
			DamagePayload payload;
			payload.criticalPolicy = DamageCriticalPolicy::Disabled;
			payload.kineticStacks = 0;
			ApplyCombatDamage(*enemy, 100000.f, ship.get(), {}, payload);
			report.Check(enemy->GetHealthComponent().GetHealth() == 0.f, "charge test confirms a real kill through damage resolution");
		}
	};

	// Uses the shipped Ability3 loadout slot unchanged. Every shipped ability has maxCharges 1; its own validator
	// rejects re-granting a modified copy, so the test must not alter the definition.
	GameAbility* GrantSingleChargeAbility(Report& report, Fixture& fixture)
	{
		auto& abilities = fixture.ship->GetAbilitySystemComponent();
		GameAbility* ability = abilities.GetAbility(sas::AbilitySlot::Ability3);
		report.Check(ability != nullptr, "charge test finds the real Ability3 loadout slot");
		report.Check(ability->GetDefinition().maxCharges == 1, "shipped Ability3 has exactly one charge", Json{ { "maxCharges", ability->GetDefinition().maxCharges } });
		return ability;
	}

	// Scenario 1a (regression guard): partial reduction, then ticking to zero restores the charge.
	void RestoreThroughTickChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		GameAbility* ability = GrantSingleChargeAbility(report, f);
		report.Check(ability->GetCharges() == 1, "one-charge ability starts with its charge");
		report.Check(ability->TryActivate(), "one-charge ability activates with its only charge");
		ability->Cancel(sas::AbilityEndReason::Completed);
		report.Check(!ability->IsActive() && ability->GetCharges() == 0, "activation spends the only charge");
		const float full = ability->GetCooldownRemaining();
		report.Check(full > 0.f, "cooldown starts when the ability ends", Json{ { "cooldownRemaining", full } });
		ability->ReduceCooldownRemaining(full * .5f);
		report.CheckValue("partial reduction leaves half of the remaining cooldown", full * .5f, ability->GetCooldownRemaining(), "ReduceCooldownRemaining(0.5 * remaining)");
		report.Check(ability->GetCharges() == 0 && !ability->TryActivate(), "partial reduction keeps the charge spent and the ability blocked");
		ability->Tick(full);
		report.Check(ability->GetCooldownRemaining() == 0.f, "tick drains the remaining cooldown to zero");
		report.Check(ability->GetCharges() == 1, "tick through zero restores the single charge");
		report.Check(ability->TryActivate(), "ability activates again after tick recovery");
		ability->Cancel(sas::AbilityEndReason::Completed);
	}

	// Scenario 1b (expected to fail today): reducing the remaining cooldown to zero must not strand the charge.
	void ReduceToZeroChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		GameAbility* ability = GrantSingleChargeAbility(report, f);
		report.Check(ability->TryActivate(), "one-charge ability activates before reduction");
		ability->Cancel(sas::AbilityEndReason::Completed);
		const float remaining = ability->GetCooldownRemaining();
		report.Check(ability->GetCharges() == 0 && remaining > 0.f, "spent ability has a running cooldown before reduction", Json{ { "cooldownRemaining", remaining }, { "charges", ability->GetCharges() } });
		ability->ReduceCooldownRemaining(remaining + 1.f);
		report.Check(ability->GetCooldownRemaining() == 0.f, "over-reduction clamps the cooldown to zero");
		report.Check(ability->GetCharges() == 1, "cooldown reaching zero by reduction restores the spent charge", Json{ { "charges", ability->GetCharges() } });
		report.Check(ability->TryActivate(), "ability is activatable after reduction clears its cooldown");
		ability->Cancel(sas::AbilityEndReason::Completed);
	}

	// Scenario 2: family Hunt Clock refunds five percent of even a very short remaining cooldown.
	void HuntClockKillChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.Install("Module.Offense.HuntClock.Uncommon", 1, 1);
		GameAbility* ability = GrantSingleChargeAbility(report, f);
		report.Check(ability->TryActivate(), "one-charge ability activates before Hunt Clock kill");
		ability->Cancel(sas::AbilityEndReason::Completed);
		const float full = ability->GetCooldownRemaining();
		ability->Tick(full - .1f);
		const float pending = ability->GetCooldownRemaining();
		report.Check(pending > 0.f && pending < .25f, "pending cooldown is short but positive", Json{ { "pendingCooldown", pending }, { "huntClockReductionFraction", kHuntClockLevelOneReduction } });
		report.Check(ability->GetCharges() == 0, "charge is still spent before the kill");
		f.Kill();
		report.CheckValue("Hunt Clock refunds five percent of the remaining cooldown", pending * (1.f - kHuntClockLevelOneReduction), ability->GetCooldownRemaining(), "remaining cooldown fraction");
		report.Check(ability->GetCharges() == 0, "percentage refund keeps the spent charge until cooldown completion");
		ability->Tick(ability->GetCooldownRemaining() + .01f);
		report.Check(ability->GetCharges() == 1, "finishing the refunded cooldown restores the spent charge", Json{ { "charges", ability->GetCharges() } });
		report.Check(ability->TryActivate(), "ability activates again after Hunt Clock kill");
		ability->Cancel(sas::AbilityEndReason::Completed);
	}

	// Runs one scenario block and records its outcome, so a failing block does not stop the next one.
	bool RunScenario(Json& scenarios, const char* name, const std::function<void()>& body)
	{
		Json entry{ { "name", name }, { "passed", false } };
		try
		{
			body();
			entry["passed"] = true;
		}
		catch (const std::exception& exception)
		{
			entry["error"] = exception.what();
			std::cerr << name << ": " << exception.what() << '\n';
		}
		scenarios.push_back(entry);
		return entry["passed"].get<bool>();
	}
}

int main(int argc, char** argv)
{
	ly::InstallLightYearsEngineAssets();
	if (argc != 2) return 2;
	Report report;
	report.artifact["test"] = "AbilityChargeRestoreE2E";
	report.artifact["inputs"] = Json{ { "abilitySlot", "Ability3 (shipped definition, unchanged)" },{ "huntClockLevelOneReduction", kHuntClockLevelOneReduction } };
	report.artifact["scenarios"] = Json::array();
	bool allPassed = true;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "charge restore content registration failed" };
		std::string title = "Ability Charge Restore E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		Json& scenarios = report.artifact["scenarios"];
		if (!RunScenario(scenarios, "1a ability-level: ticking through zero restores the charge (regression guard)", [&] { RestoreThroughTickChecks(report, application); })) allPassed = false;
		if (!RunScenario(scenarios, "1b ability-level: cooldown reduced to zero restores the charge", [&] { ReduceToZeroChecks(report, application); })) allPassed = false;
		if (!RunScenario(scenarios, "2 module-level: Hunt Clock kill on sub-reduction cooldown restores the charge", [&] { HuntClockKillChecks(report, application); })) allPassed = false;
	}
	catch (const std::exception& exception)
	{
		report.artifact["error"] = exception.what();
		std::cerr << exception.what() << '\n';
		allPassed = false;
	}
	report.artifact["passed"] = allPassed;
	WriteArtifact(argv[1], report.artifact);
	return allPassed ? 0 : 1;
}
