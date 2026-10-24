#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"

namespace
{
	using namespace uncommon_p4_e2e;

	// Authored Last Stand tables: charges [1, 1, 2] and revive health [40%, 45%, 50%], both extrapolated past level 3 by the last step.
	int ExpectedCharges(int level) { return level <= 2 ? 1 : 2 + (level - 3); }
	float ExpectedReviveFraction(int level) { return level <= 1 ? .40f : level == 2 ? .45f : .50f + .05f * static_cast<float>(level - 3); }
	// Emergency Protocol cooldown recovery [x1.75, x2.00, x2.25], extrapolated past level 3 by the last step.
	float ExpectedRecovery(int level) { return level <= 1 ? 1.75f : level == 2 ? 2.f : 2.25f + .25f * static_cast<float>(level - 3); }

	GameAbility& Defense(Fixture& f)
	{
		auto& asc = f.ship->GetAbilitySystemComponent();
		auto* ability = asc.GetAbilityById("Ability.Defense.DirectionalBarrier.Basic");
		if (!ability)
		{
			if (auto* previous = asc.GetAbility(sas::AbilitySlot::Ability1)) f.report.Check(asc.RemoveAbility(previous->GetHandle()), "replace test Q without editing default loadout");
			auto definition = *content::AbilityContentCatalog::FindById("Ability.Defense.DirectionalBarrier.Basic"); definition.slot = sas::AbilitySlot::Ability1;
			f.report.Check(asc.GrantAbility(definition).IsValid(), "grant a shipped Defense ability");
			ability = asc.GetAbilityById("Ability.Defense.DirectionalBarrier.Basic");
		}
		if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
		ability->Tick(100.f);
		return *ability;
	}

	// Revive charges are written back to the loadout the way the Player wires them, so a reinstall keeps the spent count.
	void ReviveBinding(Fixture& f)
	{
		f.ship->GetModuleApplier().SetReviveSpentSink([&f](std::uint32_t instanceId) { f.report.Check(f.loadout.AddReviveChargeSpent(instanceId), "revive charge is recorded on the loadout placement"); });
	}

	void LastStandChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			ReviveBinding(f);
			auto& health = f.ship->GetHealthComponent();
			const float maxHealth = health.GetMaxHealth();
			const auto id = f.Install("Module.Defense.LastStand.Epic", 0, level);
			const int effective = level + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), id);
			const int charges = ExpectedCharges(effective);
			const float baselinePower = f.Owner(OwnerAttributeIds::AttackPower);
			const auto enemy = Enemy(f);
			for (int use = 1; use <= charges; ++use)
			{
				Hit(*f.ship, enemy.get(), 100000.f, Plain());
				report.Check(!f.ship->GetIsPendingDestroy(), "lethal damage revives while a charge remains");
				f.Equal("revive sets health to the effective-level fraction of Max Health", ExpectedReviveFraction(effective) * maxHealth, health.GetHealth());
				f.Equal("revive grants the 20% combat bonus", baselinePower * 1.2f, f.Owner(OwnerAttributeIds::AttackPower));
				f.ship->GetModuleApplier().Tick(14.9f);
				f.Equal("combat bonus lasts until 15 seconds", baselinePower * 1.2f, f.Owner(OwnerAttributeIds::AttackPower));
				f.ship->GetModuleApplier().Tick(.2f);
				f.Equal("combat bonus ends after 15 seconds", baselinePower, f.Owner(OwnerAttributeIds::AttackPower));
			}
			report.Check(f.loadout.FindPlacement(id)->reviveChargesSpent == charges, "every granted charge is recorded as spent");
			Hit(*f.ship, enemy.get(), 100000.f, Plain());
			report.Check(f.ship->GetIsPendingDestroy(), "a spent Last Stand lets lethal damage kill");
		}
	}

	void SpentChargeChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		ReviveBinding(f);
		const auto id = f.Install("Module.Defense.LastStand.Epic", 0, 3);
		const int charges = ExpectedCharges(3 + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), id));
		const auto enemy = Enemy(f);
		Hit(*f.ship, enemy.get(), 100000.f, Plain());
		report.Check(!f.ship->GetIsPendingDestroy(), "first charge revives before the reinstall");
		const auto placement = *f.loadout.FindPlacement(id);
		f.Remove(id);
		report.Check(f.loadout.Install(f.rules, id, placement.origin, placement.rotationQuarterTurns) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "reinstall the same Last Stand");
		report.Check(f.loadout.FindPlacement(id)->reviveChargesSpent == 1, "spent charge survives reinstall");
		for (int use = 2; use <= charges; ++use)
		{
			Hit(*f.ship, enemy.get(), 100000.f, Plain());
			report.Check(!f.ship->GetIsPendingDestroy(), "remaining charges still revive after reinstall");
		}
		Hit(*f.ship, enemy.get(), 100000.f, Plain());
		report.Check(f.ship->GetIsPendingDestroy(), "lethal damage kills after the last charge");
	}

	void EmergencyChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			auto& health = f.ship->GetHealthComponent();
			const float maxHealth = health.GetMaxHealth();
			const auto id = f.Install("Module.Defense.EmergencyProtocol.Epic", 0, level);
			const float recovery = ExpectedRecovery(level + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), id));
			GameAbility& ability = Defense(f);
			report.Check(ability.TryActivate(), "activate a real Defense ability for the cooldown check");
			if (ability.IsActive()) ability.Cancel(sas::AbilityEndReason::Completed);
			report.Check(ability.IsOnCooldown() && ability.GetCooldownRemaining() > 2.f, "Defense ability starts a cooldown long enough to measure");
			const float start = ability.GetCooldownRemaining();
			f.ship->GetModuleApplier().Tick(.5f);
			f.Equal("healthy ship keeps the normal cooldown recovery", start, ability.GetCooldownRemaining());
			health.ChangeHealth(-maxHealth * .75f);
			f.ship->GetModuleApplier().Tick(.5f);
			f.Equal("threshold adds the rest of the recovery multiplier", start - .5f * (recovery - 1.f), ability.GetCooldownRemaining());
			const float low = ability.GetCooldownRemaining();
			health.Regenerate(maxHealth);
			f.ship->GetModuleApplier().Tick(.5f);
			f.Equal("leaving the threshold neither resets nor speeds the cooldown", low, ability.GetCooldownRemaining());
			f.Remove(id);
		}
	}

	void SideChecks(Report& report, Application& application)
	{
		{
			Fixture f{ report, application };
			auto& health = f.ship->GetHealthComponent();
			const float maxHealth = health.GetMaxHealth();
			const float baseline = f.ship->GetMovementSpeedMultiplier();
			f.Install("Module.Defense.EmergencyProtocol.Epic", 1, 1, "Module.SideFeature.EmergencyProtocol.EvacuationRoute");
			f.ship->GetModuleApplier().Tick(.1f);
			f.Equal("Evacuation Route is inactive at full health", baseline, f.ship->GetMovementSpeedMultiplier());
			health.ChangeHealth(-maxHealth * .69f);
			f.ship->GetModuleApplier().Tick(.1f);
			f.Equal("Evacuation Route is inactive above 30% health", baseline, f.ship->GetMovementSpeedMultiplier());
			health.ChangeHealth(-maxHealth * .02f);
			f.ship->GetModuleApplier().Tick(.1f);
			f.Equal("Evacuation Route adds 20% movement speed at 29% health", baseline * 1.2f, f.ship->GetMovementSpeedMultiplier());
			health.Regenerate(maxHealth);
			f.ship->GetModuleApplier().Tick(.1f);
			f.Equal("Evacuation Route ends when health rises above 30%", baseline, f.ship->GetMovementSpeedMultiplier());
		}
		{
			Fixture f{ report, application };
			auto& health = f.ship->GetHealthComponent();
			const float maxHealth = health.GetMaxHealth();
			f.Install("Module.Defense.EmergencyProtocol.Epic", 1, 1, "Module.SideFeature.EmergencyProtocol.Counterpressure");
			GameAbility& ability = Defense(f);
			const auto enemy = Enemy(f);
			f.ship->GetModuleApplier().Tick(.1f);
			const float healthy = Hit(*enemy, f.ship.get(), 100.f, Plain(), ability.GetDefinition().abilityId, ability.GetDefinition().abilityTags);
			f.Equal("Counterpressure leaves ability damage unchanged at full health", 100.f, healthy);
			health.ChangeHealth(-maxHealth * .75f);
			f.ship->GetModuleApplier().Tick(.1f);
			const float threshold = Hit(*enemy, f.ship.get(), 100.f, Plain(), ability.GetDefinition().abilityId, ability.GetDefinition().abilityTags);
			f.Equal("Counterpressure adds 15% ability damage at 25% health", 115.f, threshold);
		}
	}
}
int main(int argc, char** argv)
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Epic Defense content registration failed" };
		std::string title = "Epic Defense E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		LastStandChecks(report, application); SpentChargeChecks(report, application); EmergencyChecks(report, application); SideChecks(report, application); report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "EpicDefenseModulesE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
