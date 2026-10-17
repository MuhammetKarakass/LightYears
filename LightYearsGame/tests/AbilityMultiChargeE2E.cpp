#include "gameFramework/LightYearsAssets.h"
#include "CommonModuleE2ETestSupport.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"

#include <cmath>
#include <functional>
#include <string>

namespace
{
	using namespace vega;
	using namespace common_module_e2e;

	constexpr sas::AbilitySlot kSlot = sas::AbilitySlot::Ability3;
	constexpr int kBonusUnderTest = 2;

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
			report.Check(ship != nullptr, "multi-charge test spawns a real player ship");
			level->TickInternal(0.f);
			ship->SetInvulnerability(false);
			ship->SetUseScreenClamp(false);
			ship->SetActorLocation({ 10000.f, 10000.f });
			report.Check(loadout.SeedDefaults(rules) && ship->GetModuleApplier().Apply(loadout, rules), "multi-charge test uses canonical default modules and runtime applier");
		}
	};

	// The shipped Ability3 keeps maxCharges 1. Extra capacity comes from the runtime charge bonus, so the
	// shipped definition is never altered (its validator rejects modified re-grants).
	GameAbility* GetAbility3(Report& report, Fixture& fixture)
	{
		GameAbility* ability = fixture.ship->GetAbilitySystemComponent().GetAbility(kSlot);
		report.Check(ability != nullptr, "multi-charge test finds the real Ability3 loadout slot");
		report.Check(ability->GetDefinition().maxCharges == 1, "shipped Ability3 definition keeps maxCharges 1", Json{ { "maxCharges", ability->GetDefinition().maxCharges } });
		return ability;
	}

	// One real use through TryActivate; ends it immediately, as the existing charge tests do.
	bool Spend(GameAbility* ability)
	{
		const bool activated = ability->TryActivate();
		ability->Cancel(sas::AbilityEndReason::Completed);
		return activated;
	}

	// Advances exactly the shipped anti-spam gap between two charge uses.
	void WaitUseGap(GameAbility* ability)
	{
		ability->Tick(ability->GetDefinition().chargeUseInterval);
	}

	bool Near(float actual, float expected, float tolerance = 1e-3f)
	{
		return std::abs(actual - expected) <= tolerance;
	}

	// Scenario 1: capacity 3 through the real bonus path; uses are gated by the anti-spam gap, not by the timer.
	void MultiChargeUseChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		GameAbility* ability = GetAbility3(report, f);
		auto& abilities = f.ship->GetAbilitySystemComponent();
		const float gap = ability->GetDefinition().chargeUseInterval;
		report.Check(gap > 0.f, "shipped definition carries a positive default charge use interval", Json{ { "chargeUseInterval", gap } });
		report.Check(abilities.SetAbilityChargeBonus(kSlot, kBonusUnderTest), "charge bonus is accepted for Ability3");
		report.Check(ability->GetChargeCapacity() == 3, "bonus raises effective capacity to 3", Json{ { "capacity", ability->GetChargeCapacity() } });
		report.Check(ability->BuildSnapshot().chargeCapacity == 3, "runtime snapshot carries the effective capacity", Json{ { "snapshotCapacity", ability->BuildSnapshot().chargeCapacity } });
		report.Check(ability->GetCharges() == 3, "charges added by the bonus arrive full", Json{ { "charges", ability->GetCharges() } });
		report.Check(ability->TryActivate(), "first use spends from a full pool");
		ability->Cancel(sas::AbilityEndReason::Completed);
		report.Check(ability->GetCharges() == 2 && ability->IsOnCooldown(), "first use leaves 2 charges and starts the single recharge timer", Json{ { "charges", ability->GetCharges() }, { "cooldownRemaining", ability->GetCooldownRemaining() } });
		const float timerAfterFirstUse = ability->GetCooldownRemaining();
		report.Check(!ability->TryActivate(), "immediate second use is blocked by the anti-spam gap even with charges left");
		report.Check(ability->GetCharges() == 2, "a blocked attempt spends no charge");
		ability->Tick(gap);
		report.Check(ability->TryActivate(), "second use is allowed once the gap elapses while the recharge timer still runs");
		ability->Cancel(sas::AbilityEndReason::Completed);
		report.Check(ability->GetCharges() == 1 && ability->IsOnCooldown(), "second use leaves 1 charge");
		report.Check(ability->GetCooldownRemaining() <= timerAfterFirstUse - gap + 1e-3f, "using a charge does not restart the single recharge timer", Json{ { "timerAfterFirstUse", timerAfterFirstUse }, { "timerNow", ability->GetCooldownRemaining() } });
		WaitUseGap(ability);
		report.Check(ability->TryActivate(), "third use is allowed after the gap");
		ability->Cancel(sas::AbilityEndReason::Completed);
		report.Check(ability->GetCharges() == 0 && ability->IsOnCooldown(), "third use empties the pool while the recharge timer keeps running");
		WaitUseGap(ability);
		report.Check(!ability->TryActivate(), "an empty pool blocks use even after the gap");
	}

	// Scenario 2: recharge restores one charge per completed cycle; the single timer restarts at the full duration.
	void RechargeCycleChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		GameAbility* ability = GetAbility3(report, f);
		f.ship->GetAbilitySystemComponent().SetAbilityChargeBonus(kSlot, kBonusUnderTest);
		report.Check(Spend(ability), "spend charge 1 of 3");
		WaitUseGap(ability);
		report.Check(Spend(ability), "spend charge 2 of 3");
		WaitUseGap(ability);
		report.Check(Spend(ability), "spend charge 3 of 3");
		report.Check(ability->GetCharges() == 0 && ability->IsOnCooldown(), "pool is empty with the recharge timer running");
		const float first = ability->GetCooldownRemaining();
		const float full = ability->GetCooldownDuration();
		report.Check(full > .5f && first > .5f, "shipped cooldown is long enough for the staged recharge checks", Json{ { "cooldownDuration", full }, { "firstCycleRemaining", first } });

		ability->Tick(first - .05f);
		report.Check(ability->GetCharges() == 0 && ability->GetCooldownRemaining() > 0.f && ability->GetCooldownRemaining() < .1f, "partial recharge restores no charge yet", Json{ { "cooldownRemaining", ability->GetCooldownRemaining() } });
		ability->Tick(.1f);
		report.Check(ability->GetCharges() == 1, "completing the first cycle restores exactly one charge", Json{ { "charges", ability->GetCharges() } });
		report.Check(ability->GetCooldownRemaining() > full - .1f && ability->GetCooldownRemaining() <= full, "timer restarts at the full duration with the overflow carried", Json{ { "cooldownRemaining", ability->GetCooldownRemaining() }, { "cooldownDuration", full } });

		ability->Tick(full + .1f);
		report.Check(ability->GetCharges() == 2, "one long tick completes exactly one cycle, not two", Json{ { "charges", ability->GetCharges() } });
		report.Check(ability->IsOnCooldown() && ability->GetCooldownRemaining() < full, "a missing charge keeps the timer running");

		const float last = ability->GetCooldownRemaining();
		ability->Tick(last);
		report.Check(ability->GetCharges() == 3 && !ability->IsOnCooldown(), "final cycle refills the pool and clears the timer", Json{ { "charges", ability->GetCharges() } });
		WaitUseGap(ability);
		report.Check(ability->TryActivate(), "fully recharged pool activates again");
		ability->Cancel(sas::AbilityEndReason::Completed);
	}

	// Scenario 3: cooldown reduction returns charges and carries overflow across cycles, through the real ASC path too.
	void ReduceCooldownChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		GameAbility* ability = GetAbility3(report, f);
		auto& abilities = f.ship->GetAbilitySystemComponent();
		abilities.SetAbilityChargeBonus(kSlot, kBonusUnderTest);
		report.Check(Spend(ability), "reduction check spends charge 1 of 3");
		WaitUseGap(ability);
		report.Check(Spend(ability), "reduction check spends charge 2 of 3");
		WaitUseGap(ability);
		report.Check(Spend(ability), "reduction check spends charge 3 of 3");
		const float first = ability->GetCooldownRemaining();
		const float full = ability->GetCooldownDuration();

		ability->ReduceCooldownRemaining(first);
		report.Check(ability->GetCharges() == 1 && Near(ability->GetCooldownRemaining(), full), "reducing the timer to zero restores one charge and restarts at the full duration", Json{ { "charges", ability->GetCharges() }, { "cooldownRemaining", ability->GetCooldownRemaining() } });

		ability->ReduceCooldownRemaining(full * 1.5f);
		report.Check(ability->GetCharges() == 2 && Near(ability->GetCooldownRemaining(), full * .5f), "a 1.5 cycle reduction restores a second charge and carries the half cycle", Json{ { "charges", ability->GetCharges() }, { "cooldownRemaining", ability->GetCooldownRemaining() } });

		const float half = ability->GetCooldownRemaining();
		ability->ReduceCooldownRemaining(half);
		report.Check(ability->GetCharges() == 3 && !ability->IsOnCooldown(), "reducing the last timer refills the pool", Json{ { "charges", ability->GetCharges() } });

		WaitUseGap(ability);
		report.Check(Spend(ability), "ASC reduction check spends one charge");
		const float remaining = ability->GetCooldownRemaining();
		abilities.ReduceAbilityCooldowns(remaining, true);
		report.Check(ability->GetCharges() == 3 && !ability->IsOnCooldown(), "ReduceAbilityCooldowns on the real ASC returns the spent charge", Json{ { "charges", ability->GetCharges() } });
	}

	// Scenario 4: bonus add/remove keeps charges and timer consistent, through both slot and ability-id addressing.
	void ChargeBonusLifecycleChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		GameAbility* ability = GetAbility3(report, f);
		auto& abilities = f.ship->GetAbilitySystemComponent();
		const std::string abilityId = ability->GetDefinition().abilityId;
		report.Check(Spend(ability), "lifecycle check spends the single shipped charge");
		report.Check(ability->IsOnCooldown(), "single-charge spend starts the recharge timer");

		report.Check(abilities.SetAbilityChargeBonus(kSlot, kBonusUnderTest), "slot-addressed bonus is accepted while the timer runs");
		report.Check(ability->GetChargeCapacity() == 3 && ability->GetCharges() == 2, "bonus adds its charges at once without clearing the running timer", Json{ { "capacity", ability->GetChargeCapacity() }, { "charges", ability->GetCharges() } });
		report.Check(ability->IsOnCooldown(), "timer keeps running while a charge is still missing");

		WaitUseGap(ability);
		report.Check(Spend(ability), "bonus charge is usable without waiting for the timer");
		report.Check(ability->GetCharges() == 1, "one spend from the extended pool leaves 1 charge");
		report.Check(abilities.ClearAbilityChargeBonus(kSlot), "bonus is removed by slot");
		report.Check(ability->GetChargeCapacity() == 1 && ability->GetCharges() == 1 && !ability->IsOnCooldown(), "removing the bonus clips surplus and clears the timer once full", Json{ { "capacity", ability->GetChargeCapacity() }, { "charges", ability->GetCharges() } });

		report.Check(Spend(ability), "after removal the base single charge is spendable again");
		report.Check(ability->IsOnCooldown() && ability->GetCharges() == 0, "base spend starts the timer again");
		report.Check(abilities.SetAbilityChargeBonus(abilityId, kBonusUnderTest), "id-addressed bonus is accepted");
		report.Check(ability->GetCharges() == 2 && ability->GetChargeCapacity() == 3, "id-addressed bonus adds 2 full charges");
		WaitUseGap(ability);
		report.Check(Spend(ability), "id-bonus spend 1");
		WaitUseGap(ability);
		report.Check(Spend(ability), "id-bonus spend 2");
		report.Check(ability->GetCharges() == 0 && ability->IsOnCooldown(), "pool is empty with the timer running");
		WaitUseGap(ability);
		report.Check(abilities.ClearAbilityChargeBonus(abilityId), "bonus is removed by ability id");
		report.Check(ability->GetCharges() == 0 && ability->IsOnCooldown(), "removing the bonus with no charge present keeps the missing charge's timer", Json{ { "charges", ability->GetCharges() }, { "cooldownRemaining", ability->GetCooldownRemaining() } });
		report.Check(!abilities.SetAbilityChargeBonus(kSlot, 0), "clearing an already-clear bonus is a no-op");
	}

	// Scenario 5: maxCharges 1 with no bonus keeps the original single-charge behaviour exactly (regression guard).
	void SingleChargeRegressionChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		GameAbility* ability = GetAbility3(report, f);
		auto& abilities = f.ship->GetAbilitySystemComponent();
		report.Check(ability->GetChargeCapacity() == 1 && ability->GetChargeBonus() == 0 && ability->GetCharges() == 1, "single-charge ability starts with capacity 1 and a full charge");
		report.Check(!abilities.SetAbilityChargeBonus(kSlot, 0), "zero bonus on a single-charge ability changes nothing");
		report.Check(!abilities.SetAbilityChargeBonus(std::string{ "Ability.Missing.Id" }, kBonusUnderTest), "bonus on an unknown ability id is rejected");
		report.Check(ability->GetCharges() == 1, "rejected bonus calls leave the charge alone");

		report.Check(Spend(ability), "single-charge ability activates with its only charge");
		report.Check(ability->GetCharges() == 0 && ability->IsOnCooldown(), "activation spends the only charge and starts the cooldown");
		report.Check(!ability->TryActivate(), "single-charge ability is blocked while the cooldown runs");
		const float full = ability->GetCooldownRemaining();
		ability->Tick(full);
		report.Check(ability->GetCharges() == 1 && !ability->IsOnCooldown(), "ticking through the cooldown restores the single charge exactly");
		report.Check(ability->TryActivate(), "single-charge ability reactivates immediately after recovery (no anti-spam gap at capacity 1)");
		ability->Cancel(sas::AbilityEndReason::Completed);

		// The reactivation above already spent the restored charge; the single timer is running.
		report.Check(ability->GetCharges() == 0 && ability->IsOnCooldown(), "reactivation spends the restored charge and restarts the single timer", Json{ { "charges", ability->GetCharges() }, { "cooldownRemaining", ability->GetCooldownRemaining() } });
		const float remaining = ability->GetCooldownRemaining();
		ability->ReduceCooldownRemaining(remaining + 1.f);
		report.Check(ability->GetCooldownRemaining() == 0.f && ability->GetCharges() == 1, "over-reduction clears the cooldown and restores the single charge", Json{ { "charges", ability->GetCharges() } });
		report.Check(ability->TryActivate(), "single-charge ability is activatable after reduction");
		ability->Cancel(sas::AbilityEndReason::Completed);
	}

	// Scenario 6: ten attach/detach rounds keep the ability handle stable and leave no stale charge state.
	void AttachDetachRoundChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		GameAbility* ability = GetAbility3(report, f);
		auto& abilities = f.ship->GetAbilitySystemComponent();
		const sas::AbilityHandle handle = ability->GetHandle();
		const std::string abilityId = ability->GetDefinition().abilityId;
		report.Check(handle.IsValid(), "round test starts from a valid ability handle");
		for (int round = 1; round <= 10; ++round)
		{
			const std::string label = "round " + std::to_string(round);
			report.Check(ability->TryActivate(), label + " activates the single shipped charge");
			ability->Cancel(sas::AbilityEndReason::Completed);
			report.Check(abilities.SetAbilityChargeBonus(abilityId, kBonusUnderTest) && ability->GetChargeCapacity() == 3, label + " attaches a bonus (capacity 3)");
			report.Check(abilities.GetAbility(kSlot) == ability && ability->GetHandle() == handle, label + " keeps the same ability object and handle while attached");
			report.Check(ability->GetCharges() <= 3 && (ability->GetCharges() >= ability->GetChargeCapacity()) == !ability->IsOnCooldown(), label + " keeps the full-pool/timer invariant while attached", Json{ { "charges", ability->GetCharges() }, { "cooldownRemaining", ability->GetCooldownRemaining() } });
			report.Check(abilities.ClearAbilityChargeBonus(kSlot) && ability->GetChargeCapacity() == 1, label + " detaches the bonus and returns to capacity 1");
			report.Check(ability->GetChargeBonus() == 0 && ability->GetCharges() <= 1 && (ability->GetCharges() >= 1) == !ability->IsOnCooldown(), label + " leaves no stale charge state after detaching", Json{ { "charges", ability->GetCharges() }, { "cooldownRemaining", ability->GetCooldownRemaining() } });
			report.Check(ability->GetCharges() == 1 && !ability->IsOnCooldown(), label + " ends the round with one full charge");
		}
		report.Check(abilities.GetAbility(kSlot) == ability && ability->GetHandle() == handle, "handle stays identical after ten attach/detach rounds");
		report.Check(ability->TryActivate(), "ability still activates after ten rounds");
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
	vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	report.artifact["test"] = "AbilityMultiChargeE2E";
	report.artifact["inputs"] = Json{ { "abilitySlot", "Ability3 (shipped definition, unchanged; capacity via runtime bonus)" }, { "chargeBonusUnderTest", kBonusUnderTest } };
	report.artifact["scenarios"] = Json::array();
	bool allPassed = true;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "multi-charge content registration failed" };
		std::string title = "Ability Multi Charge E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		Json& scenarios = report.artifact["scenarios"];
		if (!RunScenario(scenarios, "1 capacity 3: back-to-back uses gated by the anti-spam gap, not the timer", [&] { MultiChargeUseChecks(report, application); })) allPassed = false;
		if (!RunScenario(scenarios, "2 recharge: one charge per completed cycle, single timer restarts", [&] { RechargeCycleChecks(report, application); })) allPassed = false;
		if (!RunScenario(scenarios, "3 reduction: ability and ASC cooldown reduction return charges and carry overflow", [&] { ReduceCooldownChecks(report, application); })) allPassed = false;
		if (!RunScenario(scenarios, "4 bonus lifecycle: add, clip and clear keep charges and timer consistent", [&] { ChargeBonusLifecycleChecks(report, application); })) allPassed = false;
		if (!RunScenario(scenarios, "5 regression: maxCharges 1 without bonus keeps the original behaviour", [&] { SingleChargeRegressionChecks(report, application); })) allPassed = false;
		if (!RunScenario(scenarios, "6 attach/detach: ten rounds keep the handle stable and leave no stale charge state", [&] { AttachDetachRoundChecks(report, application); })) allPassed = false;
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
