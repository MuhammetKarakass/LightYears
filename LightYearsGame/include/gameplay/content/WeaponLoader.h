#pragma once

#include "gameConfigs/combat/WeaponStructs.h"

#include <filesystem>
#include <string>

namespace ly::content
{
	class WeaponLoader
	{
	public:
		struct Result
		{
			List<PrimaryWeaponDefinition> definitions;
			std::string error;

			bool Succeeded() const noexcept
			{
				return error.empty();
			}
		};

		static Result LoadFromFile(
			const std::filesystem::path& filePath
		);
	};
}
