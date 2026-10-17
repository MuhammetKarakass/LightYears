#pragma once

#include "gameConfigs/combat/DamageTypeConfig.h"

#include <filesystem>
#include <string>

namespace vega::content
{
	class DamageStatusBalanceCatalog
	{
	public:
		static bool LoadFromFile(
			const std::filesystem::path& filePath,
			std::string* failureReason = nullptr
		);

		static const DamageStatusBalance& Get();
		static bool IsLoaded() noexcept;
	};
}
