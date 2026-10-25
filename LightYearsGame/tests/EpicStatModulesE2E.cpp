#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "gameplay/attributes/AttributeIds.h"
#include "UncommonModuleP6E2E.h"

#include <algorithm>
#include <cmath>

namespace
{
	using namespace uncommon_p6_e2e;

	// Base values only: the default loadout adds no Attack Power or Energy Power.
	void SetPower(Fixture& f, float attack, float energy)
	{
		auto& attributes = f.ship->GetAbilitySystemComponent().GetAttributes();
		attributes.SetBaseValue(OwnerAttributeIds::AttackPower, attack); attributes.SetBaseValue(OwnerAttributeIds::EnergyPower, energy);
	}
	// Effective level = placement level + zone level bonus, as the resolver computes it.
	float EffectiveLevel(Fixture& f, std::uint32_t instanceId, int level)
	{
		return static_cast<float>(level + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), instanceId));
	}
	// Lookup in the resolved effects, so the check reads the rule output rather than a live attribute.
	float ResolvedFamilyCooldownMultiplier(Fixture& f)
	{
		for (const auto& rule : f.ship->GetModuleApplier().GetResolvedEffects().abilityRules)
		{
			if (std::find(rule.requiredAbilityTags.begin(), rule.requiredAbilityTags.end(), GameplayTags::Ability::Utility) == rule.requiredAbilityTags.end()) continue;
			for (const auto& modifier : rule.attributeModifiers) if (modifier.attributeId == CommonAttributeIds::Cooldown) return modifier.magnitude;
		}
		return -1.f;
	}
	float ResolvedPrimaryFireRateMultiplier(Fixture& f, float expected)
	{
		for (const auto& modifier : f.ship->GetModuleApplier().GetResolvedEffects().primaryWeaponModifiers)
			if (modifier.attributeId == CommonAttributeIds::FireRate && std::fabs(modifier.magnitude - expected) < .0001f) return modifier.magnitude;
		return -1.f;
	}

	void TwinChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; SetPower(f, 100.f, 50.f);
			const auto twin = f.Install("Module.Offense.TwinReactor.Epic", 0, level);
			const float bonus = 5.f * EffectiveLevel(f, twin, level);
			f.Equal("Twin Reactor sets AP to the higher power plus its flat bonus", 100.f + bonus, f.Owner(OwnerAttributeIds::AttackPower));
			f.Equal("Twin Reactor sets EP to the same value", 100.f + bonus, f.Owner(OwnerAttributeIds::EnergyPower));
			SetPower(f, 40.f, 100.f);
			f.Equal("the lower AP follows the higher EP without feedback", 100.f + bonus, f.Owner(OwnerAttributeIds::AttackPower));
			f.Equal("EP keeps the equalised value", 100.f + bonus, f.Owner(OwnerAttributeIds::EnergyPower));
			SetPower(f, 100.f, 100.f);
			f.Equal("equal powers keep the shared value", 100.f + bonus, f.Owner(OwnerAttributeIds::AttackPower));
			f.Remove(twin);
			f.Equal("uninstall restores AP", 100.f, f.Owner(OwnerAttributeIds::AttackPower));
			f.Equal("uninstall restores EP", 100.f, f.Owner(OwnerAttributeIds::EnergyPower));
		}
	}

	void VitalChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const float armorBefore = f.Owner(OwnerAttributeIds::Armor);
			const float healthBefore = f.ship->GetHealthComponent().GetHealth();
			const float maxHealth = f.ship->GetHealthComponent().GetMaxHealth();
			const auto vital = f.Install("Module.Defense.VitalAlloy.Epic", 0, level);
			const float fraction = .12f + .03f * EffectiveLevel(f, vital, level);
			f.Equal("Vital Alloy grants Armor as a share of Max Health", armorBefore + fraction * maxHealth, f.Owner(OwnerAttributeIds::Armor));
			f.Equal("Vital Alloy does not spend health", healthBefore, f.ship->GetHealthComponent().GetHealth());
			f.Remove(vital);
			f.Equal("uninstall removes Vital Alloy Armor", armorBefore, f.Owner(OwnerAttributeIds::Armor));
		}
	}

	void ConfluenceChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			// Two Offense, one Defense, one Movement, one Utility and one Primary main module; system shells must not count.
			f.Install("Module.Offense.AssaultCore.Basic", 0, 1);
			f.Install("Module.Offense.AttackMatrix.Basic", 0, 1);
			f.Install("Module.Defense.HeavyArmorPlate.Basic", 0, 1);
			f.Install("Module.Movement.ThrustBooster.Basic", 0, 1);
			f.Install("Module.Utility.CoolingCircuit.Basic", 0, 1);
			f.Install("Module.Primary.FocusedBarrel.Basic", 0, 1);
			const float attack = f.Owner(OwnerAttributeIds::AttackPower);
			const float energy = f.Owner(OwnerAttributeIds::EnergyPower);
			const float maxHealth = f.Owner(OwnerAttributeIds::MaxHealth);
			const float horizontal = f.Owner(OwnerAttributeIds::MoveSpeedHorizontal);
			const float vertical = f.Owner(OwnerAttributeIds::MoveSpeedVertical);
			const auto confluence = f.Install("Module.Utility.ConfluenceCore.Epic", 0, level);
			const float eff = EffectiveLevel(f, confluence, level);
			f.Equal("Confluence adds its per-module AP for each Offense module", attack + 2.f * (eff + 1.f), f.Owner(OwnerAttributeIds::AttackPower));
			f.Equal("Confluence adds its per-module EP for each Offense module", energy + 2.f * (eff + 1.f), f.Owner(OwnerAttributeIds::EnergyPower));
			f.Equal("Confluence adds Max Health per Defense module", maxHealth + 10.f + 5.f * (eff - 1.f), f.Owner(OwnerAttributeIds::MaxHealth));
			const float movement = .25f + .1f * (eff - 1.f);
			f.Equal("Confluence adds horizontal rating per Movement module", horizontal + movement, f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
			f.Equal("Confluence adds vertical rating per Movement module", vertical + movement, f.Owner(OwnerAttributeIds::MoveSpeedVertical));
			// Per-module percentages are 1 + .5 (L-1) %, stored as fractions; the Utility count excludes Confluence itself.
			const float percent = .005f + .005f * eff;
			f.Equal("Confluence cooldown recovery counts Utility modules without itself", 1.f - percent, ResolvedFamilyCooldownMultiplier(f));
			f.Equal("Confluence fire rate counts the Primary module", 1.f + percent, ResolvedPrimaryFireRateMultiplier(f, 1.f + percent));
			f.Remove(confluence);
			f.Equal("uninstall removes Confluence AP", attack, f.Owner(OwnerAttributeIds::AttackPower));
		}
	}

	void MomentumChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			// One other Movement main module, so the Momentum count is 1.
			f.Install("Module.Movement.ThrustBooster.Basic", 0, 1);
			const float horizontal = f.Owner(OwnerAttributeIds::MoveSpeedHorizontal);
			const float vertical = f.Owner(OwnerAttributeIds::MoveSpeedVertical);
			const auto drive = f.Install("Module.Movement.MomentumDrive.Epic", 0, level);
			float total = 3.f + .5f * (EffectiveLevel(f, drive, level) - 1.f) + .25f + static_cast<float>(f.ship->GetAbilitySystemComponent().CountLoadoutAbilitiesWithTag(GameplayTags::Ability::Movement));
			f.Equal("Momentum Drive adds its base and per-module horizontal rating", horizontal + total, f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
			f.Equal("Momentum Drive adds its base and per-module vertical rating", vertical + total, f.Owner(OwnerAttributeIds::MoveSpeedVertical));
			CastMovement(f);
			total = 3.f + .5f * (EffectiveLevel(f, drive, level) - 1.f) + .25f + static_cast<float>(f.ship->GetAbilitySystemComponent().CountLoadoutAbilitiesWithTag(GameplayTags::Ability::Movement));
			f.Equal("Movement ability use doubles the module total for the window", horizontal + 2.f * total, f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
			CastMovement(f);
			f.Equal("re-use refreshes the window without stacking", horizontal + 2.f * total, f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
			auto& abilities = f.ship->GetAbilitySystemComponent();
			report.Check(abilities.CountLoadoutAbilitiesWithTag(GameplayTags::Ability::Movement) == 1, "Momentum counts Movement category from ability tags");
			auto movement = *content::AbilityContentCatalog::FindById("Ability.Movement.PhaseDrift.Basic");
			movement.slot = sas::AbilitySlot::Ability1;
			if (const auto* current = abilities.GetAbility(movement.slot)) report.Check(abilities.RemoveAbility(current->GetHandle()), "Momentum replaces Q in its fixture");
			const auto second = abilities.GrantAbility(movement);
			report.Check(second.IsValid() && abilities.CountLoadoutAbilitiesWithTag(GameplayTags::Ability::Movement) == 2, "Momentum sees a second equipped Movement ability");
			f.Equal("loadout grant recalculates base and active doubled contribution immediately", horizontal + 2.f * (total + 1.f), f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
			report.Check(abilities.RemoveAbility(second), "Momentum removes the second Movement ability");
			f.Equal("loadout removal recomputes the active doubled contribution", horizontal + 2.f * total, f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
			for (int step = 0; step < 11; ++step) f.level->TickInternal(.5f);
			f.Equal("the doubled rating ends after five seconds", horizontal + total, f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
			const auto movementHandle = abilities.GetAbility(sas::AbilitySlot::Ability2)->GetHandle();
			auto movementDefinition = abilities.GetAbility(sas::AbilitySlot::Ability2)->GetDefinition();
			report.Check(abilities.RebindAbility(movementHandle, sas::AbilitySlot::Ability1), "Movement ability moves to another loadout slot");
			f.Equal("moving between loadout slots does not duplicate the movement point", horizontal + total, f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
			abilities.ClearAbilitySlot(sas::AbilitySlot::Ability1);
			report.Check(abilities.CountLoadoutAbilitiesWithTag(GameplayTags::Ability::Movement) == 0, "an empty Movement slot does not count");
			f.Equal("clearing the loadout slot removes its movement point", horizontal + total - 1.f, f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
			report.Check(abilities.GrantAbility(movementDefinition).IsValid(), "Movement ability returns to the loadout");
			f.Equal("restoring the loadout slot restores its movement point", horizontal + total, f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
			f.Remove(drive);
		}
		Fixture f{ report, application };
		const float horizontal = f.Owner(OwnerAttributeIds::MoveSpeedHorizontal);
		const auto drive = f.Install("Module.Movement.MomentumDrive.Epic", 1, 1);
		f.Equal("the two-cell size keeps the base rating", horizontal + 3.f + .5f * (EffectiveLevel(f, drive, 1) - 1.f) + static_cast<float>(f.ship->GetAbilitySystemComponent().CountLoadoutAbilitiesWithTag(GameplayTags::Ability::Movement)), f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
		CastMovement(f);
		auto& abilities = f.ship->GetAbilitySystemComponent();
		const float total = 3.f + .5f * (EffectiveLevel(f, drive, 1) - 1.f) + 1.f;
		f.Equal("Momentum doubles its ability point without any other Movement module", horizontal + 2.f * total, f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
		const auto movementDefinition = abilities.GetAbility(sas::AbilitySlot::Ability2)->GetDefinition();
		abilities.ClearAbilitySlot(sas::AbilitySlot::Ability2);
		f.Equal("removing the last Movement ability keeps the base doubled for the remaining window", horizontal + 2.f * (total - 1.f), f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
		report.Check(abilities.GrantAbility(movementDefinition).IsValid(), "last Movement ability returns during the active window");
		f.Equal("restoring the last Movement ability restores its doubled point immediately", horizontal + 2.f * total, f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
		f.Remove(drive);
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
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "epic stat modules content registration failed" };
		std::string title = "Epic stat modules E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		TwinChecks(report, application); VitalChecks(report, application); ConfluenceChecks(report, application); MomentumChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "EpicStatModulesE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
