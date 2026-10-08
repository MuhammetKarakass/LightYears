#pragma once

#include "gameplay/module/ModuleDefinition.h"

#include <functional>
#include <string>

namespace ly
{
	struct ModuleSideFeatureDefinition
	{
		sas::ContentId sideFeatureId;
		std::string displayName;
		sas::ContentId poolId;
		double rollWeight = 1.0;
		int levelCount = 3;
		ModuleRulePackage rules;
		std::string description;
	};

	struct ModuleSideFeatureChoice
	{
		sas::ContentId sideFeatureId;
		double weight = 1.0;
	};

	struct ModuleSideFeaturePool
	{
		sas::ContentId poolId;
		List<ModuleSideFeatureChoice> choices;
	};

	using ModuleSideFeatureLookup = std::function<const ModuleSideFeatureDefinition*(const sas::ContentId&)>;

	inline bool IsModuleSideFeatureAllowed(const ModuleDefinition& module,const ModuleSideFeatureDefinition* feature)
	{
		if (!feature) return false;
		if (AllowsPrivateSideFeature(module, feature->sideFeatureId)) return !feature->poolId.IsValid();
		if (!feature->poolId.IsValid()) return false;
		return AllowsSharedSideFeature(module, feature->poolId, feature->sideFeatureId);
	}

	// A side feature needs a size ladder, a permission on the module and rules that cover the given level.
	inline bool IsSideFeatureUsableAtLevel(const ModuleDefinition& module,const ModuleSideFeatureDefinition* feature,int level)
	{
		return module.sizeVariants.size() > 1 && IsModuleSideFeatureAllowed(module, feature) &&
			feature->levelCount >= level && HasModuleRulePackageLevelCoverage(feature->rules, level);
	}
}
