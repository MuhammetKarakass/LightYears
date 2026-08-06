#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameConfigs/ability/AbilityActorStructs.h"

#include <filesystem>
#include <optional>
#include <string>

namespace ly::content
{
	class AbilityContentCatalog
	{
	public:
		static bool LoadFromFile(
			const std::filesystem::path& filePath,
			const List<const GameAbilityDefinition*>& fallbackDefinitions,
			const List<const AbilityActorDefinition*>& fallbackActorDefinitions,
			std::string* failureReason = nullptr
		);

		static const GameAbilityDefinition* FindById(const std::string& abilityId);
		static const AbilityActorDefinition* FindActorById(const std::string& actorDefinitionId);
		static const List<const GameAbilityDefinition*>& GetDefinitions();
		static std::optional<float> FindNumericSetting(
			const std::string& abilityId,
			const std::string& settingName
		);
		static bool IsLoaded() noexcept;
	};
}
