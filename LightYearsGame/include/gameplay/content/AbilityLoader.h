#pragma once
#include <nlohmann/json.hpp>

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameConfigs/ability/AbilityActorStructs.h"

#include <filesystem>
#include <map>
#include <string>

namespace ly::content
{
	class AbilityLoader
	{
	public:
		struct LoadedDefinition
		{
			std::string id;
			GameAbilityDefinition definition;
			std::map<std::string, float> numericSettings;
			List<AbilityActorDefinition> actorDefinitions;
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

		static Result LoadFromJson(const nlohmann::json& root, const List<const GameAbilityDefinition*>& fallbackDefinitions, const List<const AbilityActorDefinition*>& fallbackActorDefinitions, const std::string& selectedId = {});

		static Result LoadFromFile(
			const std::filesystem::path& filePath,
			const List<const GameAbilityDefinition*>& fallbackDefinitions,
			const List<const AbilityActorDefinition*>& fallbackActorDefinitions
		);
	};
}
