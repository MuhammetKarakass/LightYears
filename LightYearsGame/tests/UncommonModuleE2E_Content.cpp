#include "UncommonModuleE2E.h"

#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"

#include <fstream>
#include <vector>

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
			output << value.dump(2);
			return path;
		}

		std::filesystem::path WriteRaw(const std::filesystem::path& root, const std::string& name, const std::string& text)
		{
			std::filesystem::create_directories(root);
			const std::filesystem::path path = root / name;
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			if (!output) throw std::runtime_error{ "Could not write fixture: " + path.string() };
			output << text;
			return path;
		}

		Json& FindModule(Json& modules, const std::string& id)
		{
			for (Json& module : modules.at("modules")) if (module.at("id") == id) return module;
			throw std::runtime_error{ "Fixture module is missing: " + id };
		}

		Json& FindFeature(Json& features, const std::string& id)
		{
			for (Json& feature : features.at("sideFeatures")) if (feature.at("id") == id) return feature;
			throw std::runtime_error{ "Fixture side feature is missing: " + id };
		}

		void CheckModuleRejected(Report& report, const std::filesystem::path& root, const std::string& name, const Json& modules)
		{
			const auto path = WriteJson(root, name + ".json", modules);
			const ly::content::ModuleLoader::Result result = ly::content::ModuleLoader::LoadFromFile(path);
			report.Check(!result.Succeeded(), "reject invalid module fixture: " + name, result.error);
		}

		void CheckSideRejected(Report& report, const std::filesystem::path& root, const std::string& name, const Json& features)
		{
			const auto path = WriteJson(root, name + ".json", features);
			const ly::content::ModuleSideFeatureLoader::Result result = ly::content::ModuleSideFeatureLoader::LoadFromFile(path);
			report.Check(!result.Succeeded(), "reject invalid side-feature fixture: " + name, result.error);
		}
	}

	void RunUncommonContentChecks(Report& report, const std::filesystem::path& fixtureRoot)
	{
		const Json originalModules = ReadJson(DataPath("modules.json"));
		const Json originalFeatures = ReadJson(DataPath("module_side_features.json"));
		const auto invalidJson = WriteRaw(fixtureRoot, "invalid-syntax.json", "{ invalid json");
		report.Check(!ly::content::ModuleLoader::LoadFromFile(invalidJson).Succeeded(), "reject malformed JSON syntax");

		{
			Json modules = originalModules;
			Json& glass = FindModule(modules, "Module.Offense.GlassTopCore.Uncommon");
			glass["sizeVariants"][0].erase("shape");
			CheckModuleRejected(report, fixtureRoot, "missing-shape", modules);
		}
		{
			Json modules = originalModules;
			FindModule(modules, "Module.Utility.DoubleFeedCore.Uncommon")["sideFeatures"] = { "Module.SideFeature.Shared.O1" };
			CheckModuleRejected(report, fixtureRoot, "one-size-side-feature", modules);
		}
		{
			Json modules = originalModules;
			modules["modules"].push_back(modules["modules"].back());
			CheckModuleRejected(report, fixtureRoot, "duplicate-module-id", modules);
		}
		for (const auto& invalidLevel : std::vector<std::pair<std::string, Json>>{
			{ "level-count-array", Json::array({ 3, 4 }) }, { "level-count-fraction", 2.5 },
			{ "level-count-overflow", 4294967297ull } })
		{
			Json modules = originalModules;
			FindModule(modules, "Module.Offense.GlassTopCore.Uncommon")["levelCount"] = invalidLevel.second;
			CheckModuleRejected(report, fixtureRoot, invalidLevel.first, modules);
		}
		{
			Json modules = originalModules;
			FindModule(modules, "Module.Utility.DoubleFeedCore.Uncommon")["sizeVariants"][0]["attributeModifiers"][0]["magnitude"] = { 15.0, 18.0 };
			CheckModuleRejected(report, fixtureRoot, "short-main-level-table", modules);
		}
		{
			Json modules = originalModules;
			FindModule(modules, "Module.Utility.DoubleFeedCore.Uncommon")["sizeVariants"][0]["zoneEffects"] = Json::array({ Json{
				{ "zone", "Module.Zone.Unknown" }, { "scope", "FamilyOfZone" }, { "multiplier", 0.9 }
			} });
			CheckModuleRejected(report, fixtureRoot, "unknown-main-zone", modules);
		}
		{
			Json modules = originalModules;
			FindModule(modules, "Module.Utility.DoubleFeedCore.Uncommon")["sizeVariants"][0]["zoneEffects"] = Json::array({ Json{
				{ "zone", "Module.Zone.Hull" }, { "scope", "FamilyOfZone" }, { "multiplier", 0.9 }
			} });
			CheckModuleRejected(report, fixtureRoot, "main-family-zone-on-any-family-zone", modules);
		}

		{
			Json features = originalFeatures;
			FindFeature(features, "Module.SideFeature.Shared.O2")["rules"]["attributeModifiers"][0].erase("magnitude");
			CheckSideRejected(report, fixtureRoot, "missing-numeric-value", features);
		}
		{
			Json features = originalFeatures;
			FindFeature(features, "Module.SideFeature.Shared.D5")["rules"]["attributeModifiers"][0]["magnitude"] = { 1.08, 1.08 };
			CheckSideRejected(report, fixtureRoot, "short-unused-feature-table", features);
		}
		{
			Json features = originalFeatures;
			FindFeature(features, "Module.SideFeature.Shared.O2")["levelCount"] = Json::array({ 3, 3 });
			CheckSideRejected(report, fixtureRoot, "side-level-count-array", features);
		}
		{
			Json features = originalFeatures;
			FindFeature(features, "Module.SideFeature.Shared.O2")["levelCount"] = 2.5;
			CheckSideRejected(report, fixtureRoot, "side-level-count-fraction", features);
		}
		{
			Json features = originalFeatures;
			features["schemaVersion"] = 4294967297ull;
			CheckSideRejected(report, fixtureRoot, "side-schema-version-overflow", features);
		}
		{
			Json features = originalFeatures;
			FindFeature(features, "Module.SideFeature.Shared.O2")["rules"]["zoneEffects"] = Json::array({ Json{
				{ "zone", "Module.Zone.Unknown" }, { "scope", "FamilyOfZone" }, { "multiplier", 0.9 }
			} });
			const auto modulesPath = WriteJson(fixtureRoot, "unknown-side-zone-modules.json", originalModules);
			const auto featuresPath = WriteJson(fixtureRoot, "unknown-side-zone-features.json", features);
			std::string failure;
			report.Check(!ly::content::ModuleContentCatalog::LoadFromFiles(modulesPath, featuresPath, &failure),
				"reject unknown side-feature zone against the module zone catalog", failure);
		}
		{
			Json features = originalFeatures;
			FindFeature(features, "Module.SideFeature.Shared.O2")["rules"]["zoneEffects"] = Json::array({ Json{
				{ "zone", "Module.Zone.Hull" }, { "scope", "FamilyOfZone" }, { "multiplier", 0.9 }
			} });
			const auto modulesPath = WriteJson(fixtureRoot, "any-family-modules.json", originalModules);
			const auto featuresPath = WriteJson(fixtureRoot, "any-family-features.json", features);
			std::string failure;
			report.Check(!ly::content::ModuleContentCatalog::LoadFromFiles(modulesPath, featuresPath, &failure),
				"reject FamilyOfZone side effects on an any-family zone", failure);
		}
		{
			Json features = originalFeatures;
			features["sharedPools"][0]["choices"][0]["weight"] = -1.0;
			CheckSideRejected(report, fixtureRoot, "invalid-weight", features);
		}
		{
			Json features = originalFeatures;
			features["sharedPools"][0]["choices"][0]["sideFeatureId"] = "Module.SideFeature.Missing.Unknown";
			const auto modulesPath = WriteJson(fixtureRoot, "unknown-pool-modules.json", originalModules);
			const auto featuresPath = WriteJson(fixtureRoot, "unknown-pool-feature.json", features);
			std::string failure;
			report.Check(!ly::content::ModuleContentCatalog::LoadFromFiles(modulesPath, featuresPath, &failure),
				"reject unknown pool feature reference", failure);
		}
		{
			Json modules = originalModules;
			FindModule(modules, "Module.Offense.GlassTopCore.Uncommon")["sharedPools"][0]["allowedFeatureIds"] =
				Json::array({ "Module.SideFeature.Shared.D1" });
			const auto modulesPath = WriteJson(fixtureRoot, "pool-membership-modules.json", modules);
			const auto featuresPath = WriteJson(fixtureRoot, "pool-membership-features.json", originalFeatures);
			std::string failure;
			report.Check(!ly::content::ModuleContentCatalog::LoadFromFiles(modulesPath, featuresPath, &failure),
				"reject a module feature outside its declared pool", failure);
		}
		for (const auto& invalidShotCount : std::vector<std::pair<std::string, Json>>{
			{ "opening-shot-fraction", 1.5 }, { "opening-shot-overflow", 4294967297ull } })
		{
			Json features = originalFeatures;
			Json& ruleFeature = FindFeature(features, "Module.SideFeature.Shared.O2");
			ruleFeature["rules"]["damageRules"] = Json::array({ Json{
				{ "condition", "FullMagazineShot" }, { "openingShotCount", invalidShotCount.second }, { "damageMultiplier", 1.1 }
			} });
			CheckSideRejected(report, fixtureRoot, invalidShotCount.first, features);
		}
		{
			Json features = originalFeatures;
			FindFeature(features, "Module.SideFeature.Shared.O2")["rules"] = Json::object();
			CheckSideRejected(report, fixtureRoot, "empty-side-rule-package", features);
		}
		{
			Json features = originalFeatures;
			FindFeature(features, "Module.SideFeature.Shared.O2")["rules"]["triggerBuffs"] = Json::array({ Json{
				{ "trigger", "AbilityActivated" }, { "durationSeconds", Json::array({ 2.0, 2.0, 0.0 }) },
				{ "movementSpeedMultiplier", 1.1 }, { "modifiers", Json::array() }
			} });
			CheckSideRejected(report, fixtureRoot, "next-level-negative-duration", features);
		}
		{
			Json features = originalFeatures;
			FindFeature(features, "Module.SideFeature.Shared.O2")["rules"]["damageRules"] = Json::array({ Json{
				{ "condition", "FullMagazineShot" }, { "openingShotCount", 3 }, { "damageMultiplier", 1.1 },
				{ "bonusDamageFraction", Json::array({ 0.0, 0.0, 0.1 }) },
				{ "bonusDamageType", "Damage.Type.Kinetic" }, { "bonusCriticalPolicy", "Disabled" }
			} });
			CheckSideRejected(report, fixtureRoot, "next-level-fire-bonus-fraction", features);
		}
		{
			Json features = originalFeatures;
			FindFeature(features, "Module.SideFeature.Shared.O2")["rules"]["damageRules"] = Json::array({ Json{
				{ "condition", "TargetHealthAtMost" }, { "conditionValue", Json::array({ 0.5, 0.5, 1.5 }) },
				{ "damageMultiplier", 1.2 }
			} });
			CheckSideRejected(report, fixtureRoot, "next-level-health-condition-range", features);
		}

		const ly::content::ModuleLoader::Result validModules = ly::content::ModuleLoader::LoadFromFile(DataPath("modules.json"));
		const ly::content::ModuleSideFeatureLoader::Result validFeatures = ly::content::ModuleSideFeatureLoader::LoadFromFile(DataPath("module_side_features.json"));
		report.Check(validModules.Succeeded() && validFeatures.Succeeded(), "load shipped Uncommon module and side-feature JSON");
		const ly::ModuleDefinition* glass = nullptr;
		for (const ly::ModuleDefinition& definition : validModules.definitions)
			if (definition.moduleId == "Module.Offense.GlassTopCore.Uncommon") glass = &definition;
		report.Check(glass && glass->sizeVariants.size() == 2 && glass->sizeLadderCellCounts == ly::List<int>{ 1, 3 },
			"Glass Top catalog keeps only its approved one-to-three-cell ladder");
		const ly::ModuleSideFeatureDefinition* o1 = nullptr;
		for (const ly::ModuleSideFeatureDefinition& feature : validFeatures.definitions)
			if (feature.sideFeatureId == "Module.SideFeature.Shared.O1") o1 = &feature;
		report.Check(o1 && o1->rules.attributeModifiers.front().magnitudeByLevel.values == ly::List<float>{ 8.f, 8.f, 8.f },
			"unapproved side-feature growth remains three explicit constant values");
	}
}
