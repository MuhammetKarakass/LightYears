#include "gameFramework/LightYearsAssets.h"
#include "CommonModuleE2ETestSupport.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"

#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>

namespace
{
	using namespace ly;
	using namespace common_module_e2e;

	// Subjects are shipped modules, reduced in a fixture copy. Each gets a single owner probe (Owner.AttackSpeed,
	// Add 100, zone-scaled), so a placed module's zone bonus reads back as 100 * (1 + bonus). Zone rules exist only in the fixture.
	constexpr const char* kTarget = "Module.Primary.FocusedBarrel.Basic";
	constexpr const char* kAmplifier = "Module.Offense.GlassTopCore.Uncommon";
	constexpr const char* kOffense = "Module.Offense.AssaultCore.Basic";
	constexpr const char* kHull = "Module.Defense.HullReinforcement.Basic";
	constexpr const char* kMisfit = "Module.Utility.DoubleFeedCore.Uncommon";
	constexpr const char* kSecondAmplifier = "Module.Movement.ThrustBooster.Basic";
	constexpr const char* kShieldReturn = "Module.SideFeature.GlassTop.ShieldReturn";

	// Thrown when a scenario does not fit one player schematic; the runner then tries the next ship.
	struct NoZonePlacement : std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};

	Json ShippedDocument(const char* name)
	{
		Json document;
		std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data" / name };
		input >> document;
		return document;
	}

	void WriteJson(const std::filesystem::path& path, const Json& document)
	{
		std::ofstream output{ path };
		output << document.dump(2);
	}

	Json OwnerProbeJson()
	{
		return Json{ { "target", "Owner" }, { "attributeId", "Owner.AttackSpeed" }, { "operation", "Add" }, { "magnitude", 100.0 }, { "affectedByZoneBonus", true } };
	}

	Json ZoneRuleJson(const char* kind, Json bonus)
	{
		return Json{ { "kind", kind }, { "bonus", std::move(bonus) } };
	}

	// Keeps the subject's first variant but shrinks it to one cell, so zone scenarios fit any player schematic.
	void ReduceToSingleCell(Json& module)
	{
		Json variant = module["sizeVariants"][0];
		variant["shape"] = Json::array({ Json::array({ 0, 0 }) });
		module["sizeVariants"] = Json::array({ variant });
		module.erase("sizeLadder");
	}

	// Reduced document: shipped schema, zone table and system zones, with only the test subjects as modules.
	// Shipped descriptions reference their own attributes by token, so each subject gets a plain description.
	Json ZoneFixtureModules()
	{
		const Json shipped = ShippedDocument("modules.json");
		Json fixture = shipped;
		fixture["modules"] = Json::array();
		for (const Json& module : shipped.at("modules"))
		{
			const std::string id = module.at("id").get<std::string>();
			const bool subject = id == kTarget || id == kAmplifier || id == kOffense || id == kHull || id == kMisfit || id == kSecondAmplifier;
			if (!subject) continue;
			Json copy = module;
			copy["description"] = "Zone rules fixture subject.";
			copy.erase("sharedPools");
			for (Json& variant : copy["sizeVariants"]) variant["attributeModifiers"] = Json::array({ OwnerProbeJson() });
			if (id == kTarget || id == kOffense || id == kHull || id == kSecondAmplifier) ReduceToSingleCell(copy);
			if (id == kAmplifier) copy["sizeVariants"][0]["zoneRules"] = Json::array({ ZoneRuleJson("AmplifyZone", Json::array({ 0.5, 0.55, 0.6 })) });
			if (id == kSecondAmplifier) copy["sizeVariants"][0]["zoneRules"] = Json::array({ ZoneRuleJson("AmplifyZone", 0.6) });
			if (id == kMisfit) copy["sizeVariants"][0]["zoneRules"] = Json::array({ ZoneRuleJson("OutOfZoneMainBonus", Json::array({ 0.25, 0.30, 0.35 })) });
			fixture["modules"].push_back(std::move(copy));
		}
		return fixture;
	}

	Json ZoneFixtureSideFeatures()
	{
		// The shipped shield-return feature is not zone-scaled; the fixture opts it in so a leak into the side package would show.
		Json sideFeatures = ShippedDocument("module_side_features.json");
		for (Json& feature : sideFeatures["sideFeatures"])
		{
			if (feature["id"].get<std::string>() == kShieldReturn) feature["rules"]["attributeModifiers"][0]["affectedByZoneBonus"] = true;
		}
		return sideFeatures;
	}

	struct ZoneFixture
	{
		List<ModuleDefinition> modules;
		ModuleZoneTable zones;
		List<ModuleSideFeatureDefinition> sideFeatures;

		ModuleLookup Lookup() const
		{
			return [this](const sas::ContentId& id) -> const ModuleDefinition*
			{
				for (const ModuleDefinition& module : modules)
				{
					if (module.moduleId.ToString() == id.ToString()) return &module;
				}
				return nullptr;
			};
		}

		ModuleSideFeatureLookup SideLookup() const
		{
			return [this](const sas::ContentId& id) -> const ModuleSideFeatureDefinition*
			{
				for (const ModuleSideFeatureDefinition& feature : sideFeatures)
				{
					if (feature.sideFeatureId.ToString() == id.ToString()) return &feature;
				}
				return nullptr;
			};
		}
	};

	struct ZoneLayout
	{
		Report& report;
		ModuleRulesContext rules;
		ModuleLoadout loadout;

		ZoneLayout(Report& result, ModuleLookup lookup, const ModuleZoneTable& zones, ModuleSideFeatureLookup sideLookup, const std::string& shipId)
			: report{ result }, rules{ *content::ShipContentCatalog::GetPlayerShipDefinition(shipId).schematic, std::move(lookup), zones, std::move(sideLookup) }
		{
		}
	};

	// Installs one instance on the first legal origin and rotation whose covered cells all lie in the zone.
	// Throws NoZonePlacement when this schematic has no such place.
	std::uint32_t PlaceInZone(ZoneLayout& layout, const char* moduleId, int variantIndex, int level, const GameplayTag& zone, const char* sideFeature = "")
	{
		const ModuleDefinition* definition = layout.rules.lookup(moduleId);
		layout.report.Check(definition != nullptr, std::string{ "zone fixture defines " } + moduleId);
		const std::uint32_t instance = layout.loadout.AddToInventory(layout.rules, *definition, variantIndex, definition->rarity, level, sas::ContentId{ sideFeature });
		layout.report.Check(instance != 0, std::string{ "inventory accepts " } + moduleId);
		ModulePlacement candidate = *layout.loadout.FindInventoryItem(instance);
		for (int rotation = 0; rotation < 4; ++rotation)
		{
			for (int y = 0; y < layout.rules.schematic.height; ++y)
			{
				for (int x = 0; x < layout.rules.schematic.width; ++x)
				{
					candidate.origin = { x, y };
					candidate.rotationQuarterTurns = rotation;
					if (ModulePlacementRules::CanPlace(layout.rules, layout.loadout, candidate) != ModulePlacementFailure::None) continue;
					bool inZone = true;
					for (const sf::Vector2i& cell : ModulePlacementRules::GetCoveredCells(*definition, candidate))
						inZone = inZone && layout.rules.schematic.GetZoneAt(cell.x, cell.y) == zone;
					if (!inZone || layout.loadout.Install(layout.rules, instance, candidate.origin, rotation) != ModulePlacementFailure::None) continue;
					return instance;
				}
			}
		}
		throw NoZonePlacement{ std::string{ "no legal zone placement for " } + moduleId };
	}

	using ScenarioBody = std::function<void(ZoneLayout&)>;

	// Runs one scenario on the first player ship whose schematic fits every placement. Checks recorded by a
	// rejected attempt are discarded; a scenario that fits no ship fails with a named check.
	void RunOnAnyShip(Report& report, const ModuleLookup& lookup, const ModuleZoneTable& zones, const ModuleSideFeatureLookup& sideLookup, const std::string& scenario, const ScenarioBody& body)
	{
		for (const std::string& shipId : content::ShipContentCatalog::GetPlayerShipIds())
		{
			Report attempt;
			try
			{
				ZoneLayout layout{ attempt, lookup, zones, sideLookup, shipId };
				body(layout);
			}
			catch (const NoZonePlacement&)
			{
				continue;
			}
			Json& checks = report.artifact["checks"];
			for (const Json& check : attempt.artifact["checks"]) checks.push_back(check);
			return;
		}
		report.Check(false, "no player ship schematic fits the scenario: " + scenario);
	}

	ResolvedModuleEffects ResolveLayout(const ZoneLayout& layout)
	{
		return ModuleResolver::Resolve(layout.rules, layout.loadout.GetPlacements());
	}

	// Every test module carries exactly one owner modifier, so the index follows placement order.
	float OwnerProbe(const ZoneLayout& layout, const ResolvedModuleEffects& effects, std::size_t index)
	{
		layout.report.Check(index < effects.ownerModifiers.size(), "zone probe modifier is present");
		return effects.ownerModifiers[index].magnitude;
	}

	float ProbeValue(float bonus)
	{
		return 100.f * (1.f + bonus);
	}

	void CheckLoaderRejections(Report& report)
	{
		const auto rejects = [&report](const char* label, const Json& zoneRules)
		{
			bool rejected = false;
			try { content::ModuleLoader::ParseRulePackage(Json{ { "zoneRules", zoneRules } }, "zone invalid fixture", 3, true); }
			catch (const std::exception&) { rejected = true; }
			report.Check(rejected, label, zoneRules);
		};
		rejects("loader rejects AmplifyZone below the reference bonus", Json::array({ ZoneRuleJson("AmplifyZone", 0.1) }));
		rejects("loader rejects AmplifyZone above one", Json::array({ ZoneRuleJson("AmplifyZone", 1.5) }));
		rejects("loader rejects OutOfZoneMainBonus of zero", Json::array({ ZoneRuleJson("OutOfZoneMainBonus", 0.0) }));
		rejects("loader rejects OutOfZoneMainBonus above one", Json::array({ ZoneRuleJson("OutOfZoneMainBonus", 1.5) }));
		rejects("loader rejects an unknown zone rule kind", Json::array({ ZoneRuleJson("AmplifyZoneX", 0.5) }));
		rejects("loader rejects a duplicated zone rule kind", Json::array({ ZoneRuleJson("AmplifyZone", 0.5), ZoneRuleJson("AmplifyZone", 0.6) }));
		Json withField = ZoneRuleJson("AmplifyZone", 0.5);
		withField["radius"] = 2;
		rejects("loader rejects an unknown zone rule field", Json::array({ withField }));
		rejects("loader rejects a short zone rule level table", Json::array({ ZoneRuleJson("AmplifyZone", Json::array({ 0.5, 0.55 })) }));
		rejects("loader rejects an out-of-range level table entry", Json::array({ ZoneRuleJson("OutOfZoneMainBonus", Json::array({ 0.25, 0.30, 1.2 })) }));
		rejects("loader rejects a non-array zoneRules", Json::object());

		const auto package = content::ModuleLoader::ParseRulePackage(
			Json{ { "zoneRules", Json::array({ ZoneRuleJson("AmplifyZone", Json::array({ 0.5, 0.55, 0.6 })), ZoneRuleJson("OutOfZoneMainBonus", Json::array({ 0.25, 0.30, 0.35 })) }) } },
			"zone valid fixture", 3, true);
		report.Check(package.zoneRules.size() == 2 && HasModuleRulePackageLevelCoverage(package, 3) && !HasModuleRulePackageLevelCoverage(package, 4),
			"loader accepts both zone rule kinds and covers exactly their 3-level tables");
	}

	void CheckAmplifierScenarios(Report& report, const ZoneFixture& fixture)
	{
		const ModuleLookup lookup = fixture.Lookup();
		const ModuleSideFeatureLookup sideLookup = fixture.SideLookup();
		const ModuleZoneTable& zones = fixture.zones;
		const GameplayTag& nose = GameplayTags::Module::Zone::Nose;
		const GameplayTag& wings = GameplayTags::Module::Zone::Wings;
		const GameplayTag& hull = GameplayTags::Module::Zone::Hull;
		const GameplayTag& core = GameplayTags::Module::Zone::Core;

		RunOnAnyShip(report, lookup, zones, sideLookup, "standard Nose bonus", [&](ZoneLayout& layout)
		{
			PlaceInZone(layout, kTarget, 0, 1, nose);
			layout.report.CheckValue("zone rules: without an amplifier the Nose module keeps the standard 0.25", ProbeValue(0.25f), OwnerProbe(layout, ResolveLayout(layout), 0), "Primary module in Nose");
		});

		// Zone Overclocker at levels 1-3 raises the other Nose module to 0.50 / 0.55 / 0.60.
		const float amplifierBonus[] = { 0.50f, 0.55f, 0.60f };
		for (int level = 1; level <= 3; ++level)
		{
			RunOnAnyShip(report, lookup, zones, sideLookup, "amplifier level " + std::to_string(level), [&](ZoneLayout& layout)
			{
				PlaceInZone(layout, kTarget, 0, 1, nose);
				PlaceInZone(layout, kAmplifier, 0, level, nose);
				const ResolvedModuleEffects effects = ResolveLayout(layout);
				layout.report.CheckValue("zone rules: amplifier level " + std::to_string(level) + " raises the other Nose module",
					ProbeValue(amplifierBonus[level - 1]), OwnerProbe(layout, effects, 0), "amplifier shares Nose");
				layout.report.CheckValue("zone rules: the amplifier's out-of-family Nose module gets no bonus from its own zone", ProbeValue(0.f), OwnerProbe(layout, effects, 1), "amplifier is Offense in Nose");
			});
		}

		RunOnAnyShip(report, lookup, zones, sideLookup, "amplifier in Nose, module in Wings", [&](ZoneLayout& layout)
		{
			PlaceInZone(layout, kTarget, 0, 1, nose);
			PlaceInZone(layout, kAmplifier, 0, 1, nose);
			PlaceInZone(layout, kOffense, 0, 1, wings);
			const ResolvedModuleEffects effects = ResolveLayout(layout);
			layout.report.CheckValue("zone rules: an amplifier in Nose leaves a Wings module at its standard 0.25", ProbeValue(0.25f), OwnerProbe(layout, effects, 2), "Offense module in Wings");
		});

		RunOnAnyShip(report, lookup, zones, sideLookup, "amplifier raises own zone", [&](ZoneLayout& layout)
		{
			PlaceInZone(layout, kAmplifier, 0, 1, wings);
			PlaceInZone(layout, kOffense, 0, 1, wings);
			const ResolvedModuleEffects effects = ResolveLayout(layout);
			layout.report.CheckValue("zone rules: the amplifier keeps its own standard 0.25 in its zone", ProbeValue(0.25f), OwnerProbe(layout, effects, 0), "amplifier in own Wings zone");
			layout.report.CheckValue("zone rules: the amplifier raises the other Wings module to 0.50", ProbeValue(0.50f), OwnerProbe(layout, effects, 1), "Offense module in Wings");
		});

		// Hull has a 0.10 standard bonus that every family takes, so the amplifier scales it: 0.10 x (0.50 / 0.25) = 0.20.
		const float hullExpected[] = { 0.20f, 0.22f };
		for (int level = 1; level <= 2; ++level)
		{
			RunOnAnyShip(report, lookup, zones, sideLookup, "Hull amplifier level " + std::to_string(level), [&](ZoneLayout& layout)
			{
				PlaceInZone(layout, kHull, 0, 1, hull);
				PlaceInZone(layout, kAmplifier, 0, level, hull);
				const ResolvedModuleEffects effects = ResolveLayout(layout);
				layout.report.CheckValue("zone rules: the Hull bonus scales with the amplifier level (0.20 at L1, 0.22 at L2)",
					ProbeValue(hullExpected[level - 1]), OwnerProbe(layout, effects, 0), "Hull module with amplifier in Hull");
				layout.report.CheckValue("zone rules: the Hull amplifier keeps its own 0.10", ProbeValue(0.10f), OwnerProbe(layout, effects, 1), "amplifier in Hull");
			});
		}

		// Two amplifiers in one zone do not stack: each module takes the largest other amplifier (0.60 scales to 0.24 in Hull).
		RunOnAnyShip(report, lookup, zones, sideLookup, "two amplifiers in Hull", [&](ZoneLayout& layout)
		{
			PlaceInZone(layout, kHull, 0, 1, hull);
			PlaceInZone(layout, kSecondAmplifier, 0, 1, hull);
			PlaceInZone(layout, kAmplifier, 0, 1, hull);
			const ResolvedModuleEffects effects = ResolveLayout(layout);
			layout.report.CheckValue("zone rules: two amplifiers take the larger value, not their sum (Hull module)", ProbeValue(0.24f), OwnerProbe(layout, effects, 0), "Hull module, amplifiers 0.60 and 0.50");
			layout.report.CheckValue("zone rules: the 0.60 amplifier is raised only by the other amplifier (0.50), not by itself", ProbeValue(0.20f), OwnerProbe(layout, effects, 1), "second amplifier, Hull");
			layout.report.CheckValue("zone rules: the 0.50 amplifier is raised by the largest other amplifier (0.60)", ProbeValue(0.24f), OwnerProbe(layout, effects, 2), "amplifier, Hull");
		});

		// Removing the amplifier restores the standard bonus on the next resolve.
		RunOnAnyShip(report, lookup, zones, sideLookup, "amplifier removal", [&](ZoneLayout& layout)
		{
			PlaceInZone(layout, kTarget, 0, 1, nose);
			const std::uint32_t amplifier = PlaceInZone(layout, kAmplifier, 0, 1, nose);
			layout.report.CheckValue("zone rules: the amplified bonus is active before removal", ProbeValue(0.50f), OwnerProbe(layout, ResolveLayout(layout), 0), "Nose module with amplifier");
			layout.report.Check(layout.loadout.Uninstall(amplifier), "zone rules: the amplifier uninstalls through the real loadout");
			layout.report.CheckValue("zone rules: removing the amplifier returns the Nose module to 0.25", ProbeValue(0.25f), OwnerProbe(layout, ResolveLayout(layout), 0), "Nose module after removal");
		});

		// Misfit Wiring: outside its own family's zone a module takes the carrier's level bonus in the main package.
		RunOnAnyShip(report, lookup, zones, sideLookup, "misfit in own and foreign zones", [&](ZoneLayout& layout)
		{
			PlaceInZone(layout, kMisfit, 0, 2, wings);
			PlaceInZone(layout, kOffense, 0, 1, wings);
			const ResolvedModuleEffects effects = ResolveLayout(layout);
			layout.report.CheckValue("zone rules: out-of-family carrier takes the level 2 misfit bonus 0.30 in its main package", ProbeValue(0.30f), OwnerProbe(layout, effects, 0), "Utility carrier in Wings");
			layout.report.CheckValue("zone rules: a module in its own family zone is unaffected by misfit (0.25)", ProbeValue(0.25f), OwnerProbe(layout, effects, 1), "Offense module in Wings");
		});

		// The side package keeps the standard bonus (0 out of family), so the shield-return side stays at 1.06 unscaled.
		RunOnAnyShip(report, lookup, zones, sideLookup, "misfit side feature", [&](ZoneLayout& layout)
		{
			PlaceInZone(layout, kAmplifier, 1, 1, core, kShieldReturn);
			PlaceInZone(layout, kMisfit, 0, 2, core);
			const ResolvedModuleEffects effects = ResolveLayout(layout);
			layout.report.CheckValue("zone rules: out-of-family main package takes the misfit bonus (0.30)", ProbeValue(0.30f), OwnerProbe(layout, effects, 0), "3-cell GlassTop in Core");
			layout.report.Check(effects.shipModifiers.size() == 1, "zone rules: the shield-return side feature applies once");
			layout.report.CheckValue("zone rules: the misfit bonus does not reach the side feature (1.06 unscaled)", 1.06f, effects.shipModifiers[0].magnitude, "side feature of out-of-family module");
		});
	}

	void CheckEmptyCells(Report& report, const ZoneFixture& fixture)
	{
		RunOnAnyShip(report, fixture.Lookup(), fixture.zones, fixture.SideLookup(), "empty cell count", [&](ZoneLayout& layout)
		{
			const ShipSchematic& schematic = layout.rules.schematic;
			int usable = 0;
			for (int y = 0; y < schematic.height; ++y)
			{
				for (int x = 0; x < schematic.width; ++x)
				{
					if (schematic.GetZoneAt(x, y).IsValid() && !schematic.IsLockedAt(x, y)) ++usable;
				}
			}
			layout.report.CheckValue("empty cells: an empty ship counts every unlocked zoned schematic cell", static_cast<float>(usable),
				static_cast<float>(ModuleResolver::CountEmptyCells(layout.rules, layout.loadout.GetPlacements())), "player schematic");

			const std::uint32_t target = PlaceInZone(layout, kTarget, 0, 1, GameplayTags::Module::Zone::Nose);
			const int covered = static_cast<int>(layout.rules.lookup(kTarget)->sizeVariants[0].shape.cells.size());
			layout.report.CheckValue("empty cells: a placed module removes exactly the cells it covers", static_cast<float>(usable - covered),
				static_cast<float>(ModuleResolver::CountEmptyCells(layout.rules, layout.loadout.GetPlacements())), "module installed");
			layout.report.Check(layout.loadout.Uninstall(target), "empty cells: the module uninstalls through the real loadout");
			layout.report.CheckValue("empty cells: removing the module restores the full count", static_cast<float>(usable),
				static_cast<float>(ModuleResolver::CountEmptyCells(layout.rules, layout.loadout.GetPlacements())), "module removed");
		});
	}

	void CheckShippedBonusesUnchanged(Report& report)
	{
		const ModuleLookup lookup = [](const sas::ContentId& id) { return content::ModuleContentCatalog::FindById(id); };
		const ModuleSideFeatureLookup sideLookup = [](const sas::ContentId& id) { return content::ModuleSideFeatureCatalog::FindById(id); };
		const ModuleZoneTable& zones = content::ModuleContentCatalog::GetZoneTable();

		RunOnAnyShip(report, lookup, zones, sideLookup, "shipped Nose bonus", [&](ZoneLayout& layout)
		{
			PlaceInZone(layout, kTarget, 0, 1, GameplayTags::Module::Zone::Nose);
			const ModulePlacement& placed = layout.loadout.GetPlacements().front();
			layout.report.CheckValue("shipped rules: a Primary module in Nose keeps the 0.25 zone bonus", 0.25f,
				ModuleResolver::GetZoneBonus(layout.rules, *layout.rules.lookup(kTarget), placed), "shipped modules.json, no zone rules");
			const ResolvedModuleEffects effects = ResolveLayout(layout);
			layout.report.CheckValue("shipped rules: the same module's damage multiplier reads 1 + 0.10 x 1.25", 1.125f, effects.primaryWeaponModifiers.front().magnitude, "shipped FocusedBarrel in Nose");
		});
		RunOnAnyShip(report, lookup, zones, sideLookup, "shipped Hull bonus", [&](ZoneLayout& layout)
		{
			PlaceInZone(layout, kHull, 0, 1, GameplayTags::Module::Zone::Hull);
			layout.report.CheckValue("shipped rules: an any-family Hull module keeps the 0.10 zone bonus", 0.10f,
				ModuleResolver::GetZoneBonus(layout.rules, *layout.rules.lookup(kHull), layout.loadout.GetPlacements().front()), "shipped Hull bonus");
		});
		RunOnAnyShip(report, lookup, zones, sideLookup, "shipped Wings module", [&](ZoneLayout& layout)
		{
			PlaceInZone(layout, kTarget, 0, 1, GameplayTags::Module::Zone::Wings);
			layout.report.CheckValue("shipped rules: an out-of-family module keeps a zero zone bonus without zone rules", 0.f,
				ModuleResolver::GetZoneBonus(layout.rules, *layout.rules.lookup(kTarget), layout.loadout.GetPlacements().front()), "shipped Primary module in Wings");
		});
		// AbilityBay accepts only shell modules (ModulePlacementRules), so the bonus is read from the zone table and
		// from each player ship's default shell placements instead of a fresh placement.
		const ModuleZoneEntry* abilityBay = zones.FindZone(GameplayTags::Module::Zone::AbilityBay);
		report.Check(abilityBay != nullptr, "shipped zone table defines the AbilityBay zone");
		report.CheckValue("shipped rules: the AbilityBay zone table bonus is 0", 0.f, abilityBay->bonus, "modules.json zone table");

		int bayDefaults = 0;
		for (const std::string& shipId : content::ShipContentCatalog::GetPlayerShipIds())
		{
			const ShipSchematic& schematic = *content::ShipContentCatalog::GetPlayerShipDefinition(shipId).schematic;
			const ModuleRulesContext rules{ schematic, lookup, zones, sideLookup };
			for (const DefaultModulePlacement& entry : schematic.defaultModules)
			{
				if (schematic.GetZoneAt(entry.origin.x, entry.origin.y) != GameplayTags::Module::Zone::AbilityBay) continue;
				const ModuleDefinition* definition = rules.lookup(entry.moduleId);
				report.Check(definition != nullptr, "default AbilityBay module is defined: " + entry.moduleId.ToString());
				ModulePlacement placement;
				placement.instanceId = 1;
				placement.moduleId = entry.moduleId;
				placement.variantIndex = entry.variantIndex;
				placement.origin = entry.origin;
				placement.rotationQuarterTurns = entry.rotationQuarterTurns;
				report.CheckValue("shipped rules: default " + entry.moduleId.ToString() + " in " + shipId + " AbilityBay keeps a zero zone bonus", 0.f,
					ModuleResolver::GetZoneBonus(rules, *definition, placement), "default shell placement");
				++bayDefaults;
			}
		}
		report.Check(bayDefaults > 0, "at least one player default places a shell module in AbilityBay");
	}

	void RunChecks(Report& report, const std::filesystem::path& fixtureRoot)
	{
		std::filesystem::create_directories(fixtureRoot);
		WriteJson(fixtureRoot / "modules.json", ZoneFixtureModules());
		WriteJson(fixtureRoot / "module_side_features.json", ZoneFixtureSideFeatures());

		auto loadedModules = content::ModuleLoader::LoadFromFile(fixtureRoot / "modules.json");
		report.Check(loadedModules.Succeeded(), "zone fixture modules load through the real loader", loadedModules.error);
		auto loadedSides = content::ModuleSideFeatureLoader::LoadFromFile(fixtureRoot / "module_side_features.json");
		report.Check(loadedSides.Succeeded(), "zone fixture side features load through the real loader", loadedSides.error);

		ZoneFixture fixture;
		fixture.modules = std::move(loadedModules.definitions);
		fixture.zones = std::move(loadedModules.zoneTable);
		fixture.sideFeatures = std::move(loadedSides.definitions);

		CheckLoaderRejections(report);
		CheckAmplifierScenarios(report, fixture);
		CheckEmptyCells(report, fixture);
		CheckShippedBonusesUnchanged(report);
	}
}

int main(int argc, char** argv)
{
	ly::InstallLightYearsEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "rare zone rules content registration failed" };
		std::string title = "Rare Zone Rules E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		RunChecks(report, std::filesystem::path{ argv[1] }.parent_path() / "rare-zone-fixtures");
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareZoneRulesE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
