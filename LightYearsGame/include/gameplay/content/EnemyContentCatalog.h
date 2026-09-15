#pragma once

#include "framework/Core.h"
#include "gameplay/enemy/EnemyBehaviorProfile.h"
#include "gameplay/enemy/EnemyDefinition.h"

#include <filesystem>
#include <string>

namespace ly
{
	struct EnemyCombatProfile;
}

namespace ly::content
{
	class EnemyContentCatalog final
	{
	public:
		static bool LoadFromFiles(
			const std::filesystem::path& enemyDefinitionsPath,
			const std::filesystem::path& behaviorProfilesPath,
			std::string* failureReason = nullptr
		);

		static const EnemyDefinition* FindById(const std::string& enemyId);
		static const EnemyBehaviorProfile* FindBehaviorById(const std::string& profileId);
		static const List<EnemyDefinition>& GetDefinitions();
		static const List<EnemyBehaviorProfile>& GetBehaviorProfiles();
		static bool IsLoaded() noexcept;
		static void Clear();
	};
}
