#pragma once
#include <nlohmann/json.hpp>

#include "gameConfigs/ship/ShipStructs.h"

#include <filesystem>
#include <string>

namespace vega::content
{
	class ShipLoader
	{
	public:
		struct LoadedDefinition
		{
			std::string id;
			ShipDefinition definition;
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

		static Result LoadFromJson(const nlohmann::json& root, const ShipDefinition& presentationBase, const std::string& selectedId = {});

		static Result LoadFromFile(
			const std::filesystem::path& filePath,
			const ShipDefinition& presentationBase
		);
	};
}
