#include "framework/Application.h"
#include "framework/AssetManager.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/module/ModulePlacementRules.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/tags/GameplayTags.h"
#include "level/GameLevel.h"
#include "player/PlayerSpaceShip.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
	using Json = nlohmann::json;

	class HeadlessModuleLevel final : public ly::GameLevel
	{
	public:
		explicit HeadlessModuleLevel(ly::Application* application) : GameLevel{ application } {}

	protected:
		void InitializeLevelSystems() override {}
		void CreateGameHUD() override {}
		void CreateHUDControllers() override {}
		void OnGameStart() override {}
	};

	struct Report
	{
		Json artifact{ { "test", "CommonModuleE2E" }, { "passed", false }, { "checks", Json::array() } };

		void Check(bool passed, const std::string& name, Json actual = nullptr)
		{
			Json check{ { "name", name }, { "passed", passed } };
			if (!actual.is_null()) check["actual"] = std::move(actual);
			artifact["checks"].push_back(std::move(check));
			if (!passed) throw std::runtime_error{ name };
		}

		void CheckValue(const std::string& name, float expected, float actual, const std::string& input)
		{
			Json check{ { "name", name }, { "passed", std::abs(expected - actual) < 0.001f },
				{ "input", input }, { "expected", expected }, { "actual", actual } };
			artifact["checks"].push_back(std::move(check));
			if (!artifact["checks"].back()["passed"].get<bool>()) throw std::runtime_error{ name };
		}
	};

	bool NearlyEqual(float left, float right)
	{
		return std::abs(left - right) < 0.001f;
	}

	const ly::ModuleDefinition* FindModule(const ly::ModuleRulesContext& rules, const char* moduleId)
	{
		return rules.lookup ? rules.lookup(moduleId) : nullptr;
	}

	std::optional<std::pair<sf::Vector2i, int>> FindPlacement(
		const ly::ModuleRulesContext& rules,
		const ly::ModuleLoadout& loadout,
		const ly::ModuleDefinition& module,
		std::uint32_t instanceId,
		std::uint32_t ignoredInstanceId = 0,
		std::optional<sf::Vector2i> excludedOrigin = std::nullopt
	)
	{
		ly::ModulePlacement candidate;
		candidate.instanceId = instanceId;
		candidate.moduleId = module.moduleId;
		for (int rotation = 0; rotation < 4; ++rotation)
		{
			for (int y = 0; y < rules.schematic.height; ++y)
			{
				for (int x = 0; x < rules.schematic.width; ++x)
				{
					candidate.origin = { x, y };
					candidate.rotationQuarterTurns = rotation;
					if (excludedOrigin && candidate.origin == *excludedOrigin) continue;
					if (ly::ModulePlacementRules::CanPlace(rules, loadout, candidate, ignoredInstanceId) ==
						ly::ModulePlacementFailure::None)
					{
						return std::make_pair(candidate.origin, rotation);
					}
				}
			}
		}
		return std::nullopt;
	}

	std::uint32_t AddAndInstall(
		Report& report,
		ly::ModuleLoadout& loadout,
		const ly::ModuleRulesContext& rules,
		const char* moduleId
	)
	{
		const ly::ModuleDefinition* module = FindModule(rules, moduleId);
		report.Check(module != nullptr, std::string{ "catalog contains " } + moduleId);
		const std::uint32_t instanceId = loadout.AddToInventory(module->moduleId);
		const auto placement = FindPlacement(rules, loadout, *module, instanceId);
		report.Check(placement.has_value(), std::string{ "free schematic cell for " } + moduleId);
		report.Check(loadout.Install(rules, instanceId, placement->first, placement->second) ==
			ly::ModulePlacementFailure::None, std::string{ "install " } + moduleId);
		return instanceId;
	}

	ly::ModulePlacement FindInstalled(const ly::ModuleLoadout& loadout, std::uint32_t instanceId)
	{
		const ly::ModulePlacement* placement = loadout.FindPlacement(instanceId);
		if (!placement) throw std::runtime_error{ "installed module instance disappeared" };
		return *placement;
	}

	class RemoveModuleOnAttributeChange final
	{
	public:
		RemoveModuleOnAttributeChange(ly::ModuleLoadout& loadout, ly::ShipModuleApplier& applier,
			const ly::ModuleRulesContext& rules, std::uint32_t instanceId)
			: mLoadout{ loadout }, mApplier{ applier }, mRules{ rules }, mInstanceId{ instanceId }
		{
		}

		void OnAttributeChanged(sas::AttributeId id, float previousValue, float value)
		{
			if (mRemoved || id != ly::OwnerAttributeIds::EnergyPower || value <= previousValue) return;
			mRemoved = mLoadout.Uninstall(mInstanceId) && mApplier.Apply(mLoadout, mRules);
		}

		bool WasRemoved() const { return mRemoved; }

	private:
		ly::ModuleLoadout& mLoadout;
		ly::ShipModuleApplier& mApplier;
		const ly::ModuleRulesContext& mRules;
		std::uint32_t mInstanceId;
		bool mRemoved = false;
	};

	void RunCommonModuleChecks(Report& report, ly::Application& application)
	{
		using namespace ly;
		const ShipDefinition& shipDefinition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
		const ModuleRulesContext rules{
			*shipDefinition.schematic,
			[](const sas::ContentId& moduleId) { return content::ModuleContentCatalog::FindById(moduleId); },
			content::ModuleContentCatalog::GetZoneTable()
		};
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
		const std::uint32_t rhythmId = AddAndInstall(report, sourceLoadout, rules, "Module.Offense.AttackRhythm.Basic");
		const std::uint32_t energyFeedbackId = AddAndInstall(report, sourceLoadout, rules, "Module.Offense.EnergyFeedback.Basic");
		const std::uint32_t armorId = AddAndInstall(report, sourceLoadout, rules, "Module.Defense.HeavyArmorPlate.Basic");
		const std::uint32_t coilId = AddAndInstall(report, sourceLoadout, rules, "Module.Utility.EnergyCoil.Basic");
		const std::uint32_t escapeId = AddAndInstall(report, targetLoadout, rules, "Module.Movement.EscapeReflex.Basic");
		report.Check(source->GetModuleApplier().Apply(sourceLoadout, rules) &&
			target->GetModuleApplier().Apply(targetLoadout, rules), "resolved loadouts apply to their actual owners");

		LightYearsAbilitySystemComponent& sourceAbilities = source->GetAbilitySystemComponent();
		const ModuleDefinition* armorModule = FindModule(rules, "Module.Defense.HeavyArmorPlate.Basic");
		const ModuleDefinition* coilModule = FindModule(rules, "Module.Utility.EnergyCoil.Basic");
		ModulePlacement armorPlacement = FindInstalled(sourceLoadout, armorId);
		const ModulePlacement coilPlacement = FindInstalled(sourceLoadout, coilId);
		const float armorBase = armorBeforeModules;
		const float energyPowerBase = energyPowerBeforeModules +
			24.f * (1.f + ModuleResolver::GetZoneBonus(rules, *coilModule, coilPlacement));
		const float expectedArmor = armorBase + 30.f * (1.f + ModuleResolver::GetZoneBonus(rules, *armorModule, armorPlacement));
		report.CheckValue("Heavy Armor Plate applies its catalog value", expectedArmor,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::Armor), "Heavy Armor Plate installed in its resolved cell");
		report.CheckValue("Energy Coil applies its catalog value", energyPowerBase,
			sourceAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower), "Energy Coil installed in its resolved cell");
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
		(void)rhythmId;
		(void)escapeId;
		source->Destroy();
		target->Destroy();
		level->TickInternal(0.f);
	}

	void WriteArtifact(const std::filesystem::path& path, const Json& artifact)
	{
		if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
		std::ofstream output{ path, std::ios::out | std::ios::trunc };
		if (!output) throw std::runtime_error{ "Could not open Common Module E2E artifact path" };
		output << artifact.dump(2) << '\n';
	}
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: CommonModuleE2ETests <artifact-path>\n";
		return 2;
	}

	Report report;
	const std::filesystem::path artifactPath{ argv[1] };
	try
	{
		ly::AssetManager::GetAssetManager().SetAssetRootDirectory(
			(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"
		);
		if (!ly::GameContentBootstrap::Register()) throw std::runtime_error{ "GameContentBootstrap::Register failed" };
		std::string windowTitle = "Common modules E2E";
		ly::Application application{ sf::Vector2u{ 64u, 64u }, 32u, windowTitle, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		RunCommonModuleChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception)
	{
		report.artifact["error"] = exception.what();
		std::cerr << "Common module E2E failed: " << exception.what() << '\n';
	}

	try { WriteArtifact(artifactPath, report.artifact); }
	catch (const std::exception& exception)
	{
		std::cerr << "Could not write Common Module E2E artifact: " << exception.what() << '\n';
		return 1;
	}
	std::cout << "E2E artifact: " << std::filesystem::absolute(artifactPath).string() << '\n';
	return report.artifact.value("passed", false) ? 0 : 1;
}
