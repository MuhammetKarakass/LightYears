#pragma once

#include <framework/reflection/Reflection.h>
#include <filesystem>
#include <string>
#include <vector>

namespace vega { class Actor; }

namespace vega
{
	struct ReflectedTarget;
	class CommandHistory;
	struct EditorContext;

	// Asset paths are asset-root-relative with '/' separators. These helpers never touch the
	// filesystem outside root: absolute paths, root-name paths and escapes are refused.
	bool ResolveAssetFile(const std::filesystem::path& root, const std::filesystem::path& relative, std::filesystem::path& file);
	// Regular files under root that the kind accepts, sorted; hidden folders such as .vega are skipped.
	std::vector<std::string> ListAssetChoices(const std::filesystem::path& root, vega::reflect::AssetKind kind);
	// Checks a drop or assignment before any value is written. The file must exist under root with a
	// name the kind accepts; folders and other kinds are refused with a message for the Inspector.
	bool CheckAssetAssignment(const std::filesystem::path& root, const vega::reflect::PropertyInfo& property,
		const std::filesystem::path& relative, std::string& error);
	// Writes the path through SetPropertyCommand, so the edit joins the undo history like any property.
	bool AssignAssetProperty(EditorContext& context, CommandHistory& history, vega::Actor& actor, const ReflectedTarget& target,
		const vega::reflect::PropertyInfo& property, const std::filesystem::path& root, const std::filesystem::path& relative, std::string& error);
	// Inspector widget for a String property with an asset kind. Returns true when value changed.
	bool DrawAssetField(const vega::reflect::PropertyInfo& property, std::string& value, bool mixed, const std::filesystem::path& root);
}
