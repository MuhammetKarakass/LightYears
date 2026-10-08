#include "UncommonModuleP5E2E.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"

namespace uncommon_p5_e2e
{
	void ContentChecks(Report& report, const std::filesystem::path& artifactRoot)
	{
		const auto dataPath = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data";
		Json modules;
		std::ifstream input{ dataPath / "modules.json" }; input >> modules;
		std::filesystem::create_directories(artifactRoot);
		const auto reject = [&](const char* name, const Json& rules)
		{
			Json changed = modules;
			for (auto& module : changed["modules"])
				if (module["id"] == "Module.Offense.ArcExtender.Uncommon") module["sizeVariants"][0]["statusRules"] = rules;
			const auto path = artifactRoot / (std::string{ name } + ".json");
			std::ofstream output{ path }; output << changed.dump(2); output.close();
			const auto loaded = content::ModuleLoader::LoadFromFile(path);
			report.Check(!loaded.Succeeded(), std::string{ "P5 rejects invalid content: " } + name, loaded.error);
		};
		reject("unknown-status-rule", { { "futureRule", Json::array() } });
		reject("not-a-list", { { "electricOverrides", Json::object() } });
		reject("empty-electric-table", { { "electricOverrides", Json::array({ { { "damageTakenByStack", Json::array() } } }) } });
		reject("unknown-electric-field", { { "electricOverrides", Json::array({ { { "damageTakenByStack", { 0.03f, 0.06f } }, { "duration", 20.f } } }) } });
		reject("negative-electric-bonus", { { "electricOverrides", Json::array({ { { "damageTakenByStack", { 0.03f, -0.1f } } } }) } });
		reject("short-electric-level-table", { { "electricOverrides", Json::array({ { { "damageTakenByStack", Json::array({ 0.03f, Json::array({ 0.19f, 0.21f }) }) } } }) } });
		reject("zero-freeze-threshold", { { "cryoFreezes", Json::array({ { { "threshold", 0 }, { "durationSeconds", 1.f }, { "cooldownSeconds", 8.f } } }) } });
		reject("negative-freeze-duration", { { "cryoFreezes", Json::array({ { { "threshold", 4 }, { "durationSeconds", { 1.f, -1.f, 1.6f } }, { "cooldownSeconds", 8.f } } }) } });
		reject("short-freeze-level-table", { { "cryoFreezes", Json::array({ { { "threshold", 4 }, { "durationSeconds", { 1.f, 1.3f } }, { "cooldownSeconds", 8.f } } }) } });
		reject("zero-freeze-cooldown", { { "cryoFreezes", Json::array({ { { "threshold", 4 }, { "durationSeconds", 1.f }, { "cooldownSeconds", 0.f } } }) } });
		reject("negative-duration-bonus", { { "thermalCryoDurationBonuses", Json::array({ { { "bonusSeconds", -2.f } } }) } });
		reject("zero-spread-radius", { { "freezeSpreads", Json::array({ { { "radius", 0.f }, { "stacks", 1 } } }) } });
		reject("fractional-spread-stacks", { { "freezeSpreads", Json::array({ { { "radius", 400.f }, { "stacks", 1.5f } } }) } });
		const auto loaded = content::ModuleLoader::LoadFromFile(dataPath / "modules.json");
		report.Check(loaded.Succeeded(), "P5 shipped main rules and levels load through the real loader", loaded.error);
		const auto side = content::ModuleSideFeatureLoader::LoadFromFile(dataPath / "module_side_features.json");
		report.Check(side.Succeeded(), "P5 status-only side features load through the shared rule parser", side.error);
	}
}
