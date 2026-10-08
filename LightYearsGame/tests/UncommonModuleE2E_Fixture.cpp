#include "UncommonModuleE2E.h"

#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"
#include "gameplay/module/ShipModuleApplier.h"
#include "player/Player.h"

#include <fstream>
#include <limits>

namespace uncommon_module_e2e
{
	namespace
	{
		std::filesystem::path DataPath(const char* name)
		{
			return std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data" / name;
		}

		Json ReadJson(const std::filesystem::path& path)
		{
			std::ifstream input{ path };
			if (!input) throw std::runtime_error{ "Could not read fixture source: " + path.string() };
			Json value;
			input >> value;
			return value;
		}

		std::filesystem::path WriteJson(const std::filesystem::path& root, const std::string& name, const Json& value)
		{
			std::filesystem::create_directories(root);
			const std::filesystem::path path = root / name;
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			if (!output) throw std::runtime_error{ "Could not write fixture: " + path.string() };
			output << value.dump(2) << '\n';
			return path;
		}

		Json& FindModule(Json& modules, const std::string& id)
		{
			for (Json& module : modules.at("modules")) if (module.at("id") == id) return module;
			throw std::runtime_error{ "Fixture module is missing: " + id };
		}

		Json Attribute(const char* id, Json magnitude, bool affectedByZoneBonus = true)
		{
			return Json{ { "target", "Owner" }, { "attributeId", id }, { "operation", "Add" },
				{ "magnitude", std::move(magnitude) }, { "affectedByZoneBonus", affectedByZoneBonus } };
		}

		Json Trigger(Json movement, Json magnitude)
		{
			return Json{ { "trigger", "AbilityActivated" }, { "durationSeconds", Json::array({ 5.0, 5.0, 5.0 }) },
				{ "movementSpeedMultiplier", std::move(movement) },
				{ "modifiers", Json::array({ Attribute("Owner.AttackPower", std::move(magnitude), false) }) } };
		}

		Json DamageRule(Json multiplier)
		{
			return Json{ { "condition", "PrimaryShotOnly" }, { "damageMultiplier", std::move(multiplier) },
				{ "activeWhileTrigger", "AbilityActivated" }, { "activeDurationSeconds", Json::array({ 5.0, 5.0, 5.0 }) } };
		}

		Json MakeRuntimeModule()
		{
			Json module{
				{ "id", "Module.Offense.P2RuntimeFixture.Uncommon" }, { "displayName", "P2 runtime fixture" },
				{ "rarity", "Uncommon" }, { "levelCount", 3 }, { "family", "Ability.Offense" },
				{ "sideFeatures", Json::array({ "Module.SideFeature.P2RuntimeFixture.Side" }) }, { "sizeLadder", Json::array({ 1, 2 }) },
				{ "sizeVariants", Json::array({
					Json{ { "shape", Json::array({ Json::array({ 0, 0 }) }) } },
					Json{
						{ "shape", Json::array({ Json::array({ 0, 0 }), Json::array({ 1, 0 }) }) },
						{ "attributeModifiers", Json::array({ Attribute("Owner.AttackPower", Json::array({ 3.0, 4.0, 5.0 })) }) },
						{ "triggerBuffs", Json::array({ Trigger(1.1, Json::array({ 2.0, 4.0, 6.0 })) }) },
						{ "damageRules", Json::array({ DamageRule(1.1) }) }
					}
				}) }
			};
			return module;
		}

		Json MakeRuntimeValueTwin()
		{
			Json module = MakeRuntimeModule();
			module["id"] = "Module.Offense.P2RuntimeValueTwin.Uncommon";
			module["displayName"] = "P2 runtime value twin";
			module["sizeVariants"][1]["attributeModifiers"][0]["magnitude"] = Json::array({ 6.0, 8.0, 10.0 });
			module["sizeVariants"][1]["triggerBuffs"][0]["modifiers"][0]["magnitude"] = Json::array({ 3.0, 6.0, 9.0 });
			module["sizeVariants"][1]["damageRules"][0]["damageMultiplier"] = Json::array({ 1.2, 1.2, 1.2 });
			return module;
		}

		Json MakeLevelFourModule()
		{
			return Json{
				{ "id", "Module.Utility.P2LevelFourFixture.Uncommon" }, { "displayName", "P2 level four fixture" },
				{ "rarity", "Uncommon" }, { "levelCount", 4 }, { "family", "Ability.Utility" }, { "sizeLadder", Json::array({ 1 }) },
				{ "sizeVariants", Json::array({ Json{
					{ "shape", Json::array({ Json::array({ 0, 0 }) }) },
					{ "attributeModifiers", Json::array({ Attribute("Owner.AttackPower", Json::array({ 15.0, 18.0, 21.0, 24.0 })),
						Attribute("Owner.Energy.Power", Json::array({ 23.0, 28.0, 33.0, 38.0 })) }) }
				} }) }
			};
		}

		Json MakeMovementTableModule()
		{
			return Json{
				{ "id", "Module.Movement.P2NeutralFirstFixture.Uncommon" }, { "displayName", "P2 neutral-first fixture" },
				{ "rarity", "Uncommon" }, { "family", "Ability.Movement" }, { "sizeLadder", Json::array({ 1 }) },
				{ "sizeVariants", Json::array({ Json{
					{ "shape", Json::array({ Json::array({ 0, 0 }) }) },
					{ "triggerBuffs", Json::array({ Json{
						{ "trigger", "AbilityActivated" }, { "durationSeconds", Json::array({ 5.0, 5.0, 5.0 }) },
						{ "movementSpeedMultiplier", Json::array({ 1.0, 1.1, 1.2 }) }, { "modifiers", Json::array() }
					} }) }
				} }) }
			};
		}

		const ly::ModuleDefinition* FindDefinition(const ly::List<ly::ModuleDefinition>& definitions, const sas::ContentId& id)
		{
			for (const ly::ModuleDefinition& definition : definitions) if (definition.moduleId == id) return &definition;
			return nullptr;
		}

		const ly::ModuleSideFeatureDefinition* FindFeature(const ly::List<ly::ModuleSideFeatureDefinition>& definitions, const sas::ContentId& id)
		{
			for (const ly::ModuleSideFeatureDefinition& definition : definitions) if (definition.sideFeatureId == id) return &definition;
			return nullptr;
		}

		std::optional<ly::ModulePlacement> FindCandidate(const ly::ModuleRulesContext& rules, const ly::ModuleLoadout& loadout, const ly::ModuleDefinition& module, std::uint32_t instanceId, int variantIndex, const sas::ContentId& sideFeatureId = {})
		{
			ly::ModulePlacement candidate;
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
						if (ly::ModulePlacementRules::CanPlace(rules, loadout, candidate) == ly::ModulePlacementFailure::None)
							return candidate;
					}
			return std::nullopt;
		}

		float CurrentOwner(const ly::PlayerSpaceShip& ship, const sas::AttributeId& id)
		{
			return ship.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(id);
		}

		void CheckAttribute(Report& report, const std::string& name, float expected, float actual)
		{
			report.CheckValue(name, expected, actual, "live PlayerSpaceShip runtime attribute");
		}

		void BroadcastAbilityActivated(ly::PlayerSpaceShip& ship)
		{
			sas::AbilityEvent event;
			event.eventTag = ly::GameplayTags::Event::Ability::Activated;
			event.magnitude = 0.f;
			ship.GetAbilitySystemComponent().onGameplayEvent.Broadcast(event);
		}

		bool InstallFixture(Report& report, ly::ModuleLoadout& loadout, const ly::ModuleRulesContext& rules, const ly::ModuleDefinition& module, int variantIndex, int moduleLevel, const sas::ContentId& sideFeatureId, ly::PlayerSpaceShip& ship)
		{
			const std::uint32_t instanceId = loadout.AddToInventory(rules, module, variantIndex, module.rarity, moduleLevel, sideFeatureId);
			report.Check(instanceId != 0, "fixture module is accepted by canonical loadout validation");
			const auto candidate = FindCandidate(rules, loadout, module, instanceId, variantIndex, sideFeatureId);
			report.Check(candidate.has_value(), "fixture module finds a legal real schematic placement");
			if (!candidate) return false;
			report.Check(loadout.Install(rules, instanceId, candidate->origin, candidate->rotationQuarterTurns) ==
				ly::ModulePlacementFailure::None, "fixture module installs through the canonical loadout");
			report.Check(ship.GetModuleApplier().Apply(loadout, rules), "real ship applier installs temporary JSON fixture rules");
			return true;
		}

		void UninstallFixture(Report& report, ly::ModuleLoadout& loadout, const ly::ModuleRulesContext& rules,
			std::uint32_t instanceId, ly::PlayerSpaceShip& ship)
		{
			report.Check(loadout.Uninstall(instanceId) && ship.GetModuleApplier().Apply(loadout, rules),
				"temporary fixture uninstall clears actual ship effects");
		}
	}

	void RunRuntimeRuleFixtureChecks(Report& report, ly::Player& player, ly::PlayerSpaceShip& ship, common_module_e2e::HeadlessModuleLevel&, const std::filesystem::path& fixtureRoot)
	{
		using namespace ly;
		using namespace ly::content;
		const Json shippedModules = ReadJson(DataPath("modules.json"));
		Json fixtureModules = shippedModules;
		fixtureModules.at("modules").push_back(MakeRuntimeModule());
		fixtureModules.at("modules").push_back(MakeRuntimeValueTwin());
		fixtureModules.at("modules").push_back(MakeLevelFourModule());
		fixtureModules.at("modules").push_back(MakeMovementTableModule());

		Json sideFeatures = ReadJson(DataPath("module_side_features.json"));
		sideFeatures.at("sideFeatures").push_back(Json{
			{ "id", "Module.SideFeature.P2RuntimeFixture.Side" }, { "displayName", "P2 runtime fixture side" },
			{ "levelCount", 3 }, { "rollWeight", 1.0 },
			{ "rules", Json{
				{ "triggerBuffs", Json::array({ Trigger(1.2, Json::array({ 10.0, 20.0, 30.0 })) }) },
				{ "damageRules", Json::array({ DamageRule(1.3) }) }
			} }
		});
		const std::filesystem::path modulesPath = WriteJson(fixtureRoot, "runtime-fixture-modules.json", fixtureModules);
		const std::filesystem::path featuresPath = WriteJson(fixtureRoot, "runtime-fixture-side-features.json", sideFeatures);
		const ModuleLoader::Result parsedModules = ModuleLoader::LoadFromFile(modulesPath);
		const ModuleSideFeatureLoader::Result parsedFeatures = ModuleSideFeatureLoader::LoadFromFile(featuresPath);
		report.Check(parsedModules.Succeeded() && parsedFeatures.Succeeded(), "typed runtime fixtures load from JSON rule packages",
			Json{ { "modules", parsedModules.error }, { "sideFeatures", parsedFeatures.error } });
		if (!parsedModules.Succeeded() || !parsedFeatures.Succeeded()) return;
		const ModuleDefinition* runtimeModule = FindDefinition(parsedModules.definitions, "Module.Offense.P2RuntimeFixture.Uncommon");
		const ModuleDefinition* runtimeValueTwin = FindDefinition(parsedModules.definitions, "Module.Offense.P2RuntimeValueTwin.Uncommon");
		const ModuleDefinition* levelFourModule = FindDefinition(parsedModules.definitions, "Module.Utility.P2LevelFourFixture.Uncommon");
		const ModuleDefinition* movementModule = FindDefinition(parsedModules.definitions, "Module.Movement.P2NeutralFirstFixture.Uncommon");
		const ModuleSideFeatureDefinition* runtimeSide = FindFeature(parsedFeatures.definitions, "Module.SideFeature.P2RuntimeFixture.Side");
		report.Check(runtimeModule && runtimeValueTwin && levelFourModule && movementModule && runtimeSide,
			"JSON runtime fixture contains the typed value twin, namespaced rule, N=4, movement, and side packages");
		if (!runtimeModule || !runtimeValueTwin || !levelFourModule || !movementModule || !runtimeSide) return;
		report.Check(runtimeModule->sizeVariants[1].attributeModifiers.size() == runtimeValueTwin->sizeVariants[1].attributeModifiers.size() &&
			runtimeModule->sizeVariants[1].triggerBuffs.size() == runtimeValueTwin->sizeVariants[1].triggerBuffs.size() &&
			runtimeModule->sizeVariants[1].damageRules.size() == runtimeValueTwin->sizeVariants[1].damageRules.size() &&
			runtimeModule->sizeVariants[1].attributeModifiers[0].modifier.attributeId ==
				runtimeValueTwin->sizeVariants[1].attributeModifiers[0].modifier.attributeId &&
			runtimeModule->sizeVariants[1].attributeModifiers[0].magnitudeByLevel.values[1] == 4.f &&
			runtimeValueTwin->sizeVariants[1].attributeModifiers[0].magnitudeByLevel.values[1] == 8.f,
			"same behavior structure parses from JSON with only module rule values changed");
		report.Check(levelFourModule->levelCount == 4 && levelFourModule->sizeVariants[0].attributeModifiers[0].magnitudeByLevel.values.size() == 4,
			"N=4 JSON fixture requires four explicit numeric table values");
		report.Check(movementModule->sizeVariants[0].triggerBuffs[0].movementSpeedMultiplierByLevel.values == List<float>{ 1.f, 1.1f, 1.2f },
			"JSON accepts neutral-first movement table with later active levels");

		const std::optional<ModuleRulesContext> playerRulesValue = player.BuildModuleRules();
		report.Check(playerRulesValue.has_value(), "player provides a live schematic for runtime rule fixtures");
		if (!playerRulesValue) return;
		const ModuleRulesContext& playerRules = *playerRulesValue;
		const ModuleLookup moduleLookup = [&definitions = parsedModules.definitions, original = playerRules.lookup](const sas::ContentId& id)
		{
			if (const ModuleDefinition* found = FindDefinition(definitions, id)) return found;
			return original ? original(id) : nullptr;
		};
		const ModuleSideFeatureLookup sideLookup = [&definitions = parsedFeatures.definitions](const sas::ContentId& id)
		{
			return FindFeature(definitions, id);
		};
		const ModuleRulesContext fixtureRules{ playerRules.schematic, moduleLookup, playerRules.zoneTable, sideLookup };
		ModuleLoadout fixtureLoadout;
		while (fixtureLoadout.GetRevision() == ship.GetModuleApplier().GetAppliedRevision())
			fixtureLoadout.AddToInventory(sas::ContentId{ "Module.Test.RevisionSentinel.Common" });

		const sas::ContentId sideId{ "Module.SideFeature.P2RuntimeFixture.Side" };
		const float baseAttack = CurrentOwner(ship, OwnerAttributeIds::AttackPower);
		const float baseMovement = ship.GetMovementSpeedMultiplier();
		const float baseDamageMultiplier = ship.GetCombatRuntime().ResolveOutgoingDamagePlan(
			ship, List<GameplayTag>{ GameplayTags::Ability::Primary }).damageMultiplier;
		if (!InstallFixture(report, fixtureLoadout, fixtureRules, *runtimeModule, 1, 2, sideId, ship)) return;
		const ModulePlacement placement = common_module_e2e::FindInstalled(fixtureLoadout,
			fixtureLoadout.GetPlacements().back().instanceId);
		const std::uint32_t fixtureId = placement.instanceId;
		const float fixtureBonus = ModuleResolver::GetZoneBonus(fixtureRules, *runtimeModule, placement);
		const ResolvedModuleEffects& effects = ship.GetModuleApplier().GetResolvedEffects();
		constexpr std::size_t sideNamespace = std::size_t{ 1 } << (std::numeric_limits<std::size_t>::digits - 1);
		report.Check(effects.triggerBuffs.size() == 2 && effects.triggerBuffs[0].instanceId == fixtureId &&
			effects.triggerBuffs[0].ruleIndex == 0 && effects.triggerBuffs[1].instanceId == fixtureId &&
			effects.triggerBuffs[1].ruleIndex == sideNamespace,
			"main and side trigger rule index zero occupy separate runtime namespaces");
		report.Check(effects.damageRules.size() == 2 && effects.damageRules[0].instanceId == fixtureId &&
			effects.damageRules[0].ruleIndex == 0 && effects.damageRules[1].instanceId == fixtureId &&
			effects.damageRules[1].ruleIndex == sideNamespace,
			"main and side damage rule index zero occupy separate runtime namespaces");
		CheckAttribute(report, "main level-two fixture modifier resolves from JSON on the real ship",
			baseAttack + 4.f * (1.f + fixtureBonus), CurrentOwner(ship, OwnerAttributeIds::AttackPower));
		CheckAttribute(report, "both active damage windows begin closed", baseDamageMultiplier,
			ship.GetCombatRuntime().ResolveOutgoingDamagePlan(ship, List<GameplayTag>{ GameplayTags::Ability::Primary }).damageMultiplier);
		BroadcastAbilityActivated(ship);
		report.Check(ship.GetModuleTriggerRuntime().GetActiveBuffCount() == 2 &&
			ship.GetModuleTriggerRuntime().GetActiveModifierHandleCount() == 4,
			"both namespaced main and side trigger buffs activate on the real ship");
		CheckAttribute(report, "main and side trigger level tables both modify live attack power",
			baseAttack + 4.f * (1.f + fixtureBonus) + 4.f + 20.f, CurrentOwner(ship, OwnerAttributeIds::AttackPower));
		CheckAttribute(report, "main and side trigger movement multipliers both apply",
			baseMovement * 1.1f * 1.2f, ship.GetMovementSpeedMultiplier());
		CheckAttribute(report, "main and side damage windows both multiply real outgoing damage",
			baseDamageMultiplier * 1.1f * 1.3f,
			ship.GetCombatRuntime().ResolveOutgoingDamagePlan(ship, List<GameplayTag>{ GameplayTags::Ability::Primary }).damageMultiplier);

		const ModuleDefinition* canonicalGlass = content::ModuleContentCatalog::FindById("Module.Offense.GlassTopCore.Uncommon");
		const ModuleSideFeatureDefinition* canonicalO2 = content::ModuleSideFeatureCatalog::FindById("Module.SideFeature.Shared.O2");
		const std::filesystem::path emptyFeaturesPath = WriteJson(fixtureRoot, "atomic-reload-empty-features.json",
			Json{ { "schemaVersion", 1 }, { "sharedPools", Json::array() }, { "sideFeatures", Json::array() } });
		std::string reloadFailure;
		const bool invalidReload = content::ModuleContentCatalog::LoadFromFiles(modulesPath, emptyFeaturesPath, &reloadFailure);
		report.Check(!invalidReload && content::ModuleContentCatalog::FindById("Module.Offense.GlassTopCore.Uncommon") == canonicalGlass &&
			content::ModuleSideFeatureCatalog::FindById("Module.SideFeature.Shared.O2") == canonicalO2 &&
			ship.GetModuleApplier().GetResolvedEffects().triggerBuffs.size() == 2,
			"invalid two-file reload preserves live catalog identities and applied rules", reloadFailure);
		report.Check(fixtureLoadout.SetLevel(fixtureRules, fixtureId, 3) && ship.GetModuleApplier().Apply(fixtureLoadout, fixtureRules),
			"live applier remains usable after a failed content reload");
		BroadcastAbilityActivated(ship);
		CheckAttribute(report, "failed catalog reload retains the fixture's level-three live value",
			baseAttack + 5.f * (1.f + fixtureBonus) + 6.f + 30.f, CurrentOwner(ship, OwnerAttributeIds::AttackPower));
		UninstallFixture(report, fixtureLoadout, fixtureRules, fixtureId, ship);
		report.Check(ship.GetModuleTriggerRuntime().GetActiveBuffCount() == 0,
			"removing the temporary package clears both namespaced timed buffs");
		CheckAttribute(report, "main and side movement modifiers clear with their package", baseMovement,
			ship.GetMovementSpeedMultiplier());
		CheckAttribute(report, "main and side damage windows clear with their package", baseDamageMultiplier,
			ship.GetCombatRuntime().ResolveOutgoingDamagePlan(ship, List<GameplayTag>{ GameplayTags::Ability::Primary }).damageMultiplier);

		const float twinAttackBase = CurrentOwner(ship, OwnerAttributeIds::AttackPower);
		const float twinDamageBase = ship.GetCombatRuntime().ResolveOutgoingDamagePlan(
			ship, List<GameplayTag>{ GameplayTags::Ability::Primary }).damageMultiplier;
		if (!InstallFixture(report, fixtureLoadout, fixtureRules, *runtimeValueTwin, 1, 2, sideId, ship)) return;
		const std::uint32_t twinId = fixtureLoadout.GetPlacements().back().instanceId;
		const ModulePlacement twinPlacement = common_module_e2e::FindInstalled(fixtureLoadout, twinId);
		const float twinBonus = ModuleResolver::GetZoneBonus(fixtureRules, *runtimeValueTwin, twinPlacement);
		CheckAttribute(report, "JSON value twin applies its changed main modifier on the real ship",
			twinAttackBase + 8.f * (1.f + twinBonus), CurrentOwner(ship, OwnerAttributeIds::AttackPower));
		BroadcastAbilityActivated(ship);
		CheckAttribute(report, "JSON value twin changes the live trigger modifier by data only",
			twinAttackBase + 8.f * (1.f + twinBonus) + 6.f + 20.f, CurrentOwner(ship, OwnerAttributeIds::AttackPower));
		CheckAttribute(report, "JSON value twin changes the live damage result by data only",
			twinDamageBase * 1.2f * 1.3f,
			ship.GetCombatRuntime().ResolveOutgoingDamagePlan(ship, List<GameplayTag>{ GameplayTags::Ability::Primary }).damageMultiplier);
		UninstallFixture(report, fixtureLoadout, fixtureRules, twinId, ship);
		CheckAttribute(report, "JSON value twin removal clears its damage window", twinDamageBase,
			ship.GetCombatRuntime().ResolveOutgoingDamagePlan(ship, List<GameplayTag>{ GameplayTags::Ability::Primary }).damageMultiplier);

		const float levelFourAPBase = CurrentOwner(ship, OwnerAttributeIds::AttackPower);
		const float levelFourEPBase = CurrentOwner(ship, OwnerAttributeIds::EnergyPower);
		if (!InstallFixture(report, fixtureLoadout, fixtureRules, *levelFourModule, 0, 3, {}, ship)) return;
		const std::uint32_t levelFourId = fixtureLoadout.GetPlacements().back().instanceId;
		report.Check(fixtureLoadout.SetLevel(fixtureRules, levelFourId, 4) && ship.GetModuleApplier().Apply(fixtureLoadout, fixtureRules),
			"N=4 loadout mutation reaches level four and reapplies the real ship");
		const ModulePlacement levelFourPlacement = common_module_e2e::FindInstalled(fixtureLoadout, levelFourId);
		const float levelFourBonus = ModuleResolver::GetZoneBonus(fixtureRules, *levelFourModule, levelFourPlacement);
		CheckAttribute(report, "N=4 fixture level four applies real AttackPower", levelFourAPBase + 24.f * (1.f + levelFourBonus),
			CurrentOwner(ship, OwnerAttributeIds::AttackPower));
		CheckAttribute(report, "N=4 fixture level four applies real EnergyPower", levelFourEPBase + 38.f * (1.f + levelFourBonus),
			CurrentOwner(ship, OwnerAttributeIds::EnergyPower));
		UninstallFixture(report, fixtureLoadout, fixtureRules, levelFourId, ship);

		if (!InstallFixture(report, fixtureLoadout, fixtureRules, *movementModule, 0, 2, {}, ship)) return;
		const std::uint32_t movementId = fixtureLoadout.GetPlacements().back().instanceId;
		BroadcastAbilityActivated(ship);
		report.Check(ship.GetModuleTriggerRuntime().GetActiveBuffCount() == 1,
			"neutral-first movement table activates from its non-neutral level on the real ship");
		CheckAttribute(report, "neutral-first movement table applies its level-two multiplier",
			baseMovement * 1.1f, ship.GetMovementSpeedMultiplier());
		UninstallFixture(report, fixtureLoadout, fixtureRules, movementId, ship);
		report.Check(ship.GetModuleTriggerRuntime().GetActiveBuffCount() == 0,
			"neutral-first movement fixture cleans up its live timed buff");
		report.Check(ship.GetModuleApplier().Apply(player.GetModuleLoadout(), playerRules) &&
			ship.GetModuleApplier().GetAppliedRevision() == player.GetModuleLoadout().GetRevision(),
			"restore Player-owned loadout and lookup after temporary rule fixtures");
	}
}
