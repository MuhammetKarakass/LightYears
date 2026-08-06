#pragma once

#include "effects/GameplayEffectDefinition.h"

#include <filesystem>
#include <string>

namespace ly::content
{
	class EffectLoader
	{
	public:
		struct LoadedDefinition
		{
			std::string id;
			sas::GameplayEffectDefinition definition;
		};

		struct Result
		{
			List<LoadedDefinition> definitions;
			std::string error;

			bool Succeeded() const noexcept
			{
				return error.empty();
			}
		};

		static Result LoadFromFile(
			const std::filesystem::path& filePath,
			const List<const sas::GameplayEffectDefinition*>& fallbackDefinitions
		);
	};
}
