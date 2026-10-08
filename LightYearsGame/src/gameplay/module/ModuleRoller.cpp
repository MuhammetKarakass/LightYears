#include "gameplay/module/ModuleRoller.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace ModuleRoller
	{
		namespace
		{
			struct WeightedFeature
			{
				sas::ContentId id;
				double weight = 0.0;
			};

			std::uint64_t Next(std::uint64_t& seed)
			{
				seed += 0x9e3779b97f4a7c15ull;
				std::uint64_t value = seed;
				value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ull;
				value = (value ^ (value >> 27)) * 0x94d049bb133111ebull;
				return value ^ (value >> 31);
			}

			bool Contains(const List<sas::ContentId>& ids, const sas::ContentId& id)
			{
				return std::find(ids.begin(), ids.end(), id) != ids.end();
			}

			void AddCandidate(List<WeightedFeature>& candidates, const sas::ContentId& id, double weight)
			{
				if (!id.IsValid() || !std::isfinite(weight) || weight <= 0.0) return;
				if (std::find_if(candidates.begin(), candidates.end(), [&id](const WeightedFeature& candidate) { return candidate.id == id; }) != candidates.end()) return;
				candidates.push_back({ id, weight });
			}
		}

		std::optional<sas::ContentId> RollSideFeatureId(const ModuleDefinition& module, const List<ModuleSideFeaturePool>& pools, const ModuleSideFeatureLookup& lookup, std::uint64_t& seed)
		{
			if (!lookup || module.sizeVariants.size() <= 1) return std::nullopt;
			List<WeightedFeature> candidates;
			for (const sas::ContentId& featureId : module.sideFeatures)
			{
				const ModuleSideFeatureDefinition* feature = lookup(featureId);
				if (feature && !feature->poolId.IsValid() && IsSideFeatureUsableAtLevel(module, feature, module.levelCount))
					AddCandidate(candidates, featureId, feature->rollWeight);
			}
			for (const ModuleSharedPoolPermission& permission : module.sharedPools)
			{
				const auto pool = std::find_if(pools.begin(), pools.end(), [&permission](const ModuleSideFeaturePool& entry)
					{ return entry.poolId == permission.poolId; });
				if (pool == pools.end()) continue;
				for (const ModuleSideFeatureChoice& choice : pool->choices)
				{
					if (!Contains(permission.allowedFeatureIds, choice.sideFeatureId)) continue;
					const ModuleSideFeatureDefinition* feature = lookup(choice.sideFeatureId);
					if (!feature || feature->poolId != permission.poolId ||
					!IsSideFeatureUsableAtLevel(module, feature, module.levelCount)) continue;
					AddCandidate(candidates, choice.sideFeatureId, choice.weight);
				}
			}
			if (candidates.empty()) return std::nullopt;

			const double maximumWeight = std::max_element(candidates.begin(), candidates.end(),
				[](const WeightedFeature& left, const WeightedFeature& right) { return left.weight < right.weight; })->weight;
			double totalWeight = 0.0;
			for (const WeightedFeature& candidate : candidates) totalWeight += candidate.weight / maximumWeight;
			if (!std::isfinite(totalWeight) || totalWeight <= 0.0) return std::nullopt;

			const double unit = static_cast<double>(Next(seed) >> 11) * (1.0 / 9007199254740992.0);
			double cursor = unit * totalWeight;
			for (const WeightedFeature& candidate : candidates)
			{
				cursor -= candidate.weight / maximumWeight;
				if (cursor < 0.0) return candidate.id;
			}
			return candidates.back().id;
		}
	}
}
