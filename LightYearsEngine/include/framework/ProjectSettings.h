#pragma once

#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

namespace ly
{
	struct ProjectSettings
	{
		std::string name = "LightYears";
		std::string startupScene;
		unsigned int windowWidth = 1920;
		unsigned int windowHeight = 1080;
		// Invalid files throw with a diagnostic; absence is handled by the caller.
		static ProjectSettings Load(const std::filesystem::path& path);
		static ProjectSettings Parse(const nlohmann::json& document);
		nlohmann::json ToJson() const;
		std::string ExecutableName() const;
	};
}
