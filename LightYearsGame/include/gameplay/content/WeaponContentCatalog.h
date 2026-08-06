#pragma once

#include "gameConfigs/combat/WeaponStructs.h"

#include <filesystem>
#include <string>

namespace ly::content
{
	class WeaponContentCatalog
	{
	public:
		static bool LoadFromFile(
			const std::filesystem::path& filePath,
			std::string* failureReason = nullptr
		);

		static const PrimaryWeaponDefinition* FindById(
			const std::string& weaponId
		);

		static bool IsLoaded() noexcept;
	};
}
