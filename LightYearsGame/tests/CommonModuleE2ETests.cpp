#include "framework/Application.h"
#include "framework/AssetManager.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/content/GameAbilityActions.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/module/ModulePlacementRules.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h"
#include "gameplay/tags/GameplayTags.h"
#include "level/GameLevel.h"
#include "player/PlayerSpaceShip.h"
#include "presentation/hud/modules/ModuleScreenPresenter.h"
#include "widget/TextWidget.h"
#include "widget/UIStyle.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
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

	std::optional<ly::ModulePlacement> FindPlacementWithBestZoneBonus(
		const ly::ModuleRulesContext& rules,
		const ly::ModuleLoadout& loadout,
		const ly::ModuleDefinition& module,
		std::uint32_t instanceId
	)
	{
		ly::ModulePlacement candidate;
		candidate.instanceId = instanceId;
		candidate.moduleId = module.moduleId;
		std::optional<ly::ModulePlacement> bestPlacement;
		float bestBonus = -1.f;
		for (int rotation = 0; rotation < 4; ++rotation)
		{
			for (int y = 0; y < rules.schematic.height; ++y)
			{
				for (int x = 0; x < rules.schematic.width; ++x)
				{
					candidate.origin = { x, y };
					candidate.rotationQuarterTurns = rotation;
					if (ly::ModulePlacementRules::CanPlace(rules, loadout, candidate) != ly::ModulePlacementFailure::None) continue;
					const float bonus = ly::ModuleResolver::GetZoneBonus(rules, module, candidate);
					if (bonus > bestBonus)
					{
						bestBonus = bonus;
						bestPlacement = candidate;
					}
				}
			}
		}
		return bestPlacement;
	}

	std::optional<std::pair<sf::Vector2i, int>> FindPlacementWithCooldownZone(
		const ly::ModuleRulesContext& rules,
		const ly::ModuleLoadout& loadout,
		const ly::ModuleDefinition& module,
		std::uint32_t instanceId,
		const ly::GameplayTag& zone,
		std::uint32_t ignoredInstanceId = 0
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
					if (ly::ModulePlacementRules::CanPlace(rules, loadout, candidate, ignoredInstanceId) !=
						ly::ModulePlacementFailure::None) continue;
					const ly::ModuleSizeVariant& variant = module.sizeVariants[static_cast<std::size_t>(candidate.variantIndex)];
					const ly::List<sf::Vector2i> cells = ly::ModuleShapeRules::Rotate(variant.shape.cells, rotation);
					const bool fitsZone = std::all_of(cells.begin(), cells.end(), [&rules, &candidate, &zone](const sf::Vector2i& cell)
					{
						return rules.schematic.GetZoneAt(candidate.origin.x + cell.x, candidate.origin.y + cell.y) == zone;
					});
					if (!fitsZone) continue;

					ly::List<ly::ModulePlacement> placements = loadout.GetPlacements();
					placements.erase(std::remove_if(placements.begin(), placements.end(),
						[instanceId](const ly::ModulePlacement& placement) { return placement.instanceId == instanceId; }), placements.end());
					placements.push_back(candidate);
					const auto effects = ly::ModuleResolver::Resolve(rules, placements);
					const bool resolvesZone = std::any_of(effects.cooldownRules.begin(), effects.cooldownRules.end(),
						[instanceId, &zone](const ly::ResolvedModuleCooldownRule& rule)
						{
							return rule.instanceId == instanceId && rule.zone == zone;
						});
					if (zone == ly::GameplayTag{ "Module.Zone.Nose" } ? !resolvesZone : resolvesZone)
						return std::make_pair(candidate.origin, rotation);
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

	struct CapturedDamage
	{
		float originalDamage = 0.f;
		bool wasCritical = false;
		ly::List<ly::GameplayTag> damageTags;
		ly::List<ly::GameplayTag> sourceAbilityTags;
		bool skippedModuleRules = false;
	};

	class DamageCapture final
	{
	public:
		void OnDamage(const ly::DamageContext& context)
		{
			records.push_back({ context.originalDamage, context.wasCritical, context.damageTags,
				context.sourceAbilityTags, context.payload.skipModuleDamageRules });
		}

		ly::List<CapturedDamage> records;
	};

	class RemoveShatterCurrentOnPrimaryDamage final
	{
	public:
		RemoveShatterCurrentOnPrimaryDamage(ly::ModuleLoadout& loadout, ly::ShipModuleApplier& applier,
			const ly::ModuleRulesContext& rules, std::uint32_t instanceId)
			: mLoadout{ loadout }, mApplier{ applier }, mRules{ rules }, mInstanceId{ instanceId }
		{
		}

		void OnDamage(const ly::DamageContext& context)
		{
			if (mRemoved || context.payload.skipModuleDamageRules ||
				!std::any_of(context.sourceAbilityTags.begin(), context.sourceAbilityTags.end(),
					[](const ly::GameplayTag& tag) { return tag.MatchesTag(ly::GameplayTags::Ability::Primary); })) return;
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

	class DestroyTargetOnPrimaryDamage final
	{
	public:
		explicit DestroyTargetOnPrimaryDamage(ly::Actor& target) : mTarget{ target } {}

		void OnDamage(const ly::DamageContext& context)
		{
			if (!context.payload.skipModuleDamageRules && std::any_of(context.sourceAbilityTags.begin(), context.sourceAbilityTags.end(),
				[](const ly::GameplayTag& tag) { return tag.MatchesTag(ly::GameplayTags::Ability::Primary); })) mTarget.Destroy();
		}

	private:
		ly::Actor& mTarget;
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
			const TextWidget descriptionText{ description, UIStyle::Get().Font(UIFontRole::Body), 9 };
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
		const ModulePlacement focusedBarrelPlacement = FindInstalled(sourceLoadout, focusedBarrelId);
		const ModuleAttributeModifier& focusedBarrelModifier = focusedBarrelModule->sizeVariants.front().attributeModifiers.front();
		const float focusedBarrelZoneScale = focusedBarrelModifier.affectedByZoneBonus
			? 1.f + ModuleResolver::GetZoneBonus(rules, *focusedBarrelModule, focusedBarrelPlacement)
			: 1.f;
		const float focusedBarrelEffectiveMultiplier = 1.f +
			(focusedBarrelModifier.modifier.magnitude - 1.f) * focusedBarrelZoneScale;
		report.CheckValue("Focused Barrel changes resolved PrimaryFire damage by its zone-scaled catalog modifier",
			primaryDamageBeforeFocusedBarrel + 10.f * (focusedBarrelEffectiveMultiplier - 1.f),
			resolvePrimaryDamage(), "PrimaryFire FireWeaponAction resolved before and after Focused Barrel install");
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

	void RunCooldownModuleChecks(Report& report, ly::Application& application)
	{
		using namespace ly;
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
		report.Check(nose.has_value(), "Regional Cooler has a legal Nose placement");
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

		const auto lifeSupportPlacement = std::find_if(loadout.GetPlacements().begin(), loadout.GetPlacements().end(),
			[](const ModulePlacement& placement) { return placement.moduleId == "Module.Utility.LifeSupportUnit.Standard"; });
		report.Check(lifeSupportPlacement != loadout.GetPlacements().end(), "fixture finds the default Hull Life Support module");
		const std::uint32_t lifeSupportId = lifeSupportPlacement == loadout.GetPlacements().end() ? 0 : lifeSupportPlacement->instanceId;
		report.Check(lifeSupportId != 0 && loadout.Uninstall(lifeSupportId) &&
			ship->GetModuleApplier().Apply(loadout, rules), "removing default Life Support frees a legal Hull row");
		const auto hull = FindPlacementWithCooldownZone(rules, loadout, *cooler, coolerId,
			GameplayTag{ "Module.Zone.Hull" }, coolerId);
		report.Check(hull.has_value(), "Regional Cooler has a legal Hull placement");
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

	void RunFullMagazineChecks(Report& report, ly::Application& application)
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
		report.Check(source && target, "Full Magazine fixture uses real player ship owners");
		level->TickInternal(0.f);
		source->SetActorLocation({ -50000.f, -50000.f });
		target->SetActorLocation({ -40000.f, -50000.f });
		source->SetInvulnerability(false);
		target->SetInvulnerability(false);
		target->SetCollisionLayer(CollisionLayer::Enemy);
		target->SetCollisionMask(CollisionLayer::PlayerBullet);
		target->GetHealthComponent().SetMaxHealth(5000.f, true);

		ModuleLoadout loadout;
		report.Check(loadout.SeedDefaults(rules), "Full Magazine fixture seeds the real default loadout");
		const std::uint32_t fullMagazineId = AddAndInstall(report, loadout, rules, "Module.Primary.FullMagazine.Basic");
		report.Check(source->GetModuleApplier().Apply(loadout, rules), "Full Magazine applies to the real source combat runtime");
		const ModuleDefinition* module = FindModule(rules, "Module.Primary.FullMagazine.Basic");
		report.Check(module && !module->sizeVariants.empty() && module->sizeVariants.front().damageRules.size() == 1,
			"catalog resolves one typed FullMagazineShot rule");
		const ModuleDamageRule& rule = module->sizeVariants.front().damageRules.front();
		report.Check(rule.condition == ModuleDamageCondition::FullMagazineShot,
			"Full Magazine loader preserves its typed shot condition");
		report.Check(rule.openingShotCount == 3, "Full Magazine loads a three-shot opening window");
		report.CheckValue("Full Magazine resolves its authored 1.3 multiplier", 1.3f,
			source->GetCombatRuntime().ResolveFullMagazineShotMultiplier(0), "M11 installed on the real source");
		report.CheckValue("generic outgoing rules exclude FullMagazineShot", 1.f,
			source->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, { GameplayTags::Ability::Primary }).damageMultiplier,
			"the full-magazine multiplier is snapshotted before impact");
		const ModulePlacement installedModule = FindInstalled(loadout, fullMagazineId);
		const std::uint32_t duplicateId = loadout.AddToInventory(module->moduleId);
		report.Check(loadout.Install(rules, duplicateId, installedModule.origin, installedModule.rotationQuarterTurns) ==
			ModulePlacementFailure::DuplicateModule && loadout.Remove(duplicateId),
			"loadout rejects a second installed copy of Full Magazine");

		const PrimaryWeaponDefinition* fighterCatalog = content::WeaponContentCatalog::FindById("Weapon.Projectile.FighterRapidLaser.Basic");
		report.Check(fighterCatalog && fighterCatalog->magazine.has_value(), "Fighter laser catalog weapon has a magazine");
		const PrimaryWeaponDefinition fighter = *fighterCatalog;
		sas::GameplayAttributeList fighterAttributes = fighter.attributes;
		PrimaryWeaponExecutionContext fighterContext{ *source, fighter, fighterAttributes };
		fighterContext.damageTags = fighter.damageTags;
		bool canContinue = true;
		fighterContext.shouldContinue = [&canContinue]() { return canContinue; };
		PrimaryWeaponRuntimeState fighterRuntime;
		report.Check(PrimaryWeaponExecutionSystem::InitializeRuntime(fighter, fighterRuntime).isValid,
			"Fighter laser initializes its real magazine runtime");
		PrimaryWeaponExecutionSystem::BeginFire(fighterContext, fighterRuntime);
		const int fullCapacity = fighter.magazine->capacity;
		canContinue = false;
		report.Check(!PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime) &&
			fighterRuntime.magazineState.roundsRemaining == fullCapacity && fighterRuntime.magazineState.shotsFired == 0,
			"failed fire leaves a full magazine eligible for the next shot");
		canContinue = true;

		const float fighterBaseDamage = sas::FindAttributeValue(fighterAttributes, CommonAttributeIds::Damage, 0.f);
		auto projectileCount = [&level]()
		{
			return level->GetActorsByType<PrimaryWeaponProjectileActor>().size();
		};
		auto latestProjectile = [&level]() -> shared_ptr<PrimaryWeaponProjectileActor>
		{
			const auto projectiles = level->GetActorsByType<PrimaryWeaponProjectileActor>();
			return projectiles.empty() ? shared_ptr<PrimaryWeaponProjectileActor>{} : projectiles.back().lock();
		};
		auto firstProjectile = shared_ptr<PrimaryWeaponProjectileActor>{};
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime),
			"full-magazine Fighter shot fires through PrimaryWeaponExecutionSystem");
		level->TickInternal(0.f);
		firstProjectile = latestProjectile();
		report.Check(firstProjectile != nullptr, "full-magazine shot spawns a real delayed projectile");
		report.CheckValue("first full-magazine projectile snapshots x1.3", 1.3f,
			firstProjectile->GetShotMetadata().shotDamageMultiplier, "48 of 48 rounds before FireOnce debit");
		report.CheckValue("first full-magazine shot deals 30 percent more damage", fighterBaseDamage * 1.3f,
			firstProjectile->GetDamage(), "projectile damage captured at spawn");
		report.Check(fighterRuntime.magazineState.roundsRemaining == fullCapacity - 1,
			"successful first shot spends ammo after taking its snapshot");

		const std::size_t beforeSecondShot = projectileCount();
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime),
			"second Fighter shot fires after the full-magazine shot");
		level->TickInternal(0.f);
		const shared_ptr<PrimaryWeaponProjectileActor> secondProjectile = latestProjectile();
		report.Check(projectileCount() == beforeSecondShot + 1 && secondProjectile,
			"second Fighter shot creates one projectile");
		report.CheckValue("second opening shot keeps its bonus", 1.3f,
			secondProjectile->GetShotMetadata().shotDamageMultiplier, "47 of 48 rounds before FireOnce debit");
		report.CheckValue("second opening shot deals boosted damage", fighterBaseDamage * 1.3f,
			secondProjectile->GetDamage(), "one round has already been spent");
		report.CheckValue("delayed first projectile keeps its original snapshot", fighterBaseDamage * 1.3f,
			firstProjectile->GetDamage(), "second shot was fired before first projectile impact");
		PrimaryWeaponExecutionSystem::EndFire(fighterContext, fighterRuntime);
		PrimaryWeaponExecutionSystem::BeginFire(fighterContext, fighterRuntime);
		for (int shot = 3; shot <= 4; ++shot)
		{
			report.Check(PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime), "opening-window boundary Fighter shot fires");
			level->TickInternal(0.f);
			const auto projectile = latestProjectile();
			report.Check(projectile != nullptr, "opening-window boundary spawns a real projectile");
			const float expected = shot == 3 ? 1.3f : 1.f;
			report.CheckValue("Fighter third boosted and fourth ordinary shot", expected,
				projectile->GetShotMetadata().shotDamageMultiplier, "shot " + std::to_string(shot) + "; releasing fire did not reset the window");
			report.CheckValue("Fighter boundary damage matches its shot snapshot", fighterBaseDamage * expected,
				projectile->GetDamage(), "shot " + std::to_string(shot));
		}

		bool drained = true;
		for (int shot = 0; shot < fullCapacity - 4; ++shot)
		{
			if (!PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime))
			{
				drained = false;
				break;
			}
			level->TickInternal(0.f);
		}
		report.Check(drained && fighterRuntime.magazineState.roundsRemaining == 0 &&
			fighterRuntime.magazineState.reloadRemaining > 0.0,
			"real successful FireOnce calls drain the magazine and start reload");
		report.Check(!PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime) &&
			fighterRuntime.magazineState.roundsRemaining == 0,
			"empty magazine rejects fire without spending extra ammo");
		PrimaryWeaponExecutionSystem::AdvanceMagazineReload(fighter, fighterRuntime,
			static_cast<float>(fighterRuntime.magazineState.reloadRemaining) + 0.01f);
		report.Check(fighterRuntime.magazineState.roundsRemaining == fullCapacity &&
			fighterRuntime.magazineState.reloadRemaining == 0.0 && fighterRuntime.magazineState.shotsFired == 0,
			"completed reload restores the full magazine");
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime),
			"first shot after reload fires through the same runtime");
		level->TickInternal(0.f);
		const shared_ptr<PrimaryWeaponProjectileActor> reloadedProjectile = latestProjectile();
		report.CheckValue("reload makes the next shot full-magazine eligible again", 1.3f,
			reloadedProjectile->GetShotMetadata().shotDamageMultiplier, "rounds restored to magazine capacity");
		for (int shot = 2; shot <= 4; ++shot)
		{
			report.Check(PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime), "reloaded opening-window shot fires");
			level->TickInternal(0.f);
			report.CheckValue("reload restores exactly three boosted shots", shot <= 3 ? 1.3f : 1.f,
				latestProjectile()->GetShotMetadata().shotDamageMultiplier, "reloaded shot " + std::to_string(shot));
		}

		const PrimaryWeaponDefinition* dualCatalog = content::WeaponContentCatalog::FindById("Weapon.Projectile.DualKineticBlaster.Basic");
		report.Check(dualCatalog && dualCatalog->magazine.has_value() && dualCatalog->muzzleDefinitions.size() == 2,
			"Dual Kinetic Blaster keeps its shipped two-muzzle magazine configuration");
		const PrimaryWeaponDefinition dual = *dualCatalog;
		sas::GameplayAttributeList dualAttributes = dual.attributes;
		PrimaryWeaponExecutionContext dualContext{ *source, dual, dualAttributes };
		dualContext.damageTags = dual.damageTags;
		PrimaryWeaponRuntimeState dualRuntime;
		report.Check(PrimaryWeaponExecutionSystem::InitializeRuntime(dual, dualRuntime).isValid,
			"Dual Kinetic Blaster initializes its real magazine runtime");
		PrimaryWeaponExecutionSystem::BeginFire(dualContext, dualRuntime);
		const std::size_t beforeDualShot = projectileCount();
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(dualContext, dualRuntime) &&
			dualRuntime.magazineState.roundsRemaining == dual.magazine->capacity - 2,
			"full-magazine multi-muzzle shot fires and spends one round per muzzle");
		level->TickInternal(0.f);
		const auto allProjectilesAfterDual = level->GetActorsByType<PrimaryWeaponProjectileActor>();
		report.Check(allProjectilesAfterDual.size() == beforeDualShot + 2,
			"multi-muzzle shot creates both real projectiles");
		const float dualBaseDamage = sas::FindAttributeValue(dualAttributes, CommonAttributeIds::Damage, 0.f);
		for (std::size_t index = beforeDualShot; index < allProjectilesAfterDual.size(); ++index)
		{
			const shared_ptr<PrimaryWeaponProjectileActor> projectile = allProjectilesAfterDual[index].lock();
			report.Check(projectile != nullptr, "multi-muzzle projectile remains owned by the world");
			report.CheckValue("both muzzles share the full-magazine shot snapshot", 1.3f,
				projectile->GetShotMetadata().shotDamageMultiplier, "same FireOnce metadata across muzzles");
			report.CheckValue("both muzzles apply the multiplier once", dualBaseDamage * 1.3f,
				projectile->GetDamage(), "shared per-shot metadata");
		}

		for (int shot = 2; shot <= 4; ++shot)
		{
			const std::size_t beforeShot = projectileCount();
			report.Check(PrimaryWeaponExecutionSystem::FireOnce(dualContext, dualRuntime), "Dual opening-window boundary fires");
			level->TickInternal(0.f);
			const auto projectiles = level->GetActorsByType<PrimaryWeaponProjectileActor>();
			report.Check(projectiles.size() == beforeShot + 2 && dualRuntime.magazineState.shotsFired == shot,
				"two muzzles consume one opening-window shot, not two");
			for (std::size_t index = beforeShot; index < projectiles.size(); ++index)
				report.CheckValue("Dual first three volleys boosted and fourth ordinary", shot <= 3 ? 1.3f : 1.f,
					projectiles[index].lock()->GetShotMetadata().shotDamageMultiplier, "volley " + std::to_string(shot));
		}
		const PrimaryWeaponDefinition* shotgunCatalog = content::WeaponContentCatalog::FindById("Weapon.Projectile.EnemyTwinBladeScatter.Basic");
		report.Check(shotgunCatalog && !shotgunCatalog->magazine.has_value(),
			"shipped shotgun remains a no-magazine weapon");
		PrimaryWeaponDefinition magazineShotgun = *shotgunCatalog;
		magazineShotgun.magazine = PrimaryWeaponMagazineDefinition{ 3, 1.f };
		sas::GameplayAttributeList shotgunAttributes = magazineShotgun.attributes;
		PrimaryWeaponExecutionContext shotgunContext{ *source, magazineShotgun, shotgunAttributes };
		shotgunContext.damageTags = magazineShotgun.damageTags;
		PrimaryWeaponRuntimeState shotgunRuntime;
		report.Check(PrimaryWeaponExecutionSystem::InitializeRuntime(magazineShotgun, shotgunRuntime).isValid,
			"shotgun test copy initializes with an explicit magazine");
		PrimaryWeaponExecutionSystem::BeginFire(shotgunContext, shotgunRuntime);
		const std::size_t beforeShotgun = projectileCount();
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(shotgunContext, shotgunRuntime),
			"magazine-backed shotgun test copy fires through its shipped falloff handler");
		level->TickInternal(0.f);
		const auto allProjectilesAfterShotgun = level->GetActorsByType<PrimaryWeaponProjectileActor>();
		report.Check(allProjectilesAfterShotgun.size() == beforeShotgun + 3,
			"shotgun volley creates its three real pellets");
		const float shotgunBaseDamage = sas::FindAttributeValue(shotgunAttributes, CommonAttributeIds::Damage, 0.f);
		for (std::size_t index = beforeShotgun; index < allProjectilesAfterShotgun.size(); ++index)
		{
			const shared_ptr<PrimaryWeaponProjectileActor> projectile = allProjectilesAfterShotgun[index].lock();
			report.Check(projectile != nullptr, "shotgun pellet remains owned by the world");
			report.CheckValue("shotgun pellets carry the full-magazine shot metadata", 1.3f,
				projectile->GetShotMetadata().shotDamageMultiplier, "shotgun FireOnce begins with 3 of 3 rounds");
		}
		DamageCapture shotgunDamage;
		const DelegateHandle shotgunDamageHandle = source->GetCombatRuntime().onDamageDealt.BindAction(
			&shotgunDamage, &DamageCapture::OnDamage);
		const shared_ptr<PrimaryWeaponProjectileActor> firstShotgunPellet = allProjectilesAfterShotgun[beforeShotgun].lock();
		const shared_ptr<PrimaryWeaponProjectileActor> secondShotgunPellet = allProjectilesAfterShotgun[beforeShotgun + 1].lock();
		report.Check(firstShotgunPellet && secondShotgunPellet, "two shotgun pellets remain alive before delayed impacts");
		firstShotgunPellet->OnActorBeginOverlap(target.get());
		report.Check(shotgunDamage.records.size() == 1, "first shared-group pellet applies one damage event",
			Json{ { "damageEvents", shotgunDamage.records.size() } });
		secondShotgunPellet->OnActorBeginOverlap(target.get());
		source->GetCombatRuntime().onDamageDealt.UnbindAction(shotgunDamageHandle);
		report.Check(shotgunDamage.records.size() == 2,
			"shared shotgun impact group applies two pellet impacts to one target",
			Json{ { "damageEvents", shotgunDamage.records.size() } });
		report.CheckValue("shotgun falloff begins from the 1.3x full-magazine damage once", shotgunBaseDamage * 1.3f,
			shotgunDamage.records[0].originalDamage, "first target pellet, no falloff");
		report.CheckValue("shotgun falloff applies after the captured multiplier", shotgunBaseDamage * 1.3f * 0.9f,
			shotgunDamage.records[1].originalDamage, "second target pellet, 10 percent falloff");
		level->TickInternal(0.f);

		const PrimaryWeaponDefinition noMagazineShotgun = *shotgunCatalog;
		sas::GameplayAttributeList noMagazineAttributes = noMagazineShotgun.attributes;
		PrimaryWeaponExecutionContext noMagazineContext{ *source, noMagazineShotgun, noMagazineAttributes };
		noMagazineContext.damageTags = noMagazineShotgun.damageTags;
		PrimaryWeaponRuntimeState noMagazineRuntime;
		report.Check(PrimaryWeaponExecutionSystem::InitializeRuntime(noMagazineShotgun, noMagazineRuntime).isValid,
			"shipped no-magazine shotgun initializes unchanged");
		PrimaryWeaponExecutionSystem::BeginFire(noMagazineContext, noMagazineRuntime);
		const std::size_t beforeNoMagazineShot = projectileCount();
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(noMagazineContext, noMagazineRuntime),
			"shipped no-magazine shotgun fires while Full Magazine is installed");
		level->TickInternal(0.f);
		const auto noMagazineProjectiles = level->GetActorsByType<PrimaryWeaponProjectileActor>();
		report.Check(noMagazineProjectiles.size() == beforeNoMagazineShot + 3,
			"no-magazine shotgun creates its usual pellet count");
		for (std::size_t index = beforeNoMagazineShot; index < noMagazineProjectiles.size(); ++index)
		{
			const shared_ptr<PrimaryWeaponProjectileActor> projectile = noMagazineProjectiles[index].lock();
			report.Check(projectile != nullptr, "no-magazine shotgun pellet remains owned by the world");
			report.CheckValue("no-magazine weapon receives no Full Magazine multiplier", 1.f,
				projectile->GetShotMetadata().shotDamageMultiplier, "unmodified shipped weapon has no magazine");
			report.CheckValue("no-magazine shotgun keeps its base damage", shotgunBaseDamage,
				projectile->GetDamage(), "Full Magazine eligibility requires a magazine definition");
		}

		const PrimaryWeaponDefinition* coneCatalog = content::WeaponContentCatalog::FindById("Weapon.Cone.RapidShotgun.Basic");
		report.Check(coneCatalog && coneCatalog->magazine.has_value(),
			"the shipped Cone Rapid Shotgun is the third magazine-backed player weapon");
		const PrimaryWeaponDefinition cone = *coneCatalog;
		sas::GameplayAttributeList coneAttributes = cone.attributes;
		PrimaryWeaponExecutionContext coneContext{ *source, cone, coneAttributes };
		coneContext.damageTags = cone.damageTags;
		PrimaryWeaponRuntimeState coneRuntime;
		report.Check(PrimaryWeaponExecutionSystem::InitializeRuntime(cone, coneRuntime).isValid,
			"Cone Rapid Shotgun initializes its shipped magazine runtime");
		PrimaryWeaponExecutionSystem::BeginFire(coneContext, coneRuntime);
		source->SetActorLocation({ 0.f, 0.f });
		target->SetActorLocation({ 0.f, -250.f });
		DamageCapture coneDamage;
		const DelegateHandle coneDamageHandle = source->GetCombatRuntime().onDamageDealt.BindAction(
			&coneDamage, &DamageCapture::OnDamage);
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(coneContext, coneRuntime) &&
			coneRuntime.magazineState.roundsRemaining == cone.magazine->capacity - 1,
			"first Cone Rapid Shotgun shot fires and spends one round from full");
		report.Check(coneDamage.records.size() == 1,
			"Cone Rapid Shotgun reaches a real enemy-layer target");
		const float coneBaseDamage = sas::FindAttributeValue(coneAttributes, CommonAttributeIds::Damage, 0.f);
		report.CheckValue("Cone Rapid Shotgun full-magazine damage is multiplied once", coneBaseDamage * 1.3f,
			coneDamage.records.back().originalDamage, "first instant cone blast from a full magazine");
		coneDamage.records.clear();
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(coneContext, coneRuntime) &&
			coneRuntime.magazineState.roundsRemaining == cone.magazine->capacity - 2,
			"next Cone Rapid Shotgun shot spends its next round");
		report.Check(coneDamage.records.size() == 1,
			"second Cone Rapid Shotgun shot reaches the same real target");
		report.CheckValue("Cone Rapid Shotgun second opening shot remains boosted", coneBaseDamage * 1.3f,
			coneDamage.records.back().originalDamage, "second shot begins below magazine capacity");
		for (int shot = 3; shot <= 4; ++shot)
		{
			coneDamage.records.clear();
			report.Check(PrimaryWeaponExecutionSystem::FireOnce(coneContext, coneRuntime) && coneDamage.records.size() == 1,
				"Cone opening-window boundary reaches the real target");
			report.CheckValue("Cone third boosted and fourth ordinary shot", coneBaseDamage * (shot == 3 ? 1.3f : 1.f),
				coneDamage.records.back().originalDamage, "cone shot " + std::to_string(shot));
		}
		source->GetCombatRuntime().onDamageDealt.UnbindAction(coneDamageHandle);

		report.Check(loadout.Uninstall(fullMagazineId) && source->GetModuleApplier().Apply(loadout, rules),
			"uninstalling Full Magazine clears its runtime multiplier");
		report.CheckValue("removed Full Magazine no longer affects future shots", 1.f,
			source->GetCombatRuntime().ResolveFullMagazineShotMultiplier(0), "module instance returned to inventory");
		report.Check(firstProjectile->GetShotMetadata().shotDamageMultiplier == 1.3f,
			"uninstall preserves an already-created shot's captured metadata");
		PrimaryWeaponRuntimeState removalRuntime;
		report.Check(PrimaryWeaponExecutionSystem::InitializeRuntime(fighter, removalRuntime).isValid,
			"post-removal fixture starts inside a fresh opening-shot window");
		PrimaryWeaponExecutionSystem::BeginFire(fighterContext, removalRuntime);
		const std::size_t beforeAfterRemoval = projectileCount();
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(fighterContext, removalRuntime),
			"Fighter fires again after Full Magazine is removed");
		level->TickInternal(0.f);
		const shared_ptr<PrimaryWeaponProjectileActor> afterRemovalProjectile = latestProjectile();
		report.Check(projectileCount() == beforeAfterRemoval + 1 && afterRemovalProjectile,
			"post-removal Fighter shot creates one projectile");
		report.CheckValue("removed module does not affect new-shot metadata", 1.f,
			afterRemovalProjectile->GetShotMetadata().shotDamageMultiplier, "Full Magazine registration cleared");
		report.CheckValue("removed module leaves new-shot damage at base", fighterBaseDamage,
			afterRemovalProjectile->GetDamage(), "Full Magazine registration cleared");
		DamageCapture delayedImpact;
		const DelegateHandle delayedImpactHandle = source->GetCombatRuntime().onDamageDealt.BindAction(
			&delayedImpact, &DamageCapture::OnDamage);
		firstProjectile->OnActorBeginOverlap(target.get());
		source->GetCombatRuntime().onDamageDealt.UnbindAction(delayedImpactHandle);
		report.Check(delayedImpact.records.size() == 1,
			"captured projectile applies one real impact after later shots, reload, and module removal");
		report.CheckValue("delayed impact keeps the captured full-magazine damage once", std::ceil(fighterBaseDamage * 1.3f),
			delayedImpact.records.front().originalDamage, "first projectile impacts after module removal");
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
		RunCooldownModuleChecks(report, application);
		RunFullMagazineChecks(report, application);
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
