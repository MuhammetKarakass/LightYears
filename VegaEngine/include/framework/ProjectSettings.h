#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace vega
{
	class EngineContext;
	struct CollisionLayerSetting
	{
		int bit = 0;
		std::string name;
		std::vector<std::string> aliases;
	};

	struct ProjectSettings
	{
		std::string name = "Game";
		std::string startupScene;
		unsigned int windowWidth = 1920;
		unsigned int windowHeight = 1080;
		// Editor input paths are relative to the project file, never to the process cwd.
		nlohmann::json editor = nlohmann::json::object();
		unsigned int AutosaveSeconds() const { return editor.value("autosaveSeconds", 120u); }
		std::string runtimeExecutable = "Game.exe";
		std::string runtimeAssetDirectory = "assets";
		std::string runtimeVisualDirectory = "visual";
		std::string runtimeLog = "Game.log";
		std::vector<CollisionLayerSetting> collisionLayers;
		// Applies only to the owning registry, before constructing scene objects.
		void ApplyCollisionLayers(EngineContext& context) const;
		// Invalid files throw with a diagnostic; absence is handled by the caller.
		static ProjectSettings Load(const std::filesystem::path& path);
		static ProjectSettings Parse(const nlohmann::json& document);
		nlohmann::json ToJson() const;
		std::string ExecutableName() const;
	};
}
