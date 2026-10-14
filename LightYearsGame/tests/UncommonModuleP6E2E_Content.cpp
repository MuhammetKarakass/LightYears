#include "UncommonModuleP6E2E.h"
#include "gameplay/content/ModuleContentCatalog.h"

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
		reject("missing-guided-lifetime", { { "guidedSalvos", Json::array({ { { "damageFraction", .15f }, { "projectileCount", 3 }, { "speed", 600 }, { "targetRange", 1800 }, { "maxActive", 64 }, { "damageType", "Damage.Type.Kinetic" } } }) } });
		reject("fractional-guided-count", { { "guidedSalvos", Json::array({ { { "projectileCount", 1.5f } } }) } });
		reject("negative-delay", { { "delayedExplosions", Json::array({ { { "damageFraction", .25f }, { "delaySeconds", -1.f }, { "radius", 100 }, { "maxActive", 32 } } }) } });
		reject("zero-mine-fuse", { { "trailMines", Json::array({ { { "damage", { 8, 10, 12 } }, { "damageType", "Damage.Type.Kinetic" }, { "spacing", 50 }, { "windowSeconds", 5 }, { "fuseSeconds", 0 }, { "radius", 100 }, { "maxActive", { 12, 14, 16 } } } }) } });
		reject("short-mine-limit", { { "trailMines", Json::array({ { { "damage", { 8, 10, 12 } }, { "damageType", "Damage.Type.Kinetic" }, { "spacing", 50 }, { "windowSeconds", 5 }, { "fuseSeconds", 1 }, { "radius", 100 }, { "maxActive", { 12, 14 } } } }) } });
		const Json killBase{ { "maxHealthFraction", .03f }, { "radius", 100 }, { "maxActive", 32 }, { "damageType", "Damage.Type.Thermal" }, { "presentationId", "Presentation.Ability.ModuleGenerated.Explosion.Thermal" }, { "requiredTargetStatuses", { "Status.Damage.Ignite" } } };
		const auto killWith = [&killBase](const char* key, const Json& value) { Json rule = killBase; if (value.is_null()) rule.erase(key); else rule[key] = value; return Json{ { "killExplosions", Json::array({ rule }) } }; };
		reject("invalid-kill-status-kind", killWith("requiredTargetStatuses", Json::array({ "Ability.Offense" })));
		reject("kill-missing-damage-type", killWith("damageType", Json{}));
		reject("kill-non-damage-type-tag", killWith("damageType", "Status.Damage.Ignite"));
		reject("kill-missing-presentation-id", killWith("presentationId", Json{}));
		reject("kill-invalid-presentation-id", killWith("presentationId", "not a content id"));
		const Json guidedBase{ { "damageFraction", .15f }, { "projectileCount", 3 }, { "speed", 600 }, { "lifetime", 3 }, { "targetRange", 1800 }, { "maxActive", 64 }, { "damageType", "Damage.Type.Kinetic" } };
		const auto guidedWith = [&guidedBase](const char* key, const Json& value) { Json rule = guidedBase; if (value.is_null()) rule.erase(key); else rule[key] = value; return Json{ { "guidedSalvos", Json::array({ rule }) } }; };
		reject("guided-missing-damage-type", guidedWith("damageType", Json{}));
		reject("guided-non-damage-type-tag", guidedWith("damageType", "Status.Damage.Ignite"));
		reject("guided-type-without-damage", guidedWith("damageFraction", Json{}));
		const Json mineBase{ { "damage", { 8, 10, 12 } }, { "spacing", 50 }, { "windowSeconds", 5 }, { "fuseSeconds", 1 }, { "radius", 100 }, { "maxActive", { 12, 14, 16 } }, { "damageType", "Damage.Type.Kinetic" } };
		const auto mineWith = [&mineBase](const char* key, const Json& value) { Json rule = mineBase; if (value.is_null()) rule.erase(key); else rule[key] = value; return Json{ { "trailMines", Json::array({ rule }) } }; };
		reject("mine-negative-ap-coefficient", mineWith("attackPowerCoefficient", -.1f));
		reject("mine-short-ap-level-table", mineWith("attackPowerCoefficient", Json::array({ .3f, .35f })));
		reject("mine-missing-damage-type", mineWith("damageType", Json{}));
		reject("mine-non-damage-type-tag", mineWith("damageType", "Status.Damage.Ignite"));
		reject("mine-type-without-damage", Json{ { "trailMines", Json::array({ Json{ { "cryoStacks", 1 }, { "damageType", "Damage.Type.Kinetic" } } }) } });
		Json sideFile; { std::ifstream sideInput{ dataPath / "module_side_features.json" }; sideInput >> sideFile; }
		for (auto& feature : sideFile["sideFeatures"])
			if (feature["rules"].contains("generatedRules") && feature["rules"]["generatedRules"].contains("guidedSalvos"))
			{
				Json conflicting = guidedBase; conflicting["damageType"] = "Damage.Type.Energy";
				feature["rules"]["generatedRules"]["guidedSalvos"] = Json::array({ conflicting });
			}
		const auto conflictPath = artifactRoot / "catalog-guided-type-conflict.json";
		{ std::ofstream conflictOutput{ conflictPath }; conflictOutput << sideFile.dump(2); }
		std::string conflictReason;
		const bool conflictAccepted = content::ModuleContentCatalog::LoadFromFiles(dataPath / "modules.json", conflictPath, &conflictReason);
		report.Check(!conflictAccepted && conflictReason.find("damage type") != std::string::npos, "catalog rejects a side feature that changes the guided salvo damage type", conflictReason);
		const auto overlap = [&](const char* name, const char* moduleId, const char* key, const Json& rule)
		{
			Json changed = modules;
			for (auto& module : changed["modules"]) if (module["id"] == moduleId) for (auto& variant : module["sizeVariants"]) variant["generatedRules"][key] = Json::array({ rule });
			const auto path = artifactRoot / (std::string{ name } + ".json");
			std::ofstream output{ path }; output << changed.dump(2); output.close();
			std::string reason;
			const bool accepted = content::ModuleContentCatalog::LoadFromFiles(path, dataPath / "module_side_features.json", &reason);
			report.Check(!accepted && reason.find("single-instance") != std::string::npos, std::string{ "catalog rejects a side feature that duplicates a single-instance generated rule: " } + name, reason);
		};
		overlap("catalog-delayed-overlap", "Module.Offense.ScatterCore.Uncommon", "delayedExplosions", Json{ { "damageFraction", .25f }, { "delaySeconds", 1.f }, { "radius", 100 }, { "maxActive", 32 } });
		overlap("catalog-kill-overlap", "Module.Offense.ThermalShockCoil.Uncommon", "killExplosions", killBase);
		const auto loaded = content::ModuleLoader::LoadFromFile(dataPath / "modules.json");
		report.Check(loaded.Succeeded(), "P6 shipped rules load through the real loader", loaded.error);
		const auto side = content::ModuleSideFeatureLoader::LoadFromFile(dataPath / "module_side_features.json");
		report.Check(side.Succeeded(), "P6 generated-only private features use the same validated parser", side.error);
		report.Check(PresentationProfileRegistry<ModuleGuidedPresentationProfile>::Contains(ModuleGeneratedPresentationIds::Guided.ToString()) && PresentationProfileRegistry<ModuleMinePresentationProfile>::Contains(ModuleGeneratedPresentationIds::Mine.ToString()) && PresentationProfileRegistry<ModuleExplosionPresentationProfile>::Contains(ModuleGeneratedPresentationIds::ThermalExplosion.ToString()), "generated presentation content is registered before spawn");
		report.Check(!PresentationProfileRegistry<ModuleMinePresentationProfile>::Contains(ModuleGeneratedPresentationIds::Guided.ToString()), "generated profiles are isolated by concrete type");
		std::size_t killRuleCount = 0; bool killProfilesRegistered = true;
		for (const auto& feature : side.definitions) for (const auto& rule : feature.rules.generatedRules.killExplosions) { ++killRuleCount; killProfilesRegistered = killProfilesRegistered && PresentationProfileRegistry<ModuleExplosionPresentationProfile>::Contains(rule.presentationId.ToString()) && rule.damageType.IsValid(); }
		report.Check(killRuleCount > 0 && killProfilesRegistered, "shipped kill explosions declare a damage type and a registered presentation profile in JSON");
		report.Check(!PresentationProfileRegistry<ModuleGuidedPresentationProfile>::Register({ sas::ContentId{ "Invalid.Profile" } }), "typed profile registry rejects invalid presentation identities");
		ModuleRulePackage shortTable; ModuleRicochetRule rule; rule.damageFractionByLevel.values = { .25f, .3f }; shortTable.generatedRules.ricochets.push_back(rule);
		std::string failure;
		report.Check(!content::ModuleLoader::ValidateRulePackageLevelCount(shortTable, 3, "P6 fixture", &failure) && !HasModuleRulePackageLevelCoverage(shortTable, 3), "public runtime and loader coverage checks agree for generated level tables");
	}
}
