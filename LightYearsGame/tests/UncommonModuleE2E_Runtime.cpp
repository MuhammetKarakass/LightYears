#include "UncommonModuleE2E.h"

#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/module/ModuleRoller.h"
#include "player/Player.h"

namespace uncommon_module_e2e
{
	namespace
	{
		std::optional<vega::ModulePlacement> FindCandidate(const vega::ModuleRulesContext& rules, const vega::ModuleLoadout& loadout, const vega::ModuleDefinition& module, std::uint32_t instanceId, int variantIndex, const sas::ContentId& sideFeatureId = {})
		{
			vega::ModulePlacement candidate;
			candidate.instanceId = instanceId;
			candidate.moduleId = module.moduleId;
			candidate.variantIndex = variantIndex;
			candidate.sideFeatureId = sideFeatureId;
			for (int rotation = 0; rotation < 4; ++rotation)
				for (int y = 0; y < rules.schematic.height; ++y)
					for (int x = 0; x < rules.schematic.width; ++x)
					{
						candidate.origin = { x, y };
						candidate.rotationQuarterTurns = rotation;
						if (vega::ModulePlacementRules::CanPlace(rules, loadout, candidate) == vega::ModulePlacementFailure::None)
							return candidate;
					}
			return std::nullopt;
		}

		std::uint32_t AddInstall(Report& report, vega::Player& player, const vega::ModuleRulesContext& rules, const vega::ModuleDefinition& module, int variantIndex, int level, const sas::ContentId& sideFeatureId = {})
		{
			const std::uint32_t id = player.AddModuleToInventory(module.moduleId, variantIndex, module.rarity, level, sideFeatureId);
			report.Check(id != 0, "Player inventory accepts a validated Uncommon placement", Json{
				{ "moduleId", module.moduleId.ToString() }, { "variantIndex", variantIndex }, { "level", level },
				{ "sideFeatureId", sideFeatureId.ToString() } });
			const auto candidate = FindCandidate(rules, player.GetModuleLoadout(), module, id, variantIndex, sideFeatureId);
			report.Check(candidate.has_value(), "real ship schematic has a legal cell for the Uncommon module");
			report.Check(player.TryInstallModule(id, candidate->origin, candidate->rotationQuarterTurns) ==
				vega::ModulePlacementFailure::None, "Player installs Uncommon module through canonical placement rules");
			return id;
		}

		void RemoveInstalled(Report& report, vega::Player& player, std::uint32_t instanceId)
		{
			report.Check(player.TryUninstallModule(instanceId), "Player uninstalls the tested module");
		}

		float CurrentOwner(const vega::PlayerSpaceShip& ship, const sas::AttributeId& id)
		{
			return ship.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(id);
		}

		float CurrentShip(const vega::PlayerSpaceShip& ship, const sas::AttributeId& id)
		{
			return ship.GetShipRuntime().GetAttributes().GetCurrentValue(id);
		}

		void CheckAttribute(Report& report, const std::string& name, float expected, float actual)
		{
			report.CheckValue(name, expected, actual, "live PlayerSpaceShip runtime attribute");
		}
	}

	void RunUncommonRuntimeChecks(Report& report, vega::Application& application, const std::filesystem::path& fixtureRoot)
	{
		using namespace vega;
		auto level = std::make_shared<common_module_e2e::HeadlessModuleLevel>(&application);
		level->BeginPlayInternal();
		Player player;
		const shared_ptr<PlayerSpaceShip> ship = player.SpawnSpaceShip(level.get()).lock();
		report.Check(ship != nullptr, "Player spawns a real ship for Uncommon inventory and live mutation");
		level->TickInternal(0.f);
		const auto rulesValue = player.BuildModuleRules();
		report.Check(rulesValue.has_value(), "Player resolves its canonical module rules");
		if (!ship || !rulesValue) return;
		const ModuleRulesContext& rules = *rulesValue;
		const ModuleDefinition* glass = rules.lookup("Module.Offense.GlassTopCore.Uncommon");
		const ModuleDefinition* doubleFeed = rules.lookup("Module.Utility.DoubleFeedCore.Uncommon");
		const ModuleDefinition* sharpScale = rules.lookup("Module.Offense.SharpScale.Uncommon");
		report.Check(glass && doubleFeed && sharpScale, "live catalog resolves all three P2 module definitions");
		if (!glass || !doubleFeed || !sharpScale) return;

		const std::size_t grantedUncommonCount = static_cast<std::size_t>(std::count_if(
			player.GetModuleLoadout().GetInventory().begin(), player.GetModuleLoadout().GetInventory().end(),
			[](const ModulePlacement& item) { return item.rarity == ModuleRarity::Uncommon; }));
		std::size_t expectedGrantCount = 0;
		for (const auto& definition : content::ModuleContentCatalog::GetDefinitions())
		{
			if (definition.rarity != ModuleRarity::Uncommon) continue;
			expectedGrantCount += definition.sizeVariants.size();
			for (const auto& feature : content::ModuleSideFeatureCatalog::GetDefinitions())
				if (IsSideFeatureUsableAtLevel(definition, &feature, 1)) expectedGrantCount += definition.sizeVariants.size();
		}
		report.Check(grantedUncommonCount == expectedGrantCount, "test grant covers the legal level-one ladder and side-feature inventory combinations",
			Json{ { "uncommonInventoryItems", grantedUncommonCount }, { "expected", expectedGrantCount } });
		report.Check(player.AddModuleToInventory(glass->moduleId, 99, glass->rarity, 1) == 0,
			"Player rejects an invalid variant without adding an inventory item");
		report.Check(player.AddModuleToInventory(doubleFeed->moduleId, 0, doubleFeed->rarity, 1,
			sas::ContentId{ "Module.SideFeature.Shared.O1" }) == 0,
			"Player rejects side features on the one-size Double Feed module");

		const sas::ContentId o1{ "Module.SideFeature.Shared.O1" };
		const sas::ContentId o2{ "Module.SideFeature.Shared.O2" };
		const sas::ContentId shieldReturn{ "Module.SideFeature.GlassTop.ShieldReturn" };
		const float attackBeforeGlass = CurrentOwner(*ship, OwnerAttributeIds::AttackPower);
		const float healthBeforeGlass = CurrentOwner(*ship, OwnerAttributeIds::MaxHealth);
		const float critBeforeGlass = CurrentOwner(*ship, OwnerAttributeIds::CriticalChance);
		const float shieldBeforeGlass = CurrentShip(*ship, ShipAttributeIds::MaxShield);
		const std::uint32_t glassId = AddInstall(report, player, rules, *glass, 1, 1, o2);
		const ModulePlacement glassPlacement = common_module_e2e::FindInstalled(player.GetModuleLoadout(), glassId);
		const float glassBonus = ModuleResolver::GetZoneBonus(rules, *glass, glassPlacement);
		report.Check(player.GetModuleLoadout().FindPlacement(glassId) != nullptr, "Glass Top is installed on the real player ship");
		const float actualGlassAP = CurrentOwner(*ship, OwnerAttributeIds::AttackPower);
		const float actualGlassCrit = CurrentOwner(*ship, OwnerAttributeIds::CriticalChance);
		CheckAttribute(report, "large Glass Top applies level-one AP from the live baseline", attackBeforeGlass + 34.f * (1.f + glassBonus), actualGlassAP);
		CheckAttribute(report, "large Glass Top O2 is fixed +8 even inside a zone bonus", critBeforeGlass + 8.f, actualGlassCrit);
		CheckAttribute(report, "large Glass Top applies a static 16-percent health penalty", healthBeforeGlass * 0.84f,
			CurrentOwner(*ship, OwnerAttributeIds::MaxHealth));
		const ModulePlacement* installedGlass = player.GetModuleLoadout().FindPlacement(glassId);
		report.Check(installedGlass && installedGlass->rarity == ModuleRarity::Uncommon && installedGlass->level == 1 &&
			installedGlass->sideFeatureId == o2, "placement carries rarity, level, and side-feature metadata");

		const std::uint64_t beforeLevelTwo = player.GetModuleLoadout().GetRevision();
		report.Check(player.SetModuleLevel(glassId, 2), "Player level API mutates an installed module");
		report.Check(player.GetModuleLoadout().GetRevision() == beforeLevelTwo + 1 &&
			ship->GetModuleApplier().GetAppliedRevision() == player.GetModuleLoadout().GetRevision(),
			"level mutation advances revision and reapplies the live ship");
		CheckAttribute(report, "Glass Top level two reads explicit compound AP table", attackBeforeGlass + 39.78f * (1.f + glassBonus),
			CurrentOwner(*ship, OwnerAttributeIds::AttackPower));
		CheckAttribute(report, "Glass Top level two keeps its static health penalty", healthBeforeGlass * 0.84f,
			CurrentOwner(*ship, OwnerAttributeIds::MaxHealth));
		const std::uint64_t beforeLevelThree = player.GetModuleLoadout().GetRevision();
		report.Check(player.SetModuleLevel(glassId, 3), "Player accepts Glass Top level three");
		report.Check(player.GetModuleLoadout().GetRevision() == beforeLevelThree + 1 &&
			ship->GetModuleApplier().GetAppliedRevision() == player.GetModuleLoadout().GetRevision(),
			"level three mutation is applied to the active ship");
		CheckAttribute(report, "Glass Top level three reads its explicit compound AP value", attackBeforeGlass + 46.5426f * (1.f + glassBonus),
			CurrentOwner(*ship, OwnerAttributeIds::AttackPower));
		CheckAttribute(report, "Glass Top level three keeps its static health penalty", healthBeforeGlass * 0.84f,
			CurrentOwner(*ship, OwnerAttributeIds::MaxHealth));
		const std::uint64_t beforeInvalid = player.GetModuleLoadout().GetRevision();
		report.Check(!player.SetModuleLevel(glassId, 0) && !player.SetModuleSideFeature(glassId, sas::ContentId{ "Module.SideFeature.Unknown" }) &&
			player.GetModuleLoadout().GetRevision() == beforeInvalid, "invalid level and side-feature mutations are no-ops");
		const std::uint64_t beforeNoOp = player.GetModuleLoadout().GetRevision();
		report.Check(player.SetModuleLevel(glassId, 3) && player.GetModuleLoadout().GetRevision() == beforeNoOp,
			"setting the current level does not create a false revision");

		const std::uint64_t beforeSideSwitch = player.GetModuleLoadout().GetRevision();
		report.Check(player.SetModuleSideFeature(glassId, shieldReturn), "Player changes Glass Top to its private shield feature");
		report.Check(player.GetModuleLoadout().GetRevision() == beforeSideSwitch + 1 &&
			ship->GetModuleApplier().GetAppliedRevision() == player.GetModuleLoadout().GetRevision(),
			"side-feature mutation advances revision and reapplies the live ship");
		CheckAttribute(report, "private Glass Top side feature returns six percent MaxShield", shieldBeforeGlass * 1.06f,
			CurrentShip(*ship, ShipAttributeIds::MaxShield));
		CheckAttribute(report, "changing away from O2 removes its critical-chance modifier", critBeforeGlass,
			CurrentOwner(*ship, OwnerAttributeIds::CriticalChance));
		report.Check(player.TryMoveModule(glassId, glassPlacement.origin, glassPlacement.rotationQuarterTurns) == ModulePlacementFailure::None,
			"Glass Top move preserves its valid placement metadata");
		RemoveInstalled(report, player, glassId);
		const ModulePlacement* returnedGlass = player.GetModuleLoadout().FindInventoryItem(glassId);
		report.Check(returnedGlass && returnedGlass->level == 3 && returnedGlass->sideFeatureId == shieldReturn,
			"uninstall preserves the module level and side feature");

		const float smallAPBefore = CurrentOwner(*ship, OwnerAttributeIds::AttackPower);
		const float smallHealthBefore = CurrentOwner(*ship, OwnerAttributeIds::MaxHealth);
		const float smallCritBefore = CurrentOwner(*ship, OwnerAttributeIds::CriticalChance);
		const std::uint32_t smallGlassId = AddInstall(report, player, rules, *glass, 0, 1, o2);
		const ModulePlacement smallPlacement = common_module_e2e::FindInstalled(player.GetModuleLoadout(), smallGlassId);
		const float smallBonus = ModuleResolver::GetZoneBonus(rules, *glass, smallPlacement);
		CheckAttribute(report, "small Glass Top carries O2 but keeps it locked", smallCritBefore,
			CurrentOwner(*ship, OwnerAttributeIds::CriticalChance));
		report.Check(player.GetModuleLoadout().FindPlacement(smallGlassId)->sideFeatureId == o2,
			"locked side feature remains selected on the small variant");
		CheckAttribute(report, "small Glass Top real AP is +12 with its resolved zone", smallAPBefore + 12.f * (1.f + smallBonus),
			CurrentOwner(*ship, OwnerAttributeIds::AttackPower));
		CheckAttribute(report, "small Glass Top applies its static six-percent health penalty", smallHealthBefore * 0.94f,
			CurrentOwner(*ship, OwnerAttributeIds::MaxHealth));
		RemoveInstalled(report, player, smallGlassId);

		const float doubleFeedAPBase = CurrentOwner(*ship, OwnerAttributeIds::AttackPower);
		const float doubleFeedEPBase = CurrentOwner(*ship, OwnerAttributeIds::EnergyPower);
		const std::uint32_t doubleFeedId = AddInstall(report, player, rules, *doubleFeed, 0, 1);
		const ModulePlacement doubleFeedPlacement = common_module_e2e::FindInstalled(player.GetModuleLoadout(), doubleFeedId);
		const float doubleFeedBonus = ModuleResolver::GetZoneBonus(rules, *doubleFeed, doubleFeedPlacement);
		for (int moduleLevel = 1; moduleLevel <= 3; ++moduleLevel)
		{
			const std::uint64_t priorRevision = player.GetModuleLoadout().GetRevision();
			if (moduleLevel > 1) report.Check(player.SetModuleLevel(doubleFeedId, moduleLevel), "Double Feed level mutation succeeds");
			const float apValues[] = { 15.f, 18.f, 21.f };
			const float epValues[] = { 23.f, 28.f, 33.f };
			CheckAttribute(report, "Double Feed level AP matches the real attribute", doubleFeedAPBase + apValues[moduleLevel - 1] * (1.f + doubleFeedBonus),
				CurrentOwner(*ship, OwnerAttributeIds::AttackPower));
			CheckAttribute(report, "Double Feed level EP matches the real attribute", doubleFeedEPBase + epValues[moduleLevel - 1] * (1.f + doubleFeedBonus),
				CurrentOwner(*ship, OwnerAttributeIds::EnergyPower));
			if (moduleLevel > 1) report.Check(player.GetModuleLoadout().GetRevision() == priorRevision + 1 &&
				ship->GetModuleApplier().GetAppliedRevision() == player.GetModuleLoadout().GetRevision(), "Double Feed level reapply tracks its revision");
		}
		RemoveInstalled(report, player, doubleFeedId);

		const float sharpAttackBase = CurrentOwner(*ship, OwnerAttributeIds::AttackPower);
		const float sharpCritDamageBase = CurrentOwner(*ship, OwnerAttributeIds::CriticalDamage);
		const float sharpCritChanceBase = CurrentOwner(*ship, OwnerAttributeIds::CriticalChance);
		const std::uint32_t sharpId = AddInstall(report, player, rules, *sharpScale, 1, 1, o1);
		const ModulePlacement sharpPlacement = common_module_e2e::FindInstalled(player.GetModuleLoadout(), sharpId);
		const float sharpBonus = ModuleResolver::GetZoneBonus(rules, *sharpScale, sharpPlacement);
		const float sharpDamageValues[] = { 0.20f, 0.25f, 0.30f };
		for (int moduleLevel = 1; moduleLevel <= 3; ++moduleLevel)
		{
			if (moduleLevel > 1) report.Check(player.SetModuleLevel(sharpId, moduleLevel), "Sharp Scale level mutation succeeds");
			CheckAttribute(report, "Sharp Scale real critical damage follows level table", sharpCritDamageBase + sharpDamageValues[moduleLevel - 1] * (1.f + sharpBonus),
				CurrentOwner(*ship, OwnerAttributeIds::CriticalDamage));
			CheckAttribute(report, "Sharp Scale critical chance remains fixed without O1 contamination", sharpCritChanceBase + 15.f * (1.f + sharpBonus),
				CurrentOwner(*ship, OwnerAttributeIds::CriticalChance));
			CheckAttribute(report, "Sharp Scale O1 side feature adds eight attack power", sharpAttackBase + 8.f,
				CurrentOwner(*ship, OwnerAttributeIds::AttackPower));
		}
		RemoveInstalled(report, player, sharpId);

		const std::uint32_t cycleId = player.AddModuleToInventory(glass->moduleId, 0, glass->rarity, 1);
		const std::size_t baselineHandles = ship->GetModuleApplier().GetOwnedHandleCount();
		std::size_t maximumHandles = baselineHandles;
		for (int cycle = 0; cycle < 10; ++cycle)
		{
			const auto candidate = FindCandidate(rules, player.GetModuleLoadout(), *glass, cycleId, 0);
			report.Check(candidate.has_value() && player.TryInstallModule(cycleId, candidate->origin, candidate->rotationQuarterTurns) == ModulePlacementFailure::None,
				"module handle cycle installs through live Player placement");
			maximumHandles = std::max(maximumHandles, ship->GetModuleApplier().GetOwnedHandleCount());
			report.Check(player.TryUninstallModule(cycleId), "module handle cycle uninstalls through live Player placement");
			report.Check(ship->GetModuleApplier().GetOwnedHandleCount() == baselineHandles, "module handle cycle returns to its baseline count");
		}
		report.Check(maximumHandles <= baselineHandles + 2, "ten module install/uninstall cycles keep actual owned handles bounded",
			Json{ { "baseline", baselineHandles }, { "maximum", maximumHandles }, { "cycles", 10 } });

		RunRuntimeRuleFixtureChecks(report, player, *ship, *level, fixtureRoot);
	}
}
