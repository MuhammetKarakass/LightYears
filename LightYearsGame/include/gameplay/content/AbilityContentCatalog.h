#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/content/AbilityLoader.h"

#include <filesystem>
#include <optional>
#include <string>

namespace ly::content
{
	// Existing behavior validators resolve settings through the content catalog.
	// This thread-local, stack-scoped view lets them inspect unsaved candidates
	// without replacing the live catalog or changing behavior validation rules.
	class ScopedAbilityValidationView final
	{
	public:
		explicit ScopedAbilityValidationView(const List<AbilityLoader::LoadedDefinition>& definitions);
		~ScopedAbilityValidationView();
		ScopedAbilityValidationView(const ScopedAbilityValidationView&) = delete;
		ScopedAbilityValidationView& operator=(const ScopedAbilityValidationView&) = delete;
	private:
		const List<AbilityLoader::LoadedDefinition>* mPrevious;
	};

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
