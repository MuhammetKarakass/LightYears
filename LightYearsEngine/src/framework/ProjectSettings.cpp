#include "framework/ProjectSettings.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>

namespace ly
{
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
		return settings;
	}

	nlohmann::json ProjectSettings::ToJson() const
	{
		return { { "format", "vega.project" }, { "version", 1 }, { "name", name }, { "startupScene", startupScene },
			{ "window", { { "width", windowWidth }, { "height", windowHeight } } } };
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
