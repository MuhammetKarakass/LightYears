#include "UncommonModuleE2E.h"

#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/module/ModuleRoller.h"

#include <set>

namespace uncommon_module_e2e
{
	void RunUncommonRollerChecks(Report& report)
	{
		const vega::ModuleDefinition* glass = vega::content::ModuleContentCatalog::FindById("Module.Offense.GlassTopCore.Uncommon");
		const auto lookup = [](const sas::ContentId& id) { return vega::content::ModuleSideFeatureCatalog::FindById(id); };
		const vega::List<vega::ModuleSideFeaturePool>& pools = vega::content::ModuleSideFeatureCatalog::GetPools();
		report.Check(glass != nullptr, "roller receives the shipped Glass Top definition");
		if (!glass) return;
		std::uint64_t firstSeed = 0x123456789abcdef0ull;
		std::uint64_t repeatedSeed = firstSeed;
		const auto first = vega::ModuleRoller::RollSideFeatureId(*glass, pools, lookup, firstSeed);
		const auto repeated = vega::ModuleRoller::RollSideFeatureId(*glass, pools, lookup, repeatedSeed);
		report.Check(first && repeated && *first == *repeated && firstSeed == repeatedSeed, "same seed repeats the same eligible side feature");

		std::set<std::string> observed;
		const std::set<std::string> expected{ "Module.SideFeature.Shared.O2", "Module.SideFeature.Shared.O4", "Module.SideFeature.Shared.O5", "Module.SideFeature.GlassTop.ShieldReturn", "Module.SideFeature.GlassTop.LowHealthCrit" };
		bool allEligible = true;
		for (std::uint64_t seed = 0; seed < 512; ++seed)
		{
			std::uint64_t state = seed;
			const auto rolled = vega::ModuleRoller::RollSideFeatureId(*glass, pools, lookup, state);
			if (!rolled) { allEligible = false; continue; }
			const std::string id = rolled->ToString();
			observed.insert(id);
			allEligible = allEligible && expected.count(id) != 0;
		}
		report.Check(allEligible && observed == expected, "roller includes all five legal Glass Top choices and excludes non-permitted pool members",
			Json{ { "observed", observed }, { "eligibleCount", expected.size() } });

		vega::List<vega::ModuleSideFeaturePool> weightedPools = pools;
		for (vega::ModuleSideFeaturePool& pool : weightedPools)
			if (pool.poolId.ToString() == "Module.SidePool.Offense")
				for (vega::ModuleSideFeatureChoice& choice : pool.choices)
					if (choice.sideFeatureId.ToString() == "Module.SideFeature.Shared.O2") choice.weight = 100.0;
		int dominantO2Count = 0;
		for (std::uint64_t seed = 0; seed < 512; ++seed)
		{
			std::uint64_t state = seed;
			const auto rolled = vega::ModuleRoller::RollSideFeatureId(*glass, weightedPools, lookup, state);
			if (rolled && rolled->ToString() == "Module.SideFeature.Shared.O2") ++dominantO2Count;
		}
	report.Check(dominantO2Count > 450, "configured pool weight changes the deterministic weighted selection distribution",
			Json{ { "o2Weight", 100.0 }, { "o2RollsOf512", dominantO2Count } });

		vega::ModuleDefinition oneFeature = *glass;
		oneFeature.sharedPools.clear();
		oneFeature.sideFeatures = { sas::ContentId{ "Module.SideFeature.GlassTop.ShieldReturn" } };
		std::uint64_t singleSeed = 7;
		const auto single = vega::ModuleRoller::RollSideFeatureId(oneFeature, pools, lookup, singleSeed);
		report.Check(single && single->ToString() == "Module.SideFeature.GlassTop.ShieldReturn", "single eligible weighted choice is safe");

		vega::ModuleDefinition empty;
		std::uint64_t emptySeed = 7;
		report.Check(!vega::ModuleRoller::RollSideFeatureId(empty, pools, lookup, emptySeed), "empty side-feature choices are safe");
	}
}
