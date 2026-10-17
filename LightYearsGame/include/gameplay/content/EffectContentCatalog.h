#pragma once

#include "effects/GameplayEffectDefinition.h"

#include <filesystem>
#include <string>

namespace vega::content
{
	class EffectContentCatalog
	{
	public:
		// Live reload preserves the last valid catalog on failure; startup remains fail-closed.
		static bool LoadFromFile(
			const std::filesystem::path& filePath,
			const List<const sas::GameplayEffectDefinition*>& fallbackDefinitions,
			std::string* failureReason = nullptr,
			bool preserveExisting = false
		);

		static const sas::GameplayEffectDefinition* FindById(
			const std::string& effectId
		);
		static const List<const sas::GameplayEffectDefinition*>& GetDefinitions();
		static bool IsLoaded() noexcept;
	};
}
