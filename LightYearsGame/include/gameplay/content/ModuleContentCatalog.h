#pragma once

#include "gameplay/module/ModuleDefinition.h"

#include <filesystem>
#include <string>

namespace ly::content
{
	class ModuleContentCatalog
	{
	public:
		static bool LoadFromFile(
			const std::filesystem::path& filePath,
			std::string* failureReason = nullptr
		);

		static const ModuleDefinition* FindById(const sas::ContentId& moduleId);
		static const List<ModuleDefinition>& GetDefinitions();
		static const ModuleZoneTable& GetZoneTable();
		static bool IsLoaded() noexcept;
	};
}
