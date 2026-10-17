#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/content/RetainedContent.h"

#include <utility>
#include <unordered_map>

namespace vega::content
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

		std::unordered_map<std::string, std::size_t>& MutableIndex()
		{
			static std::unordered_map<std::string, std::size_t> index;
			return index;
		}

		std::unordered_map<std::string, std::size_t>& MutablePoolIndex()
		{
			static std::unordered_map<std::string, std::size_t> index;
			return index;
		}

		bool& GetLoadedState()
		{
			static bool loaded = false;
			return loaded;
		}
	}

	const ModuleSideFeatureDefinition* ModuleSideFeatureCatalog::FindById(const sas::ContentId& sideFeatureId)
	{
		const auto found = MutableIndex().find(sideFeatureId.ToString());
		return found != MutableIndex().end() ? &MutableDefinitions()[found->second] : nullptr;
	}

	const ModuleSideFeaturePool* ModuleSideFeatureCatalog::FindPool(const sas::ContentId& poolId)
	{
		const auto found = MutablePoolIndex().find(poolId.ToString());
		return found != MutablePoolIndex().end() ? &MutablePools()[found->second] : nullptr;
	}

	const List<ModuleSideFeatureDefinition>& ModuleSideFeatureCatalog::GetDefinitions() { return MutableDefinitions(); }
	const List<ModuleSideFeaturePool>& ModuleSideFeatureCatalog::GetPools() { return MutablePools(); }
	bool ModuleSideFeatureCatalog::IsLoaded() noexcept { return GetLoadedState(); }

	void ModuleSideFeatureCatalog::Commit(List<ModuleSideFeatureDefinition> definitions, List<ModuleSideFeaturePool> pools)
	{
		std::unordered_map<std::string, std::size_t> index, poolIndex;
		index.reserve(definitions.size());
		poolIndex.reserve(pools.size());
		for (std::size_t i = 0; i < definitions.size(); ++i) index.emplace(definitions[i].sideFeatureId.ToString(), i);
		for (std::size_t i = 0; i < pools.size(); ++i) poolIndex.emplace(pools[i].poolId.ToString(), i);
		RetainContent(MutableDefinitions());
		MutableDefinitions() = std::move(definitions);
		MutableIndex() = std::move(index);
		MutablePoolIndex() = std::move(poolIndex);
		RetainContent(MutablePools());
		MutablePools() = std::move(pools);
		GetLoadedState() = true;
	}
}
