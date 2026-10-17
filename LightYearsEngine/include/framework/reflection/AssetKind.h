#pragma once

#include <filesystem>

namespace ly::reflect
{
	// Marks a String property that holds an asset-root-relative path. The stored value stays a plain
	// string, so scenes, prefabs and Lua are unchanged; the kind only drives editor pickers and checks.
	enum class AssetKind { None, Texture, Sound, Font, Prefab, Scene, Script, Any };

	const char* AssetKindName(AssetKind kind);
	// Human-readable file patterns, e.g. "*.png *.jpg"; used in refusal messages.
	const char* AssetKindFilter(AssetKind kind);
	// Matches the file name only (the file need not exist), case-insensitively. Prefab and Scene
	// need their compound suffix, so "level.json" is not a scene. None accepts nothing.
	bool AcceptsAsset(AssetKind kind, const std::filesystem::path& path);
}
