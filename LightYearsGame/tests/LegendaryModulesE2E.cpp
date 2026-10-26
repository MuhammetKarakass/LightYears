#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"

namespace
{
	using namespace uncommon_p4_e2e;

	void LoaderChecks(Report& report)
	{
		Json source; std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json" }; input >> source;
		const auto loaded = content::ModuleLoader::LoadFromJson(source);
		report.Check(loaded.Succeeded(), "all shipped definitions load", loaded.error);
		int count = 0;
		for (const auto& module : loaded.definitions) if (module.rarity == ModuleRarity::Legendary) ++count;
		report.Check(count == 12, "exactly twelve Legendary main modules load", count);
		const auto entry = std::find_if(source["modules"].begin(), source["modules"].end(), [](const Json& module) { return module.value("id", "") == "Module.Primary.SecondHardpoint.Legendary"; });
		report.Check(entry != source["modules"].end(), "invalid fixtures start from shipped Legendary");
		const auto rejects = [&](const char* name, const Json& invalid, const char* diagnostic)
		{
			Json fixture = source; fixture["modules"] = Json::array({ invalid });
			const auto result = content::ModuleLoader::LoadFromJson(fixture);
			report.Check(!result.Succeeded() && result.definitions.empty() && result.error.find(diagnostic) != std::string::npos, name, result.error);
		};
		Json invalid = *entry; invalid["sizeVariants"].push_back(invalid["sizeVariants"][0]);
		rejects("Legendary rejects multiple size variants", invalid, "exactly one size variant");
		invalid = *entry; invalid["sideFeatures"] = Json::array({ "ModuleSideFeature.Offense.CriticalChance" });
		rejects("Legendary rejects a side feature", invalid, "side features");
		invalid = *entry; invalid["legendary"]["kind"] = "UnknownLegendary";
		rejects("Legendary rejects an unknown kind", invalid, "unknown Legendary kind");
		invalid = *entry; invalid["legendary"]["kind"] = "BlinkDrive";
		rejects("Legendary kind must match its family", invalid, "does not match module family");
		invalid = *entry; invalid.erase("legendary");
		rejects("Legendary requires its rule", invalid, "legendary");
		invalid = *entry; invalid["legendary"]["markDamageBonus"] = Json::array({ .2, .25 });
		rejects("Legendary rejects a short level table", invalid, "shorter than levelCount");
		invalid = *entry; invalid["legendary"]["markDamageBonus"] = Json::array({ .2, .1, .01 });
		rejects("Legendary validates extrapolated zone levels", invalid, "finite and positive");
		invalid = *entry; invalid["legendary"]["typo"] = 1;
		rejects("Legendary rejects unknown numeric fields", invalid, "unknown Legendary field");
	}

	template<typename ReadResolved, typename ReadRuntime, typename ReadValue>
	void InstalledChecks(Report& report, Application& application, const char* id, const GameplayTag& family, const List<sf::Vector2i>& shape, float first, float step, ReadResolved readResolved, ReadRuntime readRuntime, ReadValue readValue)
	{
		const auto* module = content::ModuleContentCatalog::FindById(id);
		report.Check(module && module->family == family && module->rarity == ModuleRarity::Legendary && module->levelCount == 3 && module->legendary.has_value(), "Legendary identity, family, rarity and progression", id);
		report.Check(module->sizeVariants.size() == 1 && module->sizeVariants[0].shape.cells == shape, "Legendary fixed shape and cell count", id);
		report.Check(module->sideFeatures.empty() && module->sharedPools.empty(), "Legendary has no private or shared side features", id);
		Fixture f{ report, application };
		const auto defaults = f.loadout.GetPlacements();
		for (const auto& placement : defaults) report.Check(f.loadout.Remove(placement.instanceId), "remove fixture defaults");
		const auto familyZone = family == GameplayTags::Ability::Primary ? GameplayTags::Module::Zone::Nose : family == GameplayTags::Ability::Offense ? GameplayTags::Module::Zone::Wings : family == GameplayTags::Ability::Defense ? GameplayTags::Module::Zone::Core : family == GameplayTags::Ability::Movement ? GameplayTags::Module::Zone::Engine : GameplayTags::Module::Zone::Sensor;
		const auto otherZone = family == GameplayTags::Ability::Primary ? GameplayTags::Module::Zone::Engine : GameplayTags::Module::Zone::Nose;
		for (const auto& zone : { otherZone, familyZone, GameplayTags::Module::Zone::Hull })
		{
			ShipSchematic schematic; schematic.width = 4; schematic.height = 4; schematic.cellZones.assign(16, zone);
			ModuleRulesContext rules{ schematic, f.rules.lookup, f.rules.zoneTable, f.rules.sideFeatureLookup };
			for (int level = 1; level <= 3; ++level)
			{
				const auto instance = f.loadout.AddToInventory(rules, *module, 0, module->rarity, level);
				report.Check(instance != 0 && f.loadout.Install(rules, instance, { 0, 0 }, 0) == ModulePlacementFailure::None, "install shipped Legendary through real loadout", id);
				auto& applier = f.ship->GetModuleApplier();
				report.Check(applier.Apply(f.loadout, rules), "rebuild applies Legendary");
				const auto& effects = applier.GetResolvedEffects();
				const int bonus = zone == GameplayTags::Module::Zone::Hull ? 2 : zone == familyZone ? 1 : 0;
				report.Check(ModuleResolver::FindZoneLevelBonus(effects, instance) == bonus, "Legendary receives normal zone bonus", bonus);
				const int effective = level + bonus;
				const auto& resolved = readResolved(effects.legendaryRules);
				const auto& runtime = readRuntime(applier).GetRules();
				report.Check(resolved.size() == 1 && runtime.size() == 1 && resolved[0].instanceId == instance && runtime[0].instanceId == instance, "exactly one resolved rule reaches its owner runtime", id);
				const std::string input = std::string{ id } + ", natural L" + std::to_string(level) + ", zone " + zone.name + ", effective L" + std::to_string(effective);
				report.CheckValue("resolved Legendary reads effective-level numbers", first + step * (effective - 1), readValue(resolved[0]), input);
				report.CheckValue("runtime receives level-correct numbers", readValue(resolved[0]), readValue(runtime[0]), input);
				applier.Tick(.25f);
				report.Check(readRuntime(applier).GetRules().size() == 1, "foundation tick preserves installed rules");
				report.Check(f.loadout.Uninstall(instance) && applier.Apply(f.loadout, rules), "Legendary uninstall rebuilds owner");
				report.Check(readResolved(applier.GetResolvedEffects().legendaryRules).empty() && readRuntime(applier).GetRules().empty(), "Legendary uninstall clears resolved and runtime rules");
			}
		}
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext context; vega::EngineContext::Scope scope{ context }; vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		LoaderChecks(report);
		report.Check(GameContentBootstrap::Register(), "register shipped game content");
		const List<sf::Vector2i> domino{ { 0, 0 }, { 1, 0 } }, elbow{ { 0, 0 }, { 1, 0 }, { 1, 1 } }, square{ { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
		std::string title = "Legendary modules foundation E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		InstalledChecks(report, application, "Module.Primary.SecondHardpoint.Legendary", GameplayTags::Ability::Primary, elbow, 0.2f, 0.05f, [](const auto& rules) -> const auto& { return rules.secondHardpoints; }, [](const auto& applier) -> const auto& { return applier.GetLegendarySecondHardpointRuntime(); }, [](const auto& rule) { return rule.markDamageBonus; });
		InstalledChecks(report, application, "Module.Primary.MeltdownBarrel.Legendary", GameplayTags::Ability::Primary, domino, 0.3f, 0.04f, [](const auto& rules) -> const auto& { return rules.meltdownBarrels; }, [](const auto& applier) -> const auto& { return applier.GetLegendaryMeltdownBarrelRuntime(); }, [](const auto& rule) { return rule.novaBudgetFraction; });
		InstalledChecks(report, application, "Module.Offense.PrimedArsenal.Legendary", GameplayTags::Ability::Offense, elbow, 0.15f, 0.05f, [](const auto& rules) -> const auto& { return rules.primedArsenals; }, [](const auto& applier) -> const auto& { return applier.GetLegendaryPrimedArsenalRuntime(); }, [](const auto& rule) { return rule.powerPerReadySecond; });
		InstalledChecks(report, application, "Module.Offense.ReactionCore.Legendary", GameplayTags::Ability::Offense, elbow, 2.f, 0.2f, [](const auto& rules) -> const auto& { return rules.reactionCores; }, [](const auto& applier) -> const auto& { return applier.GetLegendaryReactionCoreRuntime(); }, [](const auto& rule) { return rule.steamSeconds; });
		InstalledChecks(report, application, "Module.Offense.NeuralHijacker.Legendary", GameplayTags::Ability::Offense, elbow, 3.f, 0.4f, [](const auto& rules) -> const auto& { return rules.neuralHijackers; }, [](const auto& applier) -> const auto& { return applier.GetLegendaryNeuralHijackerRuntime(); }, [](const auto& rule) { return rule.allySeconds; });
		InstalledChecks(report, application, "Module.Defense.HardlightHull.Legendary", GameplayTags::Ability::Defense, elbow, 0.5f, 0.04f, [](const auto& rules) -> const auto& { return rules.hardlightHulls; }, [](const auto& applier) -> const auto& { return applier.GetLegendaryHardlightHullRuntime(); }, [](const auto& rule) { return rule.healthToShieldFraction; });
		InstalledChecks(report, application, "Module.Defense.BulwarkPulse.Legendary", GameplayTags::Ability::Defense, domino, 50.f, 7.f, [](const auto& rules) -> const auto& { return rules.bulwarkPulses; }, [](const auto& applier) -> const auto& { return applier.GetLegendaryBulwarkPulseRuntime(); }, [](const auto& rule) { return rule.baseDamage; });
		InstalledChecks(report, application, "Module.Defense.ParasiteHull.Legendary", GameplayTags::Ability::Defense, elbow, 0.2f, 0.1f, [](const auto& rules) -> const auto& { return rules.parasiteHulls; }, [](const auto& applier) -> const auto& { return applier.GetLegendaryParasiteHullRuntime(); }, [](const auto& rule) { return rule.borrowedHealthBonus; });
		InstalledChecks(report, application, "Module.Movement.BlinkDrive.Legendary", GameplayTags::Ability::Movement, domino, 6.f, -0.3f, [](const auto& rules) -> const auto& { return rules.blinkDrives; }, [](const auto& applier) -> const auto& { return applier.GetLegendaryBlinkDriveRuntime(); }, [](const auto& rule) { return rule.rechargeSeconds; });
		InstalledChecks(report, application, "Module.Movement.IonComet.Legendary", GameplayTags::Ability::Movement, elbow, 6.f, 1.5f, [](const auto& rules) -> const auto& { return rules.ionComets; }, [](const auto& applier) -> const auto& { return applier.GetLegendaryIonCometRuntime(); }, [](const auto& rule) { return rule.tickDamage; });
		InstalledChecks(report, application, "Module.Utility.TwinRack.Legendary", GameplayTags::Ability::Utility, square, 0.1f, 0.02f, [](const auto& rules) -> const auto& { return rules.twinRacks; }, [](const auto& applier) -> const auto& { return applier.GetLegendaryTwinRackRuntime(); }, [](const auto& rule) { return rule.pairedRefundFraction; });
		InstalledChecks(report, application, "Module.Utility.UnderdogCore.Legendary", GameplayTags::Ability::Utility, domino, 1.f, 0.125f, [](const auto& rules) -> const auto& { return rules.underdogCores; }, [](const auto& applier) -> const auto& { return applier.GetLegendaryUnderdogCoreRuntime(); }, [](const auto& rule) { return rule.offensePowerPerWeight; });
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "LightYearsLegendaryModulesE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
