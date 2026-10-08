#pragma once

#include "gameplay/module/ModuleDefinition.h"

#include <filesystem>
#include <nlohmann/json.hpp>
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
		static ModuleRulePackage ParseRulePackage(const nlohmann::json& object, const std::string& context, int minimumLevelCount, bool defaultAffectedByZoneBonus);
		static bool ValidateRulePackageLevelCount(const ModuleRulePackage& rules, int levelCount, const std::string& context, std::string* failureReason = nullptr);
		static bool ValidateRulePackageZoneEffects(const ModuleRulePackage& rules, const ModuleZoneTable& zoneTable, const std::string& context, std::string* failureReason = nullptr);
	};
}
