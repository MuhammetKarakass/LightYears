#pragma once

#include "gameConfigs/ship/ShipStructs.h"

#include <filesystem>
#include <string>

namespace ly::content
{
	class ShipContentCatalog
	{
	public:
		static bool LoadFromFile(
			const std::filesystem::path& filePath,
			std::string* failureReason = nullptr
		);

		static const ShipDefinition* FindById(const std::string& shipId);
		static const ShipDefinition& GetPlayerFighterDefinition();
		static bool IsLoaded() noexcept;
	};
}
