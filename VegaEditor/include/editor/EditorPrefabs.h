#pragma once
#include "editor/EditorCommands.h"
#include <filesystem>

namespace vega
{
	bool IsPrefabPath(const std::filesystem::path& path);
	std::vector<std::string> PrefabPaths(const std::filesystem::path& root);
	bool WritePrefab(EditorContext&, const std::string& path, const nlohmann::json& document, bool overwrite = true);
	bool CreatePrefab(EditorContext&, CommandHistory&, const std::string& path);
	bool CreateVariant(EditorContext&, const std::string& parent, const std::string& path);
	bool PlacePrefab(EditorContext&, CommandHistory&, const std::string& path, sf::Vector2f location);
	bool RefreshPrefabs(EditorContext&, CommandHistory&);
	bool PrefabPropertyOverridden(EditorContext&, vega::Actor&, const ReflectedTarget&, const std::string& property);
	bool ChangePrefabProperty(EditorContext&, CommandHistory&, const ReflectedTarget&, const std::string& property, bool apply);
	bool ChangePrefabSection(EditorContext&, CommandHistory&, const ReflectedTarget&, bool apply);
	bool ChangeAllPrefab(EditorContext&, CommandHistory&, bool apply);
}
