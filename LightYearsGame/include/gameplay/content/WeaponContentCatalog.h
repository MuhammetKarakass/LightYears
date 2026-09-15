#pragma once

#include "gameConfigs/combat/WeaponStructs.h"

#include <filesystem>
#include <optional>
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
