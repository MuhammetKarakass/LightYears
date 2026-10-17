#pragma once

#include "gameplay/module/ModuleDefinition.h"

#include <filesystem>
#include <cstdint>
#include <string>

namespace vega::content
{
	class ModuleContentCatalog
	{
	public:
		static bool LoadFromFile(
			const std::filesystem::path& filePath,
			std::string* failureReason = nullptr
		);
		static bool LoadFromFiles(const std::filesystem::path& modulesFilePath, const std::filesystem::path& sideFeaturesFilePath, std::string* failureReason = nullptr, bool validateOnly = false);

		static const ModuleDefinition* FindById(const sas::ContentId& moduleId);
		static const List<ModuleDefinition>& GetDefinitions();
		static const ModuleZoneTable& GetZoneTable();
		static std::uint64_t GetRevision() noexcept;
		static bool IsLoaded() noexcept;
	};
}
