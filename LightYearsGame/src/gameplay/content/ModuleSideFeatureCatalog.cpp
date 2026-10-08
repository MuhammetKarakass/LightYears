#include "gameplay/content/ModuleSideFeatureCatalog.h"

#include <utility>

namespace ly::content
{
	namespace
	{
		List<ModuleSideFeatureDefinition>& MutableDefinitions()
		{
			static List<ModuleSideFeatureDefinition> definitions;
			return definitions;
		}

		List<ModuleSideFeaturePool>& MutablePools()
		{
			static List<ModuleSideFeaturePool> pools;
			return pools;
		}

		bool& GetLoadedState()
		{
			static bool loaded = false;
			return loaded;
		}
	}

	const ModuleSideFeatureDefinition* ModuleSideFeatureCatalog::FindById(const sas::ContentId& sideFeatureId)
	{
		for (const ModuleSideFeatureDefinition& definition : MutableDefinitions())
			if (definition.sideFeatureId == sideFeatureId) return &definition;
		return nullptr;
	}

	const ModuleSideFeaturePool* ModuleSideFeatureCatalog::FindPool(const sas::ContentId& poolId)
	{
		for (const ModuleSideFeaturePool& pool : MutablePools())
			if (pool.poolId == poolId) return &pool;
		return nullptr;
	}

	const List<ModuleSideFeatureDefinition>& ModuleSideFeatureCatalog::GetDefinitions() { return MutableDefinitions(); }
	const List<ModuleSideFeaturePool>& ModuleSideFeatureCatalog::GetPools() { return MutablePools(); }
	bool ModuleSideFeatureCatalog::IsLoaded() noexcept { return GetLoadedState(); }

	void ModuleSideFeatureCatalog::Commit(List<ModuleSideFeatureDefinition> definitions, List<ModuleSideFeaturePool> pools)
	{
		MutableDefinitions() = std::move(definitions);
		MutablePools() = std::move(pools);
		GetLoadedState() = true;
	}
}
