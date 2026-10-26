#include "UncommonModuleCompletionE2E.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"
namespace uncommon_completion_e2e
{
	void ContentChecks(Report& report, const std::filesystem::path& artifactRoot)
	{
		const auto dataPath = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data";
		Json sides; std::ifstream input{ dataPath / "module_side_features.json" }; input >> sides;
		std::filesystem::create_directories(artifactRoot);
		const auto loaded = content::ModuleSideFeatureLoader::LoadFromFile(dataPath / "module_side_features.json");
		report.Check(loaded.Succeeded() && loaded.definitions.size() == 122, "all 122 side definitions load through public loader", loaded.error);
		const auto reject = [&](const char* name, const char* id, const Json& rules)
		{
			Json changed = sides;
			for (auto& side : changed["sideFeatures"]) if (side["id"] == id) side["rules"] = rules;
			const auto path = artifactRoot / (std::string{ name } + ".json");
			std::ofstream output{ path }; output << changed.dump(2); output.close();
			const auto result = content::ModuleSideFeatureLoader::LoadFromFile(path);
			report.Check(!result.Succeeded(), std::string{ "public loader rejects " } + name, result.error);
		};
		reject("fractional-Gold", "Module.SideFeature.Shared.G1", { { "eventActions", Json::array({ { { "trigger", "KillConfirmed" }, { "kind", "AwardGold" }, { "amount", 1.5f } } }) } });
		reject("zero-Gold", "Module.SideFeature.Shared.G1", { { "eventActions", Json::array({ { { "trigger", "KillConfirmed" }, { "kind", "AwardGold" }, { "amount", 0 } } }) } });
		reject("wrong-Gold-trigger", "Module.SideFeature.Shared.G1", { { "eventActions", Json::array({ { { "trigger", "DamageTaken" }, { "kind", "AwardGold" }, { "amount", 1 } } }) } });
		reject("short-duration-levels", "Module.SideFeature.Shared.O4", { { "statusRules", { { "durationBonuses", Json::array({ { { "bonusSeconds", { 1, 2 } } } }) } } } });
		reject("zero-duration", "Module.SideFeature.Shared.O4", { { "statusRules", { { "durationBonuses", Json::array({ { { "bonusSeconds", 0 } } }) } } } });
		reject("wrong-ship-attribute", "Module.SideFeature.GritYourTeeth.ShieldRegen", { { "ownerAttributeRules", Json::array({ { { "source", "HealthThreshold" }, { "target", "Ship" }, { "attributeId", "Ship.Shield.Max" }, { "operation", "Multiply" }, { "healthAtMost", .35f }, { "amount", 2 } } }) } });
		reject("zero-fade", "Module.SideFeature.MovementRhythm.Momentum", { { "movementFadeRules", Json::array({ { { "fadeSeconds", 0 } } }) } });
		reject("extra-fade-field", "Module.SideFeature.MovementRhythm.Momentum", { { "movementFadeRules", Json::array({ { { "fadeSeconds", 1.5f }, { "amount", 10 } } }) } });
		Json modules; std::ifstream moduleInput{ dataPath / "modules.json" }; moduleInput >> modules;
		for (const auto& module : modules["modules"])
		{
			if (module.value("rarity", "") != "Uncommon" || module["sizeVariants"].size() <= 1) continue;
			report.Check(module["sideFeatures"].size() == 2 && module["sharedPools"][0]["allowedFeatureIds"].size() == 3, "every Uncommon size ladder has two private and three shared choices", module["id"]);
		}
	}
}
