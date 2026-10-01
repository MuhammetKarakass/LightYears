#pragma once

#include "gameplay/module/ModuleDefinition.h"

#include <filesystem>
#include <string>

namespace ly::content
{
	class ModuleLoader
	{
	public:
		struct Result
		{
			List<ModuleDefinition> definitions;
			ModuleZoneTable zoneTable;
			std::string error;

			bool Succeeded() const noexcept
			{
				return error.empty();
			}
		};

		static Result LoadFromFile(const std::filesystem::path& filePath);
	};
}
