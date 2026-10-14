#include "UncommonModuleP6E2E.h"

namespace
{
	using namespace uncommon_p6_e2e;
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
	void CastDefense(Fixture& f) { auto& ability = Defense(f); f.report.Check(ability.TryActivate(), "activate real Defense ability"); if (ability.IsActive()) ability.Cancel(sas::AbilityEndReason::Completed); }
	void SetArmor(Fixture& f, float armor) { f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, armor); }
	void SetPower(Fixture& f, float attack, float energy)
	{
		auto& attributes = f.ship->GetAbilitySystemComponent().GetAttributes();
		attributes.SetBaseValue(OwnerAttributeIds::AttackPower, attack); attributes.SetBaseValue(OwnerAttributeIds::EnergyPower, energy);
	}
	void ArmorChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			const float coefficient = .35f + .05f * level;
			Fixture f{ report, application }; SetArmor(f, 80.f); SetPower(f, 100.f, 50.f);
			const auto main = f.Install("Module.Defense.ArmorConversion.Uncommon", 0, level);
			f.Equal("Armor converts into the higher power", 100.f + 80.f * coefficient, f.Owner(OwnerAttributeIds::AttackPower));
			f.Equal("main conversion leaves lower power unchanged", 50.f, f.Owner(OwnerAttributeIds::EnergyPower));
			SetPower(f, 40.f, 100.f);
			f.Equal("power switching removes the old AP bonus", 40.f, f.Owner(OwnerAttributeIds::AttackPower));
			f.Equal("power switching transfers conversion to EP", 100.f + 80.f * coefficient, f.Owner(OwnerAttributeIds::EnergyPower));
			SetPower(f, 100.f, 100.f); f.Equal("ties select AP", 100.f + 80.f * coefficient, f.Owner(OwnerAttributeIds::AttackPower));
			f.Remove(main); f.Equal("uninstall removes conversion", 100.f, f.Owner(OwnerAttributeIds::AttackPower));
			const auto balanced = f.Install("Module.Defense.ArmorConversion.Uncommon", 1, level, "Module.SideFeature.ArmorConversion.BalancedOutput");
			f.Equal("Balanced Output chooses EP on a tie", 100.f + 80.f * coefficient * .25f, f.Owner(OwnerAttributeIds::EnergyPower));
			f.Equal("Balanced Output cannot feed back into AP", 100.f + 80.f * coefficient, f.Owner(OwnerAttributeIds::AttackPower));
			SetArmor(f, 120.f); f.Equal("Armor changes refresh the main", 100.f + 120.f * coefficient, f.Owner(OwnerAttributeIds::AttackPower));
			f.Equal("Armor changes refresh the side", 100.f + 120.f * coefficient * .25f, f.Owner(OwnerAttributeIds::EnergyPower)); f.Remove(balanced);
			SetArmor(f, 80.f); const float speed = f.Owner(OwnerAttributeIds::AttackSpeed);
			const auto rapid = f.Install("Module.Defense.ArmorConversion.Uncommon", 1, level, "Module.SideFeature.ArmorConversion.RapidAlloy");
			f.Equal("eighty Armor grants eight AS points", speed + 8.f, f.Owner(OwnerAttributeIds::AttackSpeed));
			SetArmor(f, 120.f); f.Equal("Rapid Alloy tracks changed Armor", speed + 12.f, f.Owner(OwnerAttributeIds::AttackSpeed)); f.Remove(rapid);
			f.Equal("uninstall removes Rapid Alloy", speed, f.Owner(OwnerAttributeIds::AttackSpeed));
		}
		Fixture f{ report, application }; SetArmor(f, 80.f); const float speed = f.Owner(OwnerAttributeIds::AttackSpeed);
		f.Install("Module.Defense.ArmorConversion.Uncommon", 0, 1, "Module.SideFeature.ArmorConversion.RapidAlloy");
		f.Equal("two-cell Armor keeps Rapid Alloy locked", speed, f.Owner(OwnerAttributeIds::AttackSpeed));
	}
	void ShieldChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; auto& shield = f.ship->GetShieldComponent(); const float maximum = shield.GetMaxShield();
			const auto module = f.Install("Module.Defense.OvershieldLayer.Uncommon", 1, level, "Module.SideFeature.OvershieldLayer.ShieldRush");
			shield.ChangeShield(maximum); CastOffense(f); f.Equal("Offense cannot grant temporary shield", 0.f, shield.GetModuleTemporaryShield());
			CastDefense(f); f.Equal("Defense grants twenty-percent layer", maximum * .2f, shield.GetModuleTemporaryShield());
			f.Equal("layer can exceed normal maximum", maximum * 1.2f, shield.GetShield());
			f.Equal("Shield Rush activates immediately", 1.2f, f.ship->GetRuntimeModifiers().GetMovementSpeedMultiplier());
			shield.TickTemporaryOvershields(3.f); f.Equal("temporary layer decays continuously", maximum * .1f, shield.GetModuleTemporaryShield());
			CastDefense(f); f.Equal("repeat refreshes instead of stacking", maximum * .2f, shield.GetModuleTemporaryShield());
			shield.AbsorbDamage(maximum * .2f, 1.f, 0.f); f.Equal("damage consumes layer before normal shield", maximum, shield.GetShield());
			f.Equal("Shield Rush ends on consumption", 1.f, f.ship->GetRuntimeModifiers().GetMovementSpeedMultiplier());
			shield.ChangeShield(-maximum); CastDefense(f); f.Equal("empty normal shield still gains temporary layer", maximum * .2f, shield.GetShield());
			shield.TickTemporaryOvershields(6.f); f.Equal("layer below maximum still expires", 0.f, shield.GetShield());
			shield.ChangeShield(maximum); CastDefense(f); shield.GrantTemporaryOvershield("Other.Ability", 10.f, 0.f, 1.f);
			f.Remove(module); f.Equal("uninstall removes only this module layer", maximum + 10.f, shield.GetShield());
			f.Equal("uninstall removes movement modifier", 1.f, f.ship->GetRuntimeModifiers().GetMovementSpeedMultiplier());
		}
		Fixture f{ report, application }; auto& shield = f.ship->GetShieldComponent(); const float maximum = shield.GetMaxShield();
		f.Install("Module.Defense.OvershieldLayer.Uncommon", 1, 1, "Module.SideFeature.OvershieldLayer.ReserveShell"); CastDefense(f);
		const float previous = shield.GetShield(); Hit(*f.ship, nullptr, 10.f, Plain());
		f.Equal("Reserve Shell reduces incoming damage while over maximum", previous - 9.f, shield.GetShield());
		shield.ChangeShield(-10000.f); shield.ChangeShield(maximum);
		Hit(*f.ship, nullptr, 10.f, Plain()); f.Equal("Reserve Shell ends when no longer over maximum", maximum - 10.f, shield.GetShield());
	}
	void ContentChecks(Report& report)
	{
		for (const auto* id : { "Module.Defense.ArmorConversion.Uncommon", "Module.Defense.OvershieldLayer.Uncommon" })
		{
			const auto* module = content::ModuleContentCatalog::FindById(id);
			report.Check(module && module->sizeVariants.size() == 2 && module->sideFeatures.size() == 2 && module->sharedPools.front().allowedFeatureIds.size() == 3, "new module has a complete five-choice ladder");
			report.Check(module->sizeVariants[1].shape.cells == List<sf::Vector2i>{ { 0, 0 }, { 1, 0 }, { 0, 1 } }, "three-cell shape is L");
		}
	}
	void PrimerChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; const auto target = Enemy(f), other = Enemy(f, 100.f);
			const auto module = f.Install("Module.Primary.StatusPrimer.Uncommon", 0, level);
			const auto primary = [&](SpaceShip& enemy, bool suppressed = false)
			{
				auto payload = Plain(); if (suppressed) payload.moduleTriggerPolicy = DamageModuleTriggerPolicy::Suppressed;
				Hit(enemy, f.ship.get(), 1.f, payload, {}, { GameplayTags::Ability::Primary });
			};
			primary(*target); f.Equal("Primer cannot create a status on a clean target", 0.f, static_cast<float>(Stacks(*target, DamageStatusEffectIds::ElectricEffectId)));
			StatusHit(*target, f.ship.get(), DamageTypeSchema::Electric, 2); StatusHit(*target, f.ship.get(), DamageTypeSchema::Thermal, 1);
			Hit(*target, f.ship.get(), 1.f, Plain(), {}, { GameplayTags::Ability::Offense });
			f.Equal("Offense hit does not activate Primer", 2.f, static_cast<float>(Stacks(*target, DamageStatusEffectIds::ElectricEffectId)));
			primary(*target); f.Equal("Primer adds one to the most-stacked status", 3.f, static_cast<float>(Stacks(*target, DamageStatusEffectIds::ElectricEffectId)));
			f.Equal("Primer leaves the lower-stack status untouched", 1.f, static_cast<float>(Stacks(*target, DamageStatusEffectIds::IgniteEffectId)));
			primary(*target); f.Equal("same-target cooldown blocks rapid hits", 3.f, static_cast<float>(Stacks(*target, DamageStatusEffectIds::ElectricEffectId)));
			StatusHit(*other, f.ship.get(), DamageTypeSchema::Electric, 1); primary(*other);
			f.Equal("cooldown is independent for another target", 2.f, static_cast<float>(Stacks(*other, DamageStatusEffectIds::ElectricEffectId)));
			f.ship->GetCombatRuntime().Tick(1.f); primary(*target);
			f.Equal("one second permits another stack", 4.f, static_cast<float>(Stacks(*target, DamageStatusEffectIds::ElectricEffectId)));
			primary(*other, true); f.Equal("generated hits cannot activate Primer", 2.f, static_cast<float>(Stacks(*other, DamageStatusEffectIds::ElectricEffectId)));
			f.Remove(module); primary(*other); f.Equal("removing Primer ends its effect", 2.f, static_cast<float>(Stacks(*other, DamageStatusEffectIds::ElectricEffectId)));
		}
	}
}
int main(int argc, char** argv)
{
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "defense layer content registration failed" };
		std::string title = "Module defense layer E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		ContentChecks(report); ArmorChecks(report, application); ShieldChecks(report, application); PrimerChecks(report, application); report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "ModuleDefenseLayerE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
