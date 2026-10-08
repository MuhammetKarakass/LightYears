#include "UncommonModuleP6E2E.h"

namespace uncommon_p6_e2e
{
	void ContentChecks(Report& report, const std::filesystem::path& artifactRoot)
	{
		const auto dataPath = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data";
		Json modules; std::ifstream input{ dataPath / "modules.json" }; input >> modules;
		std::filesystem::create_directories(artifactRoot);
		const auto reject = [&](const char* name, const Json& rules)
		{
			Json changed = modules;
			for (auto& module : changed["modules"]) if (module["id"] == "Module.Primary.RicochetTip.Uncommon") module["sizeVariants"][0]["generatedRules"] = rules;
			const auto path = artifactRoot / (std::string{ name } + ".json");
			std::ofstream output{ path }; output << changed.dump(2); output.close();
			const auto loaded = content::ModuleLoader::LoadFromFile(path);
			report.Check(!loaded.Succeeded(), std::string{ "P6 rejects invalid generated content: " } + name, loaded.error);
		};
		reject("unknown-family", { { "future", Json::array() } });
		reject("not-a-list", { { "ricochets", Json::object() } });
		reject("multiple-family-rules", { { "ricochets", Json::array({ { { "damageFraction", .25f }, { "range", 300.f } }, { { "damageFraction", .25f }, { "range", 300.f } } }) } });
		reject("zero-range", { { "ricochets", Json::array({ { { "damageFraction", .25f }, { "range", 0.f } } }) } });
		reject("unknown-field", { { "ricochets", Json::array({ { { "damageFraction", .25f }, { "range", 300.f }, { "critical", true } } }) } });
		reject("short-levels", { { "ricochets", Json::array({ { { "damageFraction", { .25f, .3f } }, { "range", 300.f } } }) } });
		reject("over-one-fraction", { { "ricochets", Json::array({ { { "damageFraction", { .25f, .3f, 1.1f } }, { "range", 300.f } } }) } });
		reject("empty-augment", { { "ricochets", Json::array({ { { "transferStatus", false } } }) } });
		reject("missing-guided-lifetime", { { "guidedSalvos", Json::array({ { { "damageFraction", .15f }, { "projectileCount", 3 }, { "speed", 600 }, { "targetRange", 1800 }, { "maxActive", 64 } } }) } });
		reject("fractional-guided-count", { { "guidedSalvos", Json::array({ { { "projectileCount", 1.5f } } }) } });
		reject("negative-delay", { { "delayedExplosions", Json::array({ { { "damageFraction", .25f }, { "delaySeconds", -1.f }, { "radius", 100 }, { "maxActive", 32 } } }) } });
		reject("zero-mine-fuse", { { "trailMines", Json::array({ { { "damage", { 8, 10, 12 } }, { "spacing", 50 }, { "windowSeconds", 5 }, { "fuseSeconds", 0 }, { "radius", 100 }, { "maxActive", { 12, 14, 16 } } } }) } });
		reject("short-mine-limit", { { "trailMines", Json::array({ { { "damage", { 8, 10, 12 } }, { "spacing", 50 }, { "windowSeconds", 5 }, { "fuseSeconds", 1 }, { "radius", 100 }, { "maxActive", { 12, 14 } } } }) } });
		reject("invalid-kill-status-kind", { { "killExplosions", Json::array({ { { "maxHealthFraction", .03f }, { "radius", 100 }, { "maxActive", 32 }, { "requiredTargetStatuses", { "Ability.Offense" } } } }) } });
		const auto loaded = content::ModuleLoader::LoadFromFile(dataPath / "modules.json");
		report.Check(loaded.Succeeded(), "P6 shipped rules load through the real loader", loaded.error);
		const auto side = content::ModuleSideFeatureLoader::LoadFromFile(dataPath / "module_side_features.json");
		report.Check(side.Succeeded(), "P6 generated-only private features use the same validated parser", side.error);
		report.Check(PresentationProfileRegistry<ModuleGuidedPresentationProfile>::Contains(ModuleGeneratedPresentationIds::Guided.ToString()) && PresentationProfileRegistry<ModuleMinePresentationProfile>::Contains(ModuleGeneratedPresentationIds::Mine.ToString()) && PresentationProfileRegistry<ModuleExplosionPresentationProfile>::Contains(ModuleGeneratedPresentationIds::ThermalExplosion.ToString()), "generated presentation content is registered before spawn");
		report.Check(!PresentationProfileRegistry<ModuleMinePresentationProfile>::Contains(ModuleGeneratedPresentationIds::Guided.ToString()), "generated profiles are isolated by concrete type");
		report.Check(!PresentationProfileRegistry<ModuleGuidedPresentationProfile>::Register({ sas::ContentId{ "Invalid.Profile" } }), "typed profile registry rejects invalid presentation identities");
		ModuleRulePackage shortTable; ModuleRicochetRule rule; rule.damageFractionByLevel.values = { .25f, .3f }; shortTable.generatedRules.ricochets.push_back(rule);
		std::string failure;
		report.Check(!content::ModuleLoader::ValidateRulePackageLevelCount(shortTable, 3, "P6 fixture", &failure) && !HasModuleRulePackageLevelCoverage(shortTable, 3), "public runtime and loader coverage checks agree for generated level tables");
	}
}
