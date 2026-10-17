#pragma once

#include "gameConfigs/ship/ShipStructs.h"

#include <filesystem>
#include <string>

namespace vega::content
{
	class ShipContentCatalog
	{
	public:
		// Live reload preserves the last valid catalog on failure; startup remains fail-closed.
		static bool LoadFromFile(
			const std::filesystem::path& filePath,
			std::string* failureReason = nullptr,
			bool preserveExisting = false
		);

		static const ShipDefinition* FindById(const std::string& shipId);
		static List<std::string> GetPlayerShipIds();
		static const ShipDefinition& GetPlayerShipDefinition(const std::string& shipId);
		static const ShipDefinition& GetPlayerFighterDefinition();
		static bool IsLoaded() noexcept;
		static bool IsValidShipDefinition(const ShipDefinition& definition);
	};
}
