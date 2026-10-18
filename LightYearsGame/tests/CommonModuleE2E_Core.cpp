#include "CommonModuleE2ETestSupport.h"

namespace common_module_e2e
{
	void RunCommonModuleChecks(Report& report, vega::Application& application)
	{
		using namespace vega;
		const ShipDefinition& shipDefinition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
		const ModuleRulesContext rules{
			*shipDefinition.schematic,
			[](const sas::ContentId& moduleId) { return content::ModuleContentCatalog::FindById(moduleId); },
			content::ModuleContentCatalog::GetZoneTable()
		};
		const std::array<const char*, 11> commonModuleIds{
			"Module.Defense.HeavyArmorPlate.Basic",
			"Module.Offense.AssaultCore.Basic",
			"Module.Primary.FocusedBarrel.Basic",
			"Module.Movement.ThrustBooster.Basic",
			"Module.Offense.AttackRhythm.Basic",
			"Module.Defense.ShatterCurrent.Basic",
			"Module.Offense.EnergyFeedback.Basic",
			"Module.Movement.EscapeReflex.Basic",
			"Module.Offense.FinishingBlow.Basic",
			"Module.Utility.ZoneCooler.Basic",
			"Module.Primary.FullMagazine.Basic"
		};
		for (const char* moduleId : commonModuleIds)
		{
			const ModuleDefinition* module = FindModule(rules, moduleId);
			report.Check(module && !module->sizeVariants.empty(), std::string{ "Common module has a displayable variant: " } + moduleId);
			const std::string description = FormatModuleDescription(*module, module->sizeVariants.front());
			report.Check(description.find('{') == std::string::npos && description.find('}') == std::string::npos,
				std::string{ "Common card formats all numeric tokens: " } + moduleId);
			const TextWidget descriptionText{ application.GetEngineContext(), description, application.GetEngineContext().Style().Font(UIFontRole::Body), 9 };
			const float width = descriptionText.GetBound().size.x;
			report.Check(width <= 376.f, std::string{ "Common card fits one inventory description line: " } + moduleId, width);
		}
		auto level = std::make_shared<HeadlessModuleLevel>(&application);
		level->BeginPlayInternal();
		const shared_ptr<PlayerSpaceShip> source = level->SpawnActor<PlayerSpaceShip>(shipDefinition).lock();
		const shared_ptr<PlayerSpaceShip> target = level->SpawnActor<PlayerSpaceShip>(shipDefinition).lock();
		report.Check(source && target, "real player ship owners spawn");
		level->TickInternal(0.f);
		target->SetActorLocation({ 100000.f, 100000.f });
		source->SetInvulnerability(false);
		target->SetInvulnerability(false);

		ModuleLoadout sourceLoadout;
		ModuleLoadout targetLoadout;
		report.Check(sourceLoadout.SeedDefaults(rules) && targetLoadout.SeedDefaults(rules), "real catalog default loadouts seed");
		const float armorBeforeModules = source->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::Armor);
		const float energyPowerBeforeModules = source->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower);
		const float horizontalSpeedBeforeModules = source->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::MoveSpeedHorizontal);
		const float verticalSpeedBeforeModules = source->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::MoveSpeedVertical);
		const std::uint32_t rhythmId = AddAndInstall(report, sourceLoadout, rules, "Module.Offense.AttackRhythm.Basic");
		const std::uint32_t energyFeedbackId = AddAndInstall(report, sourceLoadout, rules, "Module.Offense.EnergyFeedback.Basic");
		const std::uint32_t armorId = AddAndInstall(report, sourceLoadout, rules, "Module.Defense.HeavyArmorPlate.Basic");
		const std::uint32_t coilId = AddAndInstall(report, sourceLoadout, rules, "Module.Utility.EnergyCoil.Basic");
		const ModuleDefinition* thrustModule = FindModule(rules, "Module.Movement.ThrustBooster.Basic");
		const std::uint32_t thrustId = sourceLoadout.AddToInventory(thrustModule->moduleId);
		const auto thrustPlacementCandidate = FindPlacementWithBestZoneBonus(rules, sourceLoadout, *thrustModule, thrustId);
		report.Check(thrustPlacementCandidate.has_value(), "Thrust Booster finds a legal zone-bonus placement");
		report.Check(sourceLoadout.Install(rules, thrustId, thrustPlacementCandidate->origin, thrustPlacementCandidate->rotationQuarterTurns) ==
			ModulePlacementFailure::None, "Thrust Booster installs through canonical placement rules");
		const std::uint32_t escapeId = AddAndInstall(report, targetLoadout, rules, "Module.Movement.EscapeReflex.Basic");
		report.Check(source->GetModuleApplier().Apply(sourceLoadout, rules) &&
			target->GetModuleApplier().Apply(targetLoadout, rules), "resolved loadouts apply to their actual owners");

		LightYearsAbilitySystemComponent& sourceAbilities = source->GetAbilitySystemComponent();
		const ModuleDefinition* armorModule = FindModule(rules, "Module.Defense.HeavyArmorPlate.Basic");
		const ModuleDefinition* coilModule = FindModule(rules, "Module.Utility.EnergyCoil.Basic");
		const ModuleDefinition* focusedBarrelModule = FindModule(rules, "Module.Primary.FocusedBarrel.Basic");
		ModulePlacement armorPlacement = FindInstalled(sourceLoadout, armorId);
		const ModulePlacement coilPlacement = FindInstalled(sourceLoadout, coilId);
		const ModulePlacement thrustPlacement = FindInstalled(sourceLoadout, thrustId);
		const float armorBase = armorBeforeModules;
		const float energyPowerBase = energyPowerBeforeModules +
			24.f * (1.f + ModuleResolver::GetZoneBonus(rules, *coilModule, coilPlacement));
		const float expectedArmor = armorBase + 30.f * (1.f + ModuleResolver::GetZoneBonus(rules, *armorModule, armorPlacement));
		report.CheckValue("Heavy Armor Plate applies its catalog value", expectedArmor,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::Armor), "Heavy Armor Plate installed in its resolved cell");
		report.CheckValue("Energy Coil applies its catalog value", energyPowerBase,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower), "Energy Coil installed in its resolved cell");
		const float attackPowerBeforeCore = sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackPower);
		const ModuleDefinition* assaultCore = FindModule(rules, "Module.Offense.AssaultCore.Basic");
		report.Check(assaultCore && assaultCore->family == GameplayTags::Ability::Offense &&
			assaultCore->sizeVariants.size() == 1 && assaultCore->sizeVariants.front().shape.cells.size() == 2,
			"Assault Core is a two-cell Offense module alongside the retained Energy Coil");
		const std::uint32_t assaultCoreId = AddAndInstall(report, sourceLoadout, rules, "Module.Offense.AssaultCore.Basic");
		report.Check(source->GetModuleApplier().Apply(sourceLoadout, rules), "Assault Core installs on the real source");
		const ModulePlacement assaultCorePlacement = FindInstalled(sourceLoadout, assaultCoreId);
		report.CheckValue("Assault Core adds zone-scaled +15 attack power",
			attackPowerBeforeCore + 15.f * (1.f + ModuleResolver::GetZoneBonus(rules, *assaultCore, assaultCorePlacement)),
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackPower), "real ASC owner attribute after module install");
		report.CheckValue("Assault Core preserves the separate Energy Coil effect", energyPowerBase,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower), "both content identities coexist");
		report.Check(sourceLoadout.Uninstall(assaultCoreId) && source->GetModuleApplier().Apply(sourceLoadout, rules),
			"Assault Core uninstalls through the same runtime path");
		report.CheckValue("Assault Core removal restores attack power", attackPowerBeforeCore,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackPower), "owner modifier removed");
		const ModuleSizeVariant& thrustVariant = thrustModule->sizeVariants.front();
		const auto findOwnerModifier = [&thrustVariant](const sas::AttributeId& attributeId)
		{
			return std::find_if(thrustVariant.attributeModifiers.begin(), thrustVariant.attributeModifiers.end(),
				[&attributeId](const ModuleAttributeModifier& modifier)
				{
					return modifier.target == ModuleAttributeTarget::Owner && modifier.modifier.attributeId == attributeId;
				});
		};
		const auto horizontalModifier = findOwnerModifier(OwnerAttributeIds::MoveSpeedHorizontal);
		const auto verticalModifier = findOwnerModifier(OwnerAttributeIds::MoveSpeedVertical);
		report.Check(horizontalModifier != thrustVariant.attributeModifiers.end() && verticalModifier != thrustVariant.attributeModifiers.end(),
			"Thrust Booster exposes both owner movement ratings");
		const float thrustZoneBonus = ModuleResolver::GetZoneBonus(rules, *thrustModule, thrustPlacement);
		report.Check(thrustZoneBonus > 0.f, "Thrust Booster resolves a movement or hull zone bonus", thrustZoneBonus);
		const float thrustZoneScaleHorizontal = horizontalModifier->affectedByZoneBonus ? 1.f + thrustZoneBonus : 1.f;
		const float thrustZoneScaleVertical = verticalModifier->affectedByZoneBonus ? 1.f + thrustZoneBonus : 1.f;
		report.CheckValue("Thrust Booster applies horizontal movement rating with its resolved zone",
			horizontalSpeedBeforeModules + horizontalModifier->modifier.magnitude * thrustZoneScaleHorizontal,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::MoveSpeedHorizontal), "Thrust Booster installed at its highest available zone bonus");
		report.CheckValue("Thrust Booster applies vertical movement rating with its resolved zone",
			verticalSpeedBeforeModules + verticalModifier->modifier.magnitude * thrustZoneScaleVertical,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::MoveSpeedVertical), "Thrust Booster installed at its highest available zone bonus");

		GameAbility* primaryAbility = sourceAbilities.GetAbility(sas::AbilitySlot::PrimaryFire);
		const FireWeaponAction* primaryFireAction = nullptr;
		if (primaryAbility)
		{
			for (const AbilityActionSpec& action : primaryAbility->GetDefinition().actions)
			{
				if ((primaryFireAction = std::get_if<FireWeaponAction>(&action.action))) break;
			}
		}
		report.Check(primaryAbility && primaryFireAction, "Common module E2E finds the live PrimaryFire weapon action");
		const PrimaryWeaponDefinition primaryWeaponCopy = primaryFireAction->weaponDefinition;
		const sas::GameplayAttributeList primaryDamageProbe{
			sas::GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f, 10000.f }
		};
		const auto resolvePrimaryDamage = [&sourceAbilities, primaryAbility, &primaryWeaponCopy, &primaryDamageProbe]()
		{
			GameAbilityDefinition abilityDefinition = primaryAbility->GetDefinition();
			AbilityExecutionContext context{ &sourceAbilities, &abilityDefinition, nullptr, nullptr };
			return sas::FindAttributeValue(AbilityActionAttributeResolver::ResolveAttributes(context, &primaryWeaponCopy, primaryDamageProbe),
				CommonAttributeIds::Damage, -1.f);
		};
		const float primaryDamageBeforeFocusedBarrel = resolvePrimaryDamage();
		const std::uint32_t focusedBarrelId = sourceLoadout.AddToInventory(focusedBarrelModule->moduleId);
		const auto focusedBarrelPlacementCandidate = FindPlacementWithBestZoneBonus(rules, sourceLoadout, *focusedBarrelModule, focusedBarrelId);
		report.Check(focusedBarrelPlacementCandidate.has_value(), "Focused Barrel finds a legal zone-bonus placement");
		report.Check(sourceLoadout.Install(rules, focusedBarrelId, focusedBarrelPlacementCandidate->origin,
			focusedBarrelPlacementCandidate->rotationQuarterTurns) == ModulePlacementFailure::None &&
			source->GetModuleApplier().Apply(sourceLoadout, rules), "Focused Barrel applies to the real PrimaryFire loadout");
		report.CheckValue("Focused Barrel preserves pre-impact weapon scaling", primaryDamageBeforeFocusedBarrel, resolvePrimaryDamage(), "whole-hit multiplier is applied at impact");
		report.CheckValue("Focused Barrel multiplies complete Primary hits", 1.3f, source->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, { GameplayTags::Ability::Primary }).damageMultiplier, "Primary impact plan includes Attack Power scaling");
		const float attackSpeedBase = sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackSpeed);
		GameAbility* offenseAbility = sourceAbilities.GetAbility(sas::AbilitySlot::Ability3);
		report.Check(offenseAbility && std::any_of(offenseAbility->GetDefinition().abilityTags.begin(),
			offenseAbility->GetDefinition().abilityTags.end(), [](const GameplayTag& tag)
			{
				return tag.MatchesTag(GameplayTags::Ability::Offense);
			}), "default Ability3 is a real Offense ability");
		const std::uint64_t activationsBefore = sourceAbilities.GetLoadoutAbilityActivationCount();
		report.Check(offenseAbility && offenseAbility->TryActivate(), "real shipped Offense ability activates through its ASC-owned instance");
		level->TickInternal(0.02f);
		const float attackSpeedAfterActivation = sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackSpeed);
		report.Check(sourceAbilities.GetLoadoutAbilityActivationCount() > activationsBefore,
			"Offense ability activated through the ASC lifecycle");
		report.CheckValue("Attack Rhythm adds +15 attack-speed rating", attackSpeedBase + 15.f,
			attackSpeedAfterActivation, "Ability.Offense.SeismicCharge.Basic activated");
		report.Check(source->GetModuleTriggerRuntime().GetActiveBuffCount() == 1 &&
			source->GetModuleTriggerRuntime().GetActiveModifierHandleCount() == 1,
			"Attack Rhythm owns one timed buff and modifier");

		const std::uint32_t duplicateArmorId = sourceLoadout.AddToInventory(armorModule->moduleId);
		report.Check(sourceLoadout.Install(rules, duplicateArmorId, armorPlacement.origin, armorPlacement.rotationQuarterTurns) ==
			ModulePlacementFailure::DuplicateModule, "loadout rejects a second installed copy of one module ID");
		report.Check(sourceLoadout.FindInventoryItem(duplicateArmorId) != nullptr && sourceLoadout.Remove(duplicateArmorId),
			"rejected duplicate remains an ordinary removable inventory item");
		report.CheckValue("Heavy Armor Plate remains applied after duplicate rejection", expectedArmor,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::Armor), "duplicate module ID rejected");

		auto movedArmor = FindPlacement(rules, sourceLoadout, *armorModule, armorId, armorId, armorPlacement.origin);
		report.Check(movedArmor.has_value() && movedArmor->first != armorPlacement.origin,
			"module move finds another legal placement");
		report.Check(sourceLoadout.Move(rules, armorId, movedArmor->first, movedArmor->second) == ModulePlacementFailure::None &&
			source->GetModuleApplier().Apply(sourceLoadout, rules), "moving a module rebuilds the resolved loadout");
		armorPlacement = FindInstalled(sourceLoadout, armorId);
		const float expectedMovedArmor = armorBase + 30.f * (1.f + ModuleResolver::GetZoneBonus(rules, *armorModule, armorPlacement));
		report.CheckValue("moved module resolves its new zone value", expectedMovedArmor,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::Armor), "Heavy Armor Plate moved to another cell");
		report.Check(NearlyEqual(sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackSpeed), attackSpeedAfterActivation) &&
			source->GetModuleTriggerRuntime().GetActiveBuffCount() == 1,
			"unrelated applier rebuild preserves the active timed buff");
		report.Check(sourceLoadout.Uninstall(armorId) && sourceLoadout.FindInventoryItem(armorId) &&
			source->GetModuleApplier().Apply(sourceLoadout, rules), "uninstall returns the same module instance to inventory");
		report.CheckValue("uninstall removes the armor modifier", armorBase,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::Armor), "Heavy Armor Plate returned to inventory");
		report.Check(sourceLoadout.Install(rules, armorId, armorPlacement.origin, armorPlacement.rotationQuarterTurns) ==
			ModulePlacementFailure::None && source->GetModuleApplier().Apply(sourceLoadout, rules),
			"the same inventory copy can be reinstalled");
		level->TickInternal(3.1f);
		report.CheckValue("Attack Rhythm expires after three seconds", attackSpeedBase,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackSpeed), "elapsed 3.1 seconds");
		report.Check(source->GetModuleTriggerRuntime().GetActiveModifierHandleCount() == 0,
			"Attack Rhythm expiry releases its modifier handle");
		const std::uint64_t defenseActivationsBefore = sourceAbilities.GetLoadoutAbilityActivationCount();
		GameAbility* defenseAbility = sourceAbilities.GetAbility(sas::AbilitySlot::Ability4);
		report.Check(defenseAbility && defenseAbility->TryActivate(), "shipped Defense ability activates through its ASC-owned instance");
		level->TickInternal(0.02f);
		report.Check(sourceAbilities.GetLoadoutAbilityActivationCount() > defenseActivationsBefore &&
			source->GetModuleTriggerRuntime().GetActiveBuffCount() == 0 &&
			NearlyEqual(sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackSpeed), attackSpeedBase),
			"Defense ability activation does not trigger Attack Rhythm");

		target->GetShieldComponent().ChangeShield(-target->GetShieldComponent().GetShield());
		ApplyCombatDamage(*target, 2.f, source.get(), { GameplayTags::Damage::Type::Kinetic });
		report.CheckValue("Energy Feedback ignores non-Energy damage", energyPowerBase,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower), "Kinetic damage dealt");
		report.CheckValue("Escape Reflex reacts to positive damage taken", 1.25f,
			target->GetRuntimeModifiers().GetMovementSpeedMultiplier(), "2 points of Kinetic damage received");
		report.Check(target->GetModuleTriggerRuntime().GetActiveBuffCount() == 1,
			"Escape Reflex owns one timed buff");
		level->TickInternal(1.5f);
		ApplyCombatDamage(*target, 2.f, source.get(), { GameplayTags::Damage::Type::Kinetic });
		report.CheckValue("repeated damage refreshes Escape Reflex", 1.25f,
			target->GetRuntimeModifiers().GetMovementSpeedMultiplier(), "second hit 1.5 seconds after first");
		report.Check(target->GetModuleTriggerRuntime().GetActiveBuffCount() == 1,
			"repeated damage refreshes Escape Reflex without stacking");
		level->TickInternal(1.8f);
		report.CheckValue("Escape Reflex remains active before the refreshed deadline", 1.25f,
			target->GetRuntimeModifiers().GetMovementSpeedMultiplier(), "elapsed 1.8 seconds after refreshed hit");
		level->TickInternal(0.3f);
		report.CheckValue("Escape Reflex expires at its refreshed deadline", 1.f,
			target->GetRuntimeModifiers().GetMovementSpeedMultiplier(), "elapsed 2.1 seconds after refreshed hit");

		ApplyCombatDamage(*target, 2.f, source.get(), { GameplayTags::Damage::Type::Energy });
		report.CheckValue("Energy Feedback adds +16 Energy Power on real Energy damage", energyPowerBase + 16.f,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower), "2 points of Energy damage dealt");
		level->TickInternal(1.5f);
		ApplyCombatDamage(*target, 2.f, source.get(), { GameplayTags::Damage::Type::Energy });
		report.CheckValue("repeated Energy triggers refresh without stacking", energyPowerBase + 16.f,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower), "second Energy hit 1.5 seconds after first");
		report.CheckValue("repeat damage refreshes Escape Reflex", 1.25f,
			target->GetRuntimeModifiers().GetMovementSpeedMultiplier(), "second Energy hit received");

		armorPlacement = FindInstalled(sourceLoadout, armorId);
		const auto movedArmorAgain = FindPlacement(rules, sourceLoadout, *armorModule, armorId, armorId, armorPlacement.origin);
		report.Check(movedArmorAgain.has_value() && sourceLoadout.Move(rules, armorId, movedArmorAgain->first, movedArmorAgain->second) ==
			ModulePlacementFailure::None && source->GetModuleApplier().Apply(sourceLoadout, rules),
			"an unrelated module move rebuilds while Energy Feedback is active");
		level->TickInternal(3.8f);
		report.CheckValue("unrelated rebuild keeps the original Energy Feedback deadline", energyPowerBase + 16.f,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower), "3.8 seconds after refreshed hit");
		level->TickInternal(0.3f);
		report.CheckValue("Energy Feedback expires at its refreshed deadline", energyPowerBase,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower), "4.1 seconds after refreshed hit");
		report.Check(source->GetModuleTriggerRuntime().GetActiveBuffCount() == 0,
			"Energy Feedback expiry releases its active buff");

		RemoveModuleOnAttributeChange removeOnBuff{ sourceLoadout, source->GetModuleApplier(), rules, energyFeedbackId };
		const DelegateHandle callbackHandle = sourceAbilities.GetAttributes().onAttributeChanged.BindAction(
			&removeOnBuff, &RemoveModuleOnAttributeChange::OnAttributeChanged);
		ApplyCombatDamage(*target, 2.f, source.get(), { GameplayTags::Damage::Type::Energy });
		sourceAbilities.GetAttributes().onAttributeChanged.UnbindAction(callbackHandle);
		report.Check(removeOnBuff.WasRemoved() && sourceLoadout.FindInventoryItem(energyFeedbackId),
			"synchronous attribute callback removes Energy Feedback during event dispatch");
		report.Check(source->GetModuleTriggerRuntime().GetActiveBuffCount() == 0 &&
			source->GetModuleTriggerRuntime().GetActiveModifierHandleCount() == 0 &&
			NearlyEqual(sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower), energyPowerBase),
			"deferred rule reconciliation leaves no callback-invalidated buff or modifier");

		for (int cycle = 0; cycle < 10; ++cycle)
		{
			const auto freePlacement = FindPlacement(rules, sourceLoadout, *FindModule(rules, "Module.Offense.EnergyFeedback.Basic"), energyFeedbackId);
			report.Check(freePlacement && sourceLoadout.Install(rules, energyFeedbackId, freePlacement->first, freePlacement->second) ==
				ModulePlacementFailure::None && source->GetModuleApplier().Apply(sourceLoadout, rules),
				"trigger handle cycle " + std::to_string(cycle) + " reinstalls existing inventory instance");
			ApplyCombatDamage(*target, 1.f, source.get(), { GameplayTags::Damage::Type::Energy });
			report.Check(source->GetModuleTriggerRuntime().GetActiveBuffCount() == 1 &&
				source->GetModuleTriggerRuntime().GetActiveModifierHandleCount() == 1,
				"trigger handle cycle " + std::to_string(cycle) + " creates one owned modifier");
			report.Check(sourceLoadout.Uninstall(energyFeedbackId) && source->GetModuleApplier().Apply(sourceLoadout, rules) &&
				source->GetModuleTriggerRuntime().GetActiveBuffCount() == 0 &&
				source->GetModuleTriggerRuntime().GetActiveModifierHandleCount() == 0,
				"trigger handle cycle " + std::to_string(cycle) + " removes effects on uninstall");
		}

		const ModuleDefinition* feedbackModule = FindModule(rules, "Module.Offense.EnergyFeedback.Basic");
		const auto rebootPlacement = FindPlacement(rules, sourceLoadout, *feedbackModule, energyFeedbackId);
		report.Check(rebootPlacement && sourceLoadout.Install(rules, energyFeedbackId, rebootPlacement->first, rebootPlacement->second) ==
			ModulePlacementFailure::None && source->GetModuleApplier().Apply(sourceLoadout, rules, true) &&
			source->GetModuleApplier().IsRebooting(energyFeedbackId),
			"in-combat install enters the authored reboot window");
		report.Check(source->GetModuleTriggerRuntime().GetActiveBuffCount() == 0,
			"rebooted module rules stay inactive until the reboot completes");
		level->TickInternal(rules.zoneTable.rebootDurationSeconds - 0.01f);
		report.Check(source->GetModuleApplier().IsRebooting(energyFeedbackId), "reboot remains active before its deadline");
		level->TickInternal(0.02f);
		report.Check(!source->GetModuleApplier().IsRebooting(energyFeedbackId), "reboot resolves at its deadline");
		ApplyCombatDamage(*target, 1.f, source.get(), { GameplayTags::Damage::Type::Energy });
		report.Check(source->GetModuleTriggerRuntime().GetActiveBuffCount() == 1,
			"reinstalled module triggers after reboot completes");
		report.Check(sourceLoadout.Uninstall(energyFeedbackId) && source->GetModuleApplier().Apply(sourceLoadout, rules) &&
			source->GetModuleTriggerRuntime().GetActiveModifierHandleCount() == 0,
			"module removal during an active timer clears the buff immediately");

		shared_ptr<PlayerSpaceShip> lifetimeShip = level->SpawnActor<PlayerSpaceShip>(shipDefinition).lock();
		level->TickInternal(0.f);
		lifetimeShip->SetInvulnerability(false);
		ModuleLoadout lifetimeLoadout;
		report.Check(lifetimeLoadout.SeedDefaults(rules), "lifetime fixture loads real defaults");
		AddAndInstall(report, lifetimeLoadout, rules, "Module.Offense.EnergyFeedback.Basic");
		report.Check(lifetimeShip->GetModuleApplier().Apply(lifetimeLoadout, rules), "lifetime fixture applies Energy Feedback");
		ApplyCombatDamage(*target, 1.f, lifetimeShip.get(), { GameplayTags::Damage::Type::Energy });
		report.Check(lifetimeShip->GetModuleTriggerRuntime().GetActiveBuffCount() == 1,
			"lifetime fixture has an active timed modifier before destruction");
		std::weak_ptr<PlayerSpaceShip> expiredShip = lifetimeShip;
		lifetimeShip->Destroy();
		lifetimeShip.reset();
		level->TickInternal(0.f);
		report.Check(expiredShip.expired(), "destroying the buff owner releases the runtime and its callbacks");

		const shared_ptr<PlayerSpaceShip> damageSource = level->SpawnActor<PlayerSpaceShip>(shipDefinition).lock();
		const shared_ptr<PlayerSpaceShip> damageTarget = level->SpawnActor<PlayerSpaceShip>(shipDefinition).lock();
		level->TickInternal(0.f);
		report.Check(damageSource && damageTarget, "damage-rule fixture uses real player ship owners");
		damageSource->SetActorLocation({ -50000.f, -50000.f });
		damageTarget->SetActorLocation({ -40000.f, -50000.f });
		damageSource->SetInvulnerability(false);
		damageTarget->SetInvulnerability(false);
		ModuleLoadout damageLoadout;
		report.Check(damageLoadout.SeedDefaults(rules), "damage-rule fixture loads the shipped default modules");
		const std::uint32_t shatterId = AddAndInstall(report, damageLoadout, rules, "Module.Defense.ShatterCurrent.Basic");
		const std::uint32_t finishingId = AddAndInstall(report, damageLoadout, rules, "Module.Offense.FinishingBlow.Basic");
		AddAndInstall(report, damageLoadout, rules, "Module.Offense.EnergyFeedback.Basic");
		report.Check(damageSource->GetModuleApplier().Apply(damageLoadout, rules),
			"Shatter Current, Finishing Blow, and Energy Feedback apply to the real source owner");
		damageSource->GetCombatRuntime().SetRuntimeModifier("CommonModules.E2E.Outgoing", CombatRuntimeModifier{ 1.5f });
		DamageCapture damageCapture;
		const DelegateHandle damageCaptureHandle = damageSource->GetCombatRuntime().onDamageDealt.BindAction(
			&damageCapture, &DamageCapture::OnDamage);
		DamagePayload guaranteedCritical;
		guaranteedCritical.criticalPolicy = DamageCriticalPolicy::Guaranteed;
		const List<GameplayTag> primaryTags{ GameplayTags::Ability::Primary };
		const List<GameplayTag> offenseTags{ GameplayTags::Ability::Offense };
		damageTarget->GetShieldComponent().ChangeShield(-damageTarget->GetShieldComponent().GetShield());
		damageTarget->GetHealthComponent().SetInitialHealth(260.f, 1000.f);
		ApplyCombatDamage(*damageTarget, 10.f, damageSource.get(), { GameplayTags::Damage::Type::Kinetic },
			guaranteedCritical, {}, primaryTags);
		report.Check(damageCapture.records.size() == 1, "Finishing Blow does not trigger above 25 percent health",
			Json{ { "damageEvents", damageCapture.records.size() } });
		report.CheckValue("Finishing Blow leaves a 26 percent target unchanged", 22.5f,
			damageCapture.records.front().originalDamage, "raw 10 x outgoing 1.5 x guaranteed critical 1.5 at 26 percent");
		damageCapture.records.clear();
		damageTarget->GetHealthComponent().SetInitialHealth(250.f, 1000.f);
		ApplyCombatDamage(*damageTarget, 10.f, damageSource.get(), { GameplayTags::Damage::Type::Kinetic },
			guaranteedCritical, {}, primaryTags);
		report.Check(damageCapture.records.size() == 1, "Finishing Blow boundary hit has no Shatter bonus before shield break",
			Json{ { "damageEvents", damageCapture.records.size() } });
		report.CheckValue("Finishing Blow activates at exactly 25 percent health", 27.f,
			damageCapture.records.front().originalDamage, "raw 10 x module 1.2 x outgoing 1.5 x guaranteed critical 1.5 at 25 percent");
		damageCapture.records.clear();
		damageTarget->GetHealthComponent().SetInitialHealth(240.f, 1000.f);
		ApplyCombatDamage(*damageTarget, 10.f, damageSource.get(), { GameplayTags::Damage::Type::Kinetic },
			guaranteedCritical, {}, primaryTags);
		report.CheckValue("Finishing Blow stays active below its health threshold", 27.f,
			damageCapture.records.front().originalDamage, "raw 10 x module 1.2 x outgoing 1.5 x guaranteed critical 1.5 at 24 percent");

		ShieldComponent& sourceShield = damageSource->GetShieldComponent();
		sourceShield.SetMaxShield(100.f);
		sourceShield.ChangeShield(100.f);
		sourceShield.ChangeShield(-100.f);
		damageTarget->GetHealthComponent().SetInitialHealth(800.f, 1000.f);
		damageCapture.records.clear();
		DamagePayload nonCritical;
		nonCritical.criticalPolicy = DamageCriticalPolicy::Disabled;
		ApplyCombatDamage(*damageTarget, 10.f, damageSource.get(), { GameplayTags::Damage::Type::Kinetic },
			nonCritical, {}, primaryTags);
		report.Check(damageCapture.records.size() == 1, "ChangeShield to zero does not arm Shatter Current",
			Json{ { "damageEvents", damageCapture.records.size() } });
		report.CheckValue("manual shield correction adds no Energy damage", 15.f,
			damageCapture.records.front().originalDamage, "raw 10 x outgoing 1.5; shield set to zero without a damage hit");

		sourceShield.ChangeShield(100.f);
		ApplyCombatDamage(*damageSource, 101.f, damageTarget.get(), { GameplayTags::Damage::Type::Kinetic }, nonCritical);
		report.CheckValue("real shield damage breaks the source shield", 0.f, sourceShield.GetShield(), "101 incoming Kinetic against 100 shield");
		damageTarget->GetHealthComponent().SetInitialHealth(240.f, 1000.f);
		const float energyPowerBeforeBonus = damageSource->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower);
		damageCapture.records.clear();
		ApplyCombatDamage(*damageTarget, 10.f, damageSource.get(), { GameplayTags::Damage::Type::Kinetic },
			guaranteedCritical, {}, primaryTags);
		report.Check(damageCapture.records.size() == 2, "a shield break gives the next Primary hit one Energy secondary",
			Json{ { "damageEvents", damageCapture.records.size() } });
		report.CheckValue("main hit applies Finishing Blow and the source multiplier once", 27.f,
			damageCapture.records[0].originalDamage, "raw 10 x Finishing Blow 1.2 x outgoing 1.5 x critical 1.5");
		report.CheckValue("Shatter bonus uses raw damage, Finishing Blow, and outgoing once without critical", 5.4f,
			damageCapture.records[1].originalDamage, "raw 10 x Finishing Blow 1.2 x bonus 0.30 x outgoing 1.5");
		report.Check(damageCapture.records[0].wasCritical && !damageCapture.records[1].wasCritical,
			"the Primary hit can critical while the Energy secondary cannot");
		report.Check(damageCapture.records[1].damageTags.size() == 1 &&
			damageCapture.records[1].damageTags.front() == GameplayTags::Damage::Type::Energy &&
			std::any_of(damageCapture.records[1].sourceAbilityTags.begin(), damageCapture.records[1].sourceAbilityTags.end(),
				[](const GameplayTag& tag) { return tag.MatchesTag(GameplayTags::Ability::Primary); }) &&
			damageCapture.records[1].skippedModuleRules,
			"secondary keeps Primary source tags, carries only Energy damage, and cannot recurse");
		report.CheckValue("Energy secondary activates Energy Feedback through normal damage events", energyPowerBeforeBonus + 16.f,
			damageSource->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower),
			"one positive SourceDamageDealt Energy event");

		damageCapture.records.clear();
		damageTarget->GetHealthComponent().SetInitialHealth(800.f, 1000.f);
		ApplyCombatDamage(*damageTarget, 10.f, damageSource.get(), { GameplayTags::Damage::Type::Kinetic },
			nonCritical, {}, offenseTags);
		report.Check(damageCapture.records.size() == 1 && damageCapture.records.front().damageTags.size() == 1 &&
			damageCapture.records.front().damageTags.front() == GameplayTags::Damage::Type::Kinetic,
			"Shatter Current does not add bonus damage to non-Primary hits",
			Json{ { "damageEvents", damageCapture.records.size() } });

		level->TickInternal(4.5f);
		const ModuleDefinition* finishingModule = FindModule(rules, "Module.Offense.FinishingBlow.Basic");
		const ModulePlacement finishingPlacement = FindInstalled(damageLoadout, finishingId);
		const auto movedFinishing = FindPlacement(rules, damageLoadout, *finishingModule, finishingId,
			finishingId, finishingPlacement.origin);
		report.Check(movedFinishing && damageLoadout.Move(rules, finishingId, movedFinishing->first, movedFinishing->second) ==
			ModulePlacementFailure::None && damageSource->GetModuleApplier().Apply(damageLoadout, rules),
			"an unrelated Finishing Blow move rebuilds while Shatter Current is active");
		damageCapture.records.clear();
		ApplyCombatDamage(*damageTarget, 10.f, damageSource.get(), { GameplayTags::Damage::Type::Kinetic },
			nonCritical, {}, primaryTags);
		report.Check(damageCapture.records.size() == 2,
			"unchanged Shatter Current preserves its remaining window across an applier rebuild",
			Json{ { "damageEvents", damageCapture.records.size() } });
		report.CheckValue("preserved Shatter window adds one noncritical Energy hit", 4.5f,
			damageCapture.records[1].originalDamage, "10 x 0.30 x outgoing 1.5 with 0.5 seconds remaining");
		level->TickInternal(0.6f);
		damageCapture.records.clear();
		ApplyCombatDamage(*damageTarget, 10.f, damageSource.get(), { GameplayTags::Damage::Type::Kinetic },
			nonCritical, {}, primaryTags);
		report.Check(damageCapture.records.size() == 1, "Shatter Current window expires after five seconds",
			Json{ { "damageEvents", damageCapture.records.size() } });

		sourceShield.ChangeShield(100.f);
		ApplyCombatDamage(*damageSource, 101.f, damageTarget.get(), { GameplayTags::Damage::Type::Kinetic }, nonCritical);
		RemoveShatterCurrentOnPrimaryDamage removeShatter{ damageLoadout, damageSource->GetModuleApplier(), rules, shatterId };
		const DelegateHandle removeShatterHandle = damageSource->GetCombatRuntime().onDamageDealt.BindAction(
			&removeShatter, &RemoveShatterCurrentOnPrimaryDamage::OnDamage);
		damageCapture.records.clear();
		ApplyCombatDamage(*damageTarget, 10.f, damageSource.get(), { GameplayTags::Damage::Type::Kinetic },
			nonCritical, {}, primaryTags);
		damageSource->GetCombatRuntime().onDamageDealt.UnbindAction(removeShatterHandle);
		report.Check(removeShatter.WasRemoved() && damageLoadout.FindInventoryItem(shatterId) &&
			damageCapture.records.size() == 2 && damageCapture.records[1].damageTags.size() == 1 &&
			damageCapture.records[1].damageTags.front() == GameplayTags::Damage::Type::Energy,
			"main-hit callback can remove Shatter Current while its copied bonus still resolves");
		damageCapture.records.clear();
		ApplyCombatDamage(*damageTarget, 10.f, damageSource.get(), { GameplayTags::Damage::Type::Kinetic },
			nonCritical, {}, primaryTags);
		report.Check(damageCapture.records.size() == 1,
			"removing Shatter Current clears its rule immediately after the in-flight hit");

		const auto rebootShatter = FindPlacement(rules, damageLoadout, *FindModule(rules, "Module.Defense.ShatterCurrent.Basic"), shatterId);
		report.Check(rebootShatter && damageLoadout.Install(rules, shatterId, rebootShatter->first, rebootShatter->second) ==
			ModulePlacementFailure::None && damageSource->GetModuleApplier().Apply(damageLoadout, rules, true) &&
			damageSource->GetModuleApplier().IsRebooting(shatterId),
			"reinstalling Shatter Current in combat starts its normal reboot");
		damageCapture.records.clear();
		ApplyCombatDamage(*damageTarget, 10.f, damageSource.get(), { GameplayTags::Damage::Type::Kinetic },
			nonCritical, {}, primaryTags);
		report.Check(damageCapture.records.size() == 1, "reboot suppresses Shatter Current bonus rules immediately",
			Json{ { "damageEvents", damageCapture.records.size() } });
		level->TickInternal(rules.zoneTable.rebootDurationSeconds + 0.01f);
		damageCapture.records.clear();
		ApplyCombatDamage(*damageTarget, 10.f, damageSource.get(), { GameplayTags::Damage::Type::Kinetic },
			nonCritical, {}, primaryTags);
		report.Check(damageCapture.records.size() == 1,
			"finishing a module reboot does not restore a previously active Shatter window");

		report.Check(damageLoadout.Uninstall(shatterId) && damageLoadout.Uninstall(finishingId) &&
			damageSource->GetModuleApplier().Apply(damageLoadout, rules), "removing both damage-rule modules restores the baseline pipeline");
		damageCapture.records.clear();
		damageTarget->GetHealthComponent().SetInitialHealth(800.f, 1000.f);
		ApplyCombatDamage(*damageTarget, 10.f, damageSource.get(), { GameplayTags::Damage::Type::Kinetic },
			guaranteedCritical, {}, primaryTags);
		report.Check(damageCapture.records.size() == 1, "no-rule path produces only its original damage event",
			Json{ { "damageEvents", damageCapture.records.size() } });
		report.CheckValue("no-rule path preserves the original critical and outgoing multiplier", 22.5f,
			damageCapture.records.front().originalDamage, "raw 10 x outgoing 1.5 x guaranteed critical 1.5");

		const auto destroyFixtureShatter = FindPlacement(rules, damageLoadout,
			*FindModule(rules, "Module.Defense.ShatterCurrent.Basic"), shatterId);
		report.Check(destroyFixtureShatter && damageLoadout.Install(rules, shatterId, destroyFixtureShatter->first,
			destroyFixtureShatter->second) == ModulePlacementFailure::None &&
			damageSource->GetModuleApplier().Apply(damageLoadout, rules), "Shatter Current can be restored for destruction coverage");
		sourceShield.ChangeShield(100.f);
		ApplyCombatDamage(*damageSource, 101.f, damageTarget.get(), { GameplayTags::Damage::Type::Kinetic }, nonCritical);
		DestroyTargetOnPrimaryDamage destroyTarget{ *damageTarget };
		const DelegateHandle destroyTargetHandle = damageSource->GetCombatRuntime().onDamageDealt.BindAction(
			&destroyTarget, &DestroyTargetOnPrimaryDamage::OnDamage);
		damageCapture.records.clear();
		ApplyCombatDamage(*damageTarget, 10.f, damageSource.get(), { GameplayTags::Damage::Type::Kinetic },
			nonCritical, {}, primaryTags);
		damageSource->GetCombatRuntime().onDamageDealt.UnbindAction(destroyTargetHandle);
		report.Check(damageTarget->GetIsPendingDestroy() && damageCapture.records.size() == 1,
			"destroying the hit target from its main-hit callback suppresses the Energy secondary",
			Json{ { "damageEvents", damageCapture.records.size() } });
		damageSource->GetCombatRuntime().onDamageDealt.UnbindAction(damageCaptureHandle);
		damageSource->Destroy();
		damageTarget->Destroy();
		(void)rhythmId;
		(void)escapeId;
		source->Destroy();
		target->Destroy();
		level->TickInternal(0.f);
	}


}
