#pragma once

#include "effects/GameplayEffectDefinition.h"

#include <filesystem>
#include <string>

namespace ly::content
{
	class EffectContentCatalog
	{
	public:
		static bool LoadFromFile(
			const std::filesystem::path& filePath,
			const List<const sas::GameplayEffectDefinition*>& fallbackDefinitions,
			std::string* failureReason = nullptr
		);

		static const sas::GameplayEffectDefinition* FindById(
			const std::string& effectId
		);
		static const List<const sas::GameplayEffectDefinition*>& GetDefinitions();
		static bool IsLoaded() noexcept;
	};
}
