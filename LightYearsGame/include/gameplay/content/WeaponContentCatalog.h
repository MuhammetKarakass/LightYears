#pragma once

#include "gameConfigs/combat/WeaponStructs.h"

#include <filesystem>
#include <optional>
#include <string>

namespace vega::content
{
	class WeaponContentCatalog
	{
	public:
		// Live reload preserves the last valid catalog on failure; startup remains fail-closed.
		static bool LoadFromFile(
			const std::filesystem::path& filePath,
			std::string* failureReason = nullptr,
			bool preserveExisting = false
		);

		static const PrimaryWeaponDefinition* FindById(const std::string& weaponId);
		static std::optional<float> ResolveAuthoredAttributeAtLevel(
			const std::string& weaponId,
			int weaponLevel,
			const sas::AttributeId& attributeId,
			std::string* failureReason = nullptr
		);

		static bool IsLoaded() noexcept;
	};
}
