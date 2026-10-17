#include "framework/ProjectSettings.h"
#include "framework/EngineContext.h"
#include "framework/reflection/Reflection.h"
#include "framework/debug/Log.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <set>

namespace ly
{
	namespace
	{
		void ValidateCollisionLayers(const std::vector<CollisionLayerSetting>& layers)
		{
			std::set<int> bits;
			std::set<std::string> names;
			const auto addName = [&](const std::string& name)
			{
				if (name.empty() || name == "None" || name.find('|') != std::string::npos ||
					name.find_first_of("\t\r\n") != std::string::npos || name.front() == ' ' || name.back() == ' ' ||
					std::all_of(name.begin(), name.end(), [](unsigned char c) { return std::isdigit(c); }) || !names.insert(name).second)
					throw std::runtime_error("names and aliases must be unique, nonempty labels (not None, numbers or pipe combinations)");
			};
			for (const auto& layer : layers)
			{
				if (layer.bit < 0 || layer.bit >= 8 || !bits.insert(layer.bit).second)
					throw std::runtime_error("bits must be unique integers from 0 to 7");
				addName(layer.name);
				for (const auto& alias : layer.aliases) addName(alias);
			}
			// Neutral spellings always identify their original bit, including unnamed channels.
			for (int bit = 0; bit < 8; ++bit)
			{
				const auto neutral = "Layer " + std::to_string(bit);
				for (const auto& layer : layers)
					if (layer.bit != bit && (layer.name == neutral || std::find(layer.aliases.begin(), layer.aliases.end(), neutral) != layer.aliases.end()))
						throw std::runtime_error("neutral labels cannot identify a different bit");
			}
		}
		void WarnCollisionLayers(const std::exception& error)
		{
			LY_CORE_WARN("Project collisionLayers: %s; using neutral defaults", error.what());
		}
	}

	void ProjectSettings::ApplyCollisionLayers(EngineContext& context) const
	{
		reflect::EnumLabels labels, aliases;
		for (int bit = 0; bit < 8; ++bit) labels.emplace_back(1 << bit, "Layer " + std::to_string(bit));
		try
		{
			ValidateCollisionLayers(collisionLayers);
			for (const auto& layer : collisionLayers)
			{
				if (layer.name != labels[layer.bit].second) aliases.push_back(labels[layer.bit]);
				labels[layer.bit].second = layer.name;
				for (const auto& alias : layer.aliases) aliases.emplace_back(1 << layer.bit, alias);
			}
		}
		catch (const std::exception& error) { WarnCollisionLayers(error); }
		reflect::RegisterEngineTypes(context.Types());
		context.Types().SetCollisionLabels(labels, aliases);
	}

	ProjectSettings ProjectSettings::Load(const std::filesystem::path& path)
	{
		std::ifstream input{ path };
		if (!input) throw std::runtime_error("Cannot open project: " + path.string());
		nlohmann::json document;
		input >> document;
		return Parse(document);
	}

	ProjectSettings ProjectSettings::Parse(const nlohmann::json& document)
	{
		if (!document.is_object() || document.value("format", std::string{}) != "vega.project" ||
			!document.contains("version") || !document["version"].is_number_integer() || document["version"] != 1)
			throw std::runtime_error("Expected vega.project version 1");
		ProjectSettings settings;
		if (document.contains("collisionLayers"))
		{
			try
			{
				const auto& layers = document.at("collisionLayers");
				if (!layers.is_array()) throw std::runtime_error("expected a list");
				for (const auto& layer : layers)
				{
					if (!layer.is_object() || !layer.contains("bit") || !layer["bit"].is_number_integer() ||
						layer["bit"].get<double>() < 0 || layer["bit"].get<double>() > 7)
						throw std::runtime_error("expected an object with bit from 0 to 7");
					CollisionLayerSetting entry;
					entry.bit = layer.at("bit").get<int>();
					entry.name = layer.at("name").get<std::string>();
					entry.aliases = layer.value("aliases", std::vector<std::string>{});
					settings.collisionLayers.push_back(std::move(entry));
				}
				ValidateCollisionLayers(settings.collisionLayers);
			}
			catch (const std::exception& error)
			{
				settings.collisionLayers.clear();
				WarnCollisionLayers(error);
			}
		}
		settings.name = document.at("name").get<std::string>();
		if (settings.name.find_first_not_of(" \t\r\n") == std::string::npos)
			throw std::runtime_error("Project name must not be empty");
		settings.startupScene = document.value("startupScene", std::string{});
		if (!settings.startupScene.empty())
		{
			const auto path = std::filesystem::u8path(settings.startupScene);
			if (path.is_absolute() || path.has_root_name() || path.has_root_directory() ||
				settings.startupScene.find('\\') != std::string::npos ||
				settings.startupScene.size() < 11 || settings.startupScene.substr(settings.startupScene.size() - 11) != ".scene.json")
				throw std::runtime_error("Startup scene must be an asset-relative .scene.json path");
			for (const auto& part : path)
				if (part == "..") throw std::runtime_error("Startup scene cannot leave the asset root");
		}
		if (document.contains("window"))
		{
			const auto& window = document["window"];
			if (!window.is_object()) throw std::runtime_error("Project window must be an object");
			const auto dimension = [&](const char* key, unsigned int fallback)
			{
				if (!window.contains(key)) return fallback;
				const auto& value = window[key];
				if (!value.is_number_integer() || value.get<double>() < 1 || value.get<double>() > 16384)
					throw std::runtime_error(std::string{ "Window " } + key + " must be an integer from 1 to 16384");
				return value.get<unsigned int>();
			};
			settings.windowWidth = dimension("width", settings.windowWidth);
			settings.windowHeight = dimension("height", settings.windowHeight);
		}
		settings.editor = document.value("editor", nlohmann::json::object());
		if (!settings.editor.is_object()) throw std::runtime_error("Project editor must be an object");
		if (settings.editor.contains("autosaveSeconds"))
		{
			const auto& seconds = settings.editor.at("autosaveSeconds");
			if (!seconds.is_number_integer() || seconds.get<double>() < 0 || seconds.get<double>() > 86400)
				throw std::runtime_error("Editor autosaveSeconds must be an integer from 0 to 86400 (0 disables periodic autosave)");
		}
		for (const auto* key : { "assetRoot", "font", "visualAssets", "gameBuildDir", "releaseGameBuildDir", "exportDir" })
			if (settings.editor.contains(key) && !settings.editor[key].is_string())
				throw std::runtime_error(std::string{ "Project editor " } + key + " must be a path string");
		const auto runtime = document.value("runtime", nlohmann::json::object());
		if (!runtime.is_object()) throw std::runtime_error("Project runtime must be an object");
		settings.runtimeExecutable = runtime.value("executable", settings.runtimeExecutable);
		settings.runtimeAssetDirectory = runtime.value("assetDirectory", settings.runtimeAssetDirectory);
		settings.runtimeVisualDirectory = runtime.value("visualDirectory", settings.runtimeVisualDirectory);
		settings.runtimeLog = runtime.value("log", settings.runtimeLog);
		for (const auto& value : { settings.runtimeExecutable, settings.runtimeAssetDirectory, settings.runtimeVisualDirectory, settings.runtimeLog })
		{
			const auto path = std::filesystem::u8path(value);
			if (path.empty() || path.is_absolute() || path.has_root_name() || path.has_root_directory() || value.find('\\') != std::string::npos)
				throw std::runtime_error("Runtime paths must be nonempty package-relative paths");
			for (const auto& part : path)
				if (part == ".." || part == ".") throw std::runtime_error("Runtime paths cannot leave the package");
		}
		if (std::filesystem::u8path(settings.runtimeExecutable).has_parent_path() || std::filesystem::u8path(settings.runtimeLog).has_parent_path())
			throw std::runtime_error("Runtime executable and log must be filenames");
		return settings;
	}

	nlohmann::json ProjectSettings::ToJson() const
	{
		nlohmann::json result = { { "format", "vega.project" }, { "version", 1 }, { "name", name }, { "startupScene", startupScene },
			{ "window", { { "width", windowWidth }, { "height", windowHeight } } }, { "editor", editor },
			{ "runtime", { { "executable", runtimeExecutable }, { "assetDirectory", runtimeAssetDirectory },
				{ "visualDirectory", runtimeVisualDirectory }, { "log", runtimeLog } } } };
		if (!collisionLayers.empty())
		{
			result["collisionLayers"] = nlohmann::json::array();
			for (const auto& layer : collisionLayers)
			{
				nlohmann::json entry = { { "bit", layer.bit }, { "name", layer.name } };
				if (!layer.aliases.empty()) entry["aliases"] = layer.aliases;
				result["collisionLayers"].push_back(std::move(entry));
			}
		}
		return result;
	}

	std::string ProjectSettings::ExecutableName() const
	{
		std::string result;
		for (unsigned char character : name)
			result += character < 32 || std::string{ "<>:\"/\\|?*" }.find(character) != std::string::npos ? '_' : static_cast<char>(character);
		while (!result.empty() && (result.back() == '.' || result.back() == ' ')) result.pop_back();
		if (result.empty()) result = "Game";
		auto stem = result.substr(0, result.find('.'));
		std::transform(stem.begin(), stem.end(), stem.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
		if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL" ||
			(stem.size() == 4 && (stem.substr(0, 3) == "COM" || stem.substr(0, 3) == "LPT") && stem[3] >= '1' && stem[3] <= '9')) result.insert(result.begin(), '_');
		return result + ".exe";
	}
}
