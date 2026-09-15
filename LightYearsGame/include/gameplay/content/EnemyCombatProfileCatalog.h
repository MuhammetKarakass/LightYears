#pragma once

#include "gameplay/enemy/EnemyCombatProfile.h"

#include <filesystem>
#include <string>

namespace ly::content
{
	class EnemyCombatProfileCatalog
	{
	public:
		static bool LoadFromFile(
			const std::filesystem::path& filePath,
			std::string* failureReason = nullptr
		);

		static const EnemyCombatProfile* FindById(const std::string& profileId);
		static const List<EnemyCombatProfile>& GetProfiles();
		static bool ValidateProfile(
			const EnemyCombatProfile& profile,
			std::string* failureReason = nullptr
		);
		static bool ValidateShippedProfiles(
			std::string* failureReason = nullptr
		);
		static bool IsLoaded() noexcept;
		static void Clear();
	};
}
