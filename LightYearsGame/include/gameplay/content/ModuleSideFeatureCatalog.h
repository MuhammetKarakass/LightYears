#pragma once

#include "gameplay/module/ModuleSideFeature.h"

#include <filesystem>
#include <string>

namespace ly::content
{
	class ModuleContentCatalog;

	class ModuleSideFeatureCatalog
	{
	public:
		static const ModuleSideFeatureDefinition* FindById(const sas::ContentId& sideFeatureId);
		static const ModuleSideFeaturePool* FindPool(const sas::ContentId& poolId);
		static const List<ModuleSideFeatureDefinition>& GetDefinitions();
		static const List<ModuleSideFeaturePool>& GetPools();
		static bool IsLoaded() noexcept;

	private:
		static void Commit(List<ModuleSideFeatureDefinition> definitions, List<ModuleSideFeaturePool> pools);
		friend class ModuleContentCatalog;
	};
}
