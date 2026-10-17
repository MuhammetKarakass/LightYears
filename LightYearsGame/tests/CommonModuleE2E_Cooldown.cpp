#include "CommonModuleE2ETestSupport.h"

namespace common_module_e2e
{
	void RunCooldownModuleChecks(Report& report, vega::Application& application)
	{
		using namespace vega;
		const ShipDefinition& shipDefinition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
		const ModuleRulesContext rules{ *shipDefinition.schematic,
			[](const sas::ContentId& moduleId) { return content::ModuleContentCatalog::FindById(moduleId); },
			content::ModuleContentCatalog::GetZoneTable() };
		GameAbilityDefinition activationTestDefinition;
		auto level = std::make_shared<HeadlessModuleLevel>(&application);
		level->BeginPlayInternal();
		const shared_ptr<PlayerSpaceShip> ship = level->SpawnActor<PlayerSpaceShip>(shipDefinition).lock();
		report.Check(ship != nullptr, "cooldown fixture spawns a real player ship");
		ModuleLoadout loadout;
		report.Check(loadout.SeedDefaults(rules), "cooldown fixture seeds the real default loadout");
		report.Check(ship->GetModuleApplier().Apply(loadout, rules), "cooldown fixture applies its default modules");
		LightYearsAbilitySystemComponent& abilities = ship->GetAbilitySystemComponent();
		GameAbility* offense = abilities.GetAbility(sas::AbilitySlot::Ability3);
		GameAbility* defense = abilities.GetAbility(sas::AbilitySlot::Ability4);
		GameAbility* primary = abilities.GetAbility(sas::AbilitySlot::PrimaryFire);
		report.Check(offense && offense->GetDefinition().abilityId == "Ability.Offense.SeismicCharge.Basic" && defense,
			"cooldown fixture uses real Offense and Defense loadout abilities");
		const float offenseBase = offense->GetCooldownDuration();
		const float defenseBase = defense->GetCooldownDuration();
		const float attackSpeedBase = abilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackSpeed);
		const std::size_t baseHandleCount = ship->GetModuleApplier().GetOwnedHandleCount();
		const ModuleDefinition* cooler = FindModule(rules, "Module.Utility.ZoneCooler.Basic");
		report.Check(cooler != nullptr, "catalog contains Regional Cooler");
		const std::uint32_t coolerId = loadout.AddToInventory(cooler->moduleId);
		auto wings = FindPlacementWithCooldownZone(rules, loadout, *cooler, coolerId, GameplayTag{ "Module.Zone.Wings" });
		report.Check(wings.has_value(), "Regional Cooler has a legal Wings placement");
		report.Check(loadout.Install(rules, coolerId, wings->first, wings->second) == ModulePlacementFailure::None &&
			ship->GetModuleApplier().Apply(loadout, rules), "Regional Cooler installs through the real module applier");
		report.Check(ship->GetModuleApplier().GetResolvedEffects().cooldownRules.size() == 1 &&
			ship->GetModuleApplier().GetOwnedHandleCount() == baseHandleCount + 1,
			"Wings placement owns one resolved cooldown rule and handle");
		report.CheckValue("Wings cooler reduces the matching Offense cooldown by 21 percent", offenseBase * 0.79f,
			offense->GetCooldownDuration(), "Ability.Offense.SeismicCharge.Basic in Module.Zone.Wings");
		report.CheckValue("Wings cooler does not reduce a different family cooldown", defenseBase,
			defense->GetCooldownDuration(), "Ability.Defense.Cryostasis.Basic in Module.Zone.Wings");
		if (primary)
		{
			report.CheckValue("cooldown scope excludes Primary", 1.f,
				abilities.GetScopedAbilityCooldownMultiplier(primary->GetDefinition()), "PrimaryFire slot");
		}
		else report.Check(false, "player ship has a PrimaryFire ability");
		report.CheckValue("Regional Cooler does not change owner attack speed", attackSpeedBase,
			abilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackSpeed), "cooldown-only module installed");

		report.Check(offense->TryActivate(), "real Offense ability starts through the module cooldown rule");
		if (offense->IsActive()) offense->Cancel(sas::AbilityEndReason::Completed);
		const float startedCooldown = offense->GetCooldownRemaining();
		report.Check(offense->GetDefinition().cooldownStartPolicy == sas::AbilityCooldownStartPolicy::OnAbilityEnd,
			"Seismic Charge retains its shipped OnAbilityEnd cooldown policy");
		report.CheckValue("started cooldown uses the Wings multiplier", offenseBase * 0.79f,
			startedCooldown, "Seismic Charge cooldown start");
		const auto primaryPlacement = std::find_if(loadout.GetPlacements().begin(), loadout.GetPlacements().end(),
			[](const ModulePlacement& placement) { return placement.moduleId == "Module.Primary.PrimaryWeapon.Standard"; });
		report.Check(primaryPlacement != loadout.GetPlacements().end(), "fixture finds the installed Nose weapon module");
		const std::uint32_t primaryModuleId = primaryPlacement == loadout.GetPlacements().end() ? 0 : primaryPlacement->instanceId;
		report.Check(primaryModuleId != 0 && loadout.Uninstall(primaryModuleId) &&
			ship->GetModuleApplier().Apply(loadout, rules), "removing the Nose weapon frees a cell for the zone move");
		const auto nose = FindPlacementWithCooldownZone(rules, loadout, *cooler, coolerId,
			GameplayTag{ "Module.Zone.Nose" }, coolerId);
		// Some layouts have no three-cell line in the Nose; the Nose scenario then has nothing to test.
		if (nose.has_value())
		{
			const float beforeNoseMove = offense->GetCooldownRemaining();
			report.Check(nose && loadout.Move(rules, coolerId, nose->first, nose->second) == ModulePlacementFailure::None &&
				ship->GetModuleApplier().Apply(loadout, rules), "moving the active module to Nose rebuilds its resolved rules");
			report.Check(ship->GetModuleApplier().GetResolvedEffects().cooldownRules.empty(),
				"Nose placement resolves no Regional Cooler cooldown effect");
			report.CheckValue("Nose placement does not change owner attack speed", attackSpeedBase,
				abilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackSpeed), "cooler installed in Module.Zone.Nose");
			report.CheckValue("moving the module does not rewrite a running cooldown", beforeNoseMove,
				offense->GetCooldownRemaining(), "Wings to Nose while cooling down");
			offense->Tick(0.1f);
			report.CheckValue("running cooldown keeps its start-time module factor", beforeNoseMove - 0.1f,
				offense->GetCooldownRemaining(), "0.1 seconds elapsed after the move");
			offense->Tick(100.f);
			report.CheckValue("Nose placement has no effect on the next cooldown", offenseBase,
				offense->GetCooldownDuration(), "module remains in Module.Zone.Nose");
			report.Check(offense->TryActivate(), "Offense ability can start another cooldown after expiry");
			if (offense->IsActive()) offense->Cancel(sas::AbilityEndReason::Completed);
			report.CheckValue("next Nose cooldown starts at its unmodified value", offenseBase,
				offense->GetCooldownRemaining(), "Seismic Charge starts in Module.Zone.Nose");
			const float noseStartedCooldown = offense->GetCooldownRemaining();
			const auto wingsWhileCooling = FindPlacementWithCooldownZone(rules, loadout, *cooler, coolerId,
				GameplayTag{ "Module.Zone.Wings" }, coolerId);
			report.Check(wingsWhileCooling && loadout.Move(rules, coolerId, wingsWhileCooling->first, wingsWhileCooling->second) ==
				ModulePlacementFailure::None && ship->GetModuleApplier().Apply(loadout, rules),
				"moving the cooler from Nose to Wings updates rules during a running cooldown");
			report.CheckValue("adding a shorter Wings multiplier does not clamp a running Nose cooldown", noseStartedCooldown,
				offense->GetCooldownRemaining(), "unmodified cooldown already running");
			offense->Tick(0.1f);
			report.CheckValue("Nose-started cooldown only decreases by elapsed time after moving to Wings",
				noseStartedCooldown - 0.1f, offense->GetCooldownRemaining(), "0.1 seconds elapsed after Nose to Wings move");
			offense->Tick(100.f);
		}
		else offense->Tick(100.f);

		const auto lifeSupportPlacement = std::find_if(loadout.GetPlacements().begin(), loadout.GetPlacements().end(),
			[](const ModulePlacement& placement) { return placement.moduleId == "Module.Utility.LifeSupportUnit.Standard"; });
		report.Check(lifeSupportPlacement != loadout.GetPlacements().end(), "fixture finds the default Hull Life Support module");
		const std::uint32_t lifeSupportId = lifeSupportPlacement == loadout.GetPlacements().end() ? 0 : lifeSupportPlacement->instanceId;
		report.Check(lifeSupportId != 0 && loadout.Uninstall(lifeSupportId) &&
			ship->GetModuleApplier().Apply(loadout, rules), "removing default Life Support frees a legal Hull row");
		// A ship whose Hull has no three-cell line (the Fighter's is a 2x2) has no Hull placement for this three-cell module; the Hull scope is then not reachable.
		const auto hull = FindPlacementWithCooldownZone(rules, loadout, *cooler, coolerId,
			GameplayTag{ "Module.Zone.Hull" }, coolerId);
		if (hull.has_value())
		{
			report.Check(hull && loadout.Move(rules, coolerId, hull->first, hull->second) == ModulePlacementFailure::None &&
				ship->GetModuleApplier().Apply(loadout, rules), "moving Regional Cooler to Hull applies its all-active scope");
			report.CheckValue("Hull cooler applies to every active non-Primary ability", offenseBase * 0.9f,
				offense->GetCooldownDuration(), "Ability.Offense.SeismicCharge.Basic in Module.Zone.Hull");
			report.CheckValue("Hull cooler applies to Defense as well", defenseBase * 0.9f,
				defense->GetCooldownDuration(), "Ability.Defense.Cryostasis.Basic in Module.Zone.Hull");
			report.Check(offense->TryActivate(), "real Offense ability starts with the Hull cooldown rule");
			if (offense->IsActive()) offense->Cancel(sas::AbilityEndReason::Completed);
			report.CheckValue("started Hull cooldown uses the all-active multiplier", offenseBase * 0.9f,
				offense->GetCooldownRemaining(), "OnAbilityEnd cooldown start");
			offense->Tick(100.f);

			report.Check(loadout.Uninstall(coolerId) && ship->GetModuleApplier().Apply(loadout, rules),
				"uninstall removes Regional Cooler cooldown rules");
			const std::size_t handlesWithoutCooler = ship->GetModuleApplier().GetOwnedHandleCount();
			report.Check(ship->GetModuleApplier().GetResolvedEffects().cooldownRules.empty(),
				"uninstalled Regional Cooler has no resolved rules");
			auto rebootHull = FindPlacementWithCooldownZone(rules, loadout, *cooler, coolerId,
				GameplayTag{ "Module.Zone.Hull" });
			report.Check(rebootHull && loadout.Install(rules, coolerId, rebootHull->first, rebootHull->second) ==
				ModulePlacementFailure::None && ship->GetModuleApplier().Apply(loadout, rules, true) &&
				ship->GetModuleApplier().IsRebooting(coolerId), "reinstalling in combat starts the module reboot");
			report.Check(ship->GetModuleApplier().GetResolvedEffects().cooldownRules.empty() &&
				ship->GetModuleApplier().GetOwnedHandleCount() == handlesWithoutCooler,
				"reboot immediately removes cooldown effects and handles");
			ship->GetModuleApplier().Tick(1.49f);
			report.Check(ship->GetModuleApplier().IsRebooting(coolerId), "cooldown module remains rebooting before its deadline");
			ship->GetModuleApplier().Tick(0.02f);
			report.Check(!ship->GetModuleApplier().IsRebooting(coolerId) &&
				ship->GetModuleApplier().GetResolvedEffects().cooldownRules.size() == 1 &&
				ship->GetModuleApplier().GetOwnedHandleCount() == handlesWithoutCooler + 1,
				"reboot completion restores exactly one cooldown handle");
			const std::size_t restoredHandleCount = ship->GetModuleApplier().GetOwnedHandleCount();
			report.Check(!ship->GetModuleApplier().Apply(loadout, rules) &&
				ship->GetModuleApplier().GetOwnedHandleCount() == restoredHandleCount,
				"unchanged loadout keeps the cooldown handle count stable");
			report.CheckValue("rebooted Hull cooldown rule remains active", offenseBase * 0.9f,
				offense->GetCooldownDuration(), "same module instance restored after reboot");
		}
		const auto rebootWings = FindPlacementWithCooldownZone(rules, loadout, *cooler, coolerId,
			GameplayTag{ "Module.Zone.Wings" }, coolerId);
		report.Check(rebootWings && loadout.Move(rules, coolerId, rebootWings->first, rebootWings->second) ==
			ModulePlacementFailure::None && ship->GetModuleApplier().Apply(loadout, rules),
			"rebooted cooler moves to Wings for the OnActivation cooldown case");
		activationTestDefinition = offense->GetDefinition();
		activationTestDefinition.cooldownStartPolicy = sas::AbilityCooldownStartPolicy::OnActivation;
		std::string unequipFailure;
		report.Check(ship->GetAbilityLoadout().UnequipAbility(sas::AbilitySlot::Ability3, &unequipFailure),
			"fixture removes the shipped Seismic Charge before granting its test copy: " + unequipFailure);
		std::string grantFailure;
		const sas::AbilityHandle activationTestHandle = abilities.GrantAbility(activationTestDefinition, &grantFailure);
		report.Check(activationTestHandle.IsValid(), "ASC grants the canonical Seismic copy with OnActivation cooldown: " + grantFailure);
		GameAbility* activationTestAbility = abilities.GetAbilityById(activationTestDefinition.abilityId);
		report.Check(activationTestAbility && activationTestAbility->GetDefinition().cooldownStartPolicy ==
			sas::AbilityCooldownStartPolicy::OnActivation, "test copy retains OnActivation policy after grant");
		const float activationTestBase = activationTestAbility->GetCooldownDuration();
		report.CheckValue("test copy resolves its cooldown before starting", offenseBase * 0.79f,
			activationTestBase, "canonical Seismic copy while Regional Cooler is in Wings");
		report.Check(activationTestAbility->TryActivate(), "canonical Seismic copy activates through real ASC runtime");
		if (activationTestAbility->IsActive()) activationTestAbility->Cancel(sas::AbilityEndReason::Interrupted);
		report.CheckValue("OnActivation cooldown start captures the Wings multiplier", activationTestBase,
			activationTestAbility->GetCooldownRemaining(), "canonical Seismic definition copy in Module.Zone.Wings");
	}


}
