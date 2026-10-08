#include "gameplay/content/ModuleContentCatalog.h"

#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"

#include <filesystem>
#include <set>
#include <stdexcept>
#include <utility>

namespace ly::content
{
	namespace
	{
		List<ModuleDefinition>& MutableDefinitions()
		{
			static List<ModuleDefinition> definitions;
			return definitions;
		}

		ModuleZoneTable& MutableZoneTable()
		{
			static ModuleZoneTable zoneTable;
			return zoneTable;
		}

		bool& GetLoadedState()
		{
			static bool loaded = false;
			return loaded;
		}
	}

	bool ModuleContentCatalog::LoadFromFile(
		const std::filesystem::path& filePath,
		std::string* failureReason)
	{
		ModuleLoader::Result loaded = ModuleLoader::LoadFromFile(filePath);
		if (!loaded.Succeeded())
		{
			if (failureReason)
			{
				*failureReason = loaded.error;
			}
			return false;
		}
		const std::filesystem::path sideFeaturesPath = filePath.parent_path() / "module_side_features.json";
		if (std::filesystem::exists(sideFeaturesPath)) return LoadFromFiles(filePath, sideFeaturesPath, failureReason);
		for (const ModuleDefinition& definition : loaded.definitions)
			if (!definition.sideFeatures.empty() || !definition.sharedPools.empty())
			{
				if (failureReason) *failureReason = "Module definitions require module_side_features.json";
				return false;
			}
		MutableDefinitions() = std::move(loaded.definitions);
		MutableZoneTable() = std::move(loaded.zoneTable);
		ModuleSideFeatureCatalog::Commit({}, {});
		GetLoadedState() = true;
		return true;
	}

	bool ModuleContentCatalog::LoadFromFiles(const std::filesystem::path& modulesFilePath, const std::filesystem::path& sideFeaturesFilePath, std::string* failureReason)
	{
		ModuleLoader::Result modules = ModuleLoader::LoadFromFile(modulesFilePath);
		if (!modules.Succeeded())
		{
			if (failureReason) *failureReason = modules.error;
			return false;
		}
		ModuleSideFeatureLoader::Result sideFeatures = ModuleSideFeatureLoader::LoadFromFile(sideFeaturesFilePath);
		if (!sideFeatures.Succeeded())
		{
			if (failureReason) *failureReason = sideFeatures.error;
			return false;
		}

		const auto findFeature = [&sideFeatures](const sas::ContentId& id) -> const ModuleSideFeatureDefinition*
		{
			for (const ModuleSideFeatureDefinition& feature : sideFeatures.definitions)
				if (feature.sideFeatureId == id) return &feature;
			return nullptr;
		};
		const auto findPool = [&sideFeatures](const sas::ContentId& id) -> const ModuleSideFeaturePool*
		{
			for (const ModuleSideFeaturePool& pool : sideFeatures.pools)
				if (pool.poolId == id) return &pool;
			return nullptr;
		};
		const auto poolContains = [](const ModuleSideFeaturePool& pool, const sas::ContentId& id)
		{
			for (const ModuleSideFeatureChoice& choice : pool.choices)
				if (choice.sideFeatureId == id) return true;
			return false;
		};
		try
		{
			for (const ModuleSideFeatureDefinition& feature : sideFeatures.definitions)
			{
				std::string failure;
				if (!ModuleLoader::ValidateRulePackageZoneEffects(feature.rules, modules.zoneTable,
					"Side feature " + feature.sideFeatureId.ToString(), &failure)) throw std::runtime_error(failure);
			}
			for (const ModuleSideFeaturePool& pool : sideFeatures.pools)
				for (const ModuleSideFeatureChoice& choice : pool.choices)
				{
					const ModuleSideFeatureDefinition* feature = findFeature(choice.sideFeatureId);
					if (!feature) throw std::runtime_error("Pool " + pool.poolId.ToString() + " references unknown side feature " + choice.sideFeatureId.ToString());
					if (feature->poolId != pool.poolId)
						throw std::runtime_error("Side feature " + feature->sideFeatureId.ToString() + " has a mismatched poolId");
				}
			for (const ModuleSideFeatureDefinition& feature : sideFeatures.definitions)
			{
				if (feature.poolId.IsValid())
				{
					const ModuleSideFeaturePool* pool = findPool(feature.poolId);
					if (!pool || !poolContains(*pool, feature.sideFeatureId))
						throw std::runtime_error("Side feature " + feature.sideFeatureId.ToString() + " is missing from its declared pool");
				}
			}
			// Delayed and kill explosions keep one rule per module instance, so a side feature may not add a second one.
			const auto rejectSingletonOverlap = [](const ModuleDefinition& module, const ModuleSideFeatureDefinition& feature)
			{
				const ModuleGeneratedRules& side = feature.rules.generatedRules;
				for (const ModuleSizeVariant& variant : module.sizeVariants)
					if ((!side.delayedExplosions.empty() && !variant.generatedRules.delayedExplosions.empty()) || (!side.killExplosions.empty() && !variant.generatedRules.killExplosions.empty()))
						throw std::runtime_error("Side feature " + feature.sideFeatureId.ToString() + " duplicates a single-instance generated rule already defined by module " + module.moduleId.ToString());
			};
			for (const ModuleDefinition& module : modules.definitions)
			{
				for (const sas::ContentId& featureId : module.sideFeatures)
				{
					const ModuleSideFeatureDefinition* feature = findFeature(featureId);
					if (!feature) throw std::runtime_error("Module " + module.moduleId.ToString() + " references unknown side feature " + featureId.ToString());
					if (feature->poolId.IsValid()) throw std::runtime_error("Private side feature " + featureId.ToString() + " must not declare poolId");
					std::string failure;
					if (!ModuleLoader::ValidateRulePackageLevelCount(feature->rules, module.levelCount,
						"Side feature " + featureId.ToString(), &failure)) throw std::runtime_error(failure);
					if (feature->levelCount < module.levelCount) throw std::runtime_error("Side feature " + featureId.ToString() + " has fewer levels than module " + module.moduleId.ToString());
					rejectSingletonOverlap(module, *feature);
				}
				for (const ModuleSharedPoolPermission& permission : module.sharedPools)
				{
					const ModuleSideFeaturePool* pool = findPool(permission.poolId);
					if (!pool) throw std::runtime_error("Module " + module.moduleId.ToString() + " references unknown pool " + permission.poolId.ToString());
					for (const sas::ContentId& featureId : permission.allowedFeatureIds)
					{
						const ModuleSideFeatureDefinition* feature = findFeature(featureId);
						if (!feature || feature->poolId != permission.poolId || !poolContains(*pool, featureId))
							throw std::runtime_error("Module " + module.moduleId.ToString() + " has invalid pool membership for " + featureId.ToString());
						std::string failure;
						if (!ModuleLoader::ValidateRulePackageLevelCount(feature->rules, module.levelCount,
							"Side feature " + featureId.ToString(), &failure)) throw std::runtime_error(failure);
						if (feature->levelCount < module.levelCount) throw std::runtime_error("Side feature " + featureId.ToString() + " has fewer levels than module " + module.moduleId.ToString());
						rejectSingletonOverlap(module, *feature);
					}
				}
			}
		}
		catch (const std::exception& exception)
		{
			if (failureReason) *failureReason = "Failed to validate module side-feature references: " + std::string{ exception.what() };
			return false;
		}

		MutableDefinitions() = std::move(modules.definitions);
		MutableZoneTable() = std::move(modules.zoneTable);
		ModuleSideFeatureCatalog::Commit(std::move(sideFeatures.definitions), std::move(sideFeatures.pools));
		GetLoadedState() = true;
		return true;
	}

	const ModuleDefinition* ModuleContentCatalog::FindById(const sas::ContentId& moduleId)
	{
		for (const ModuleDefinition& definition : MutableDefinitions())
		{
			if (definition.moduleId == moduleId)
			{
				return &definition;
			}
		}
		return nullptr;
	}

	const List<ModuleDefinition>& ModuleContentCatalog::GetDefinitions()
	{
		return MutableDefinitions();
	}

	const ModuleZoneTable& ModuleContentCatalog::GetZoneTable()
	{
		return MutableZoneTable();
	}

	bool ModuleContentCatalog::IsLoaded() noexcept
	{
		return GetLoadedState();
	}
}
