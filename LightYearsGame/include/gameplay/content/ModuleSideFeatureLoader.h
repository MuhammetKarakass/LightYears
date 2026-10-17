#pragma once

#include "gameplay/module/ModuleSideFeature.h"

#include <filesystem>
#include <string>

namespace vega::content
{
	class ModuleSideFeatureLoader
	{
	public:
		struct Result
		{
			List<ModuleSideFeatureDefinition> definitions;
			List<ModuleSideFeaturePool> pools;
			std::string error;

			bool Succeeded() const noexcept { return error.empty(); }
		};

		static Result LoadFromFile(const std::filesystem::path& filePath);
	};
}
