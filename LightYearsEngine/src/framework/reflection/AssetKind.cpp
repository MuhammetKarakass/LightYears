#include "framework/reflection/AssetKind.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace ly::reflect
{
	namespace
	{
		std::string LowerName(const std::filesystem::path& path)
		{
			auto name = path.filename().u8string();
			// ASCII-only folding; UTF-8 continuation bytes are left untouched.
			std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return name;
		}
		bool EndsWith(const std::string& name, std::string_view suffix)
		{
			// The stem must be non-empty, so a bare ".prefab.json" is not an asset.
			return name.size() > suffix.size() && name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
		}
	}

	const char* AssetKindName(AssetKind kind)
	{
		switch (kind)
		{
		case AssetKind::Texture: return "Texture";
		case AssetKind::Sound: return "Sound";
		case AssetKind::Font: return "Font";
		case AssetKind::Prefab: return "Prefab";
		case AssetKind::Scene: return "Scene";
		case AssetKind::Script: return "Script";
		case AssetKind::Any: return "Any";
		case AssetKind::None: break;
		}
		return "None";
	}

	const char* AssetKindFilter(AssetKind kind)
	{
		switch (kind)
		{
		case AssetKind::Texture: return "*.png *.jpg *.jpeg *.bmp *.tga";
		case AssetKind::Sound: return "*.wav *.ogg *.flac *.mp3";
		case AssetKind::Font: return "*.ttf *.otf";
		case AssetKind::Prefab: return "*.prefab.json";
		case AssetKind::Scene: return "*.scene.json";
		case AssetKind::Script: return "*.lua";
		case AssetKind::Any: return "any file";
		case AssetKind::None: break;
		}
		return "";
	}

	bool AcceptsAsset(AssetKind kind, const std::filesystem::path& path)
	{
		const auto name = LowerName(path);
		if (name.empty()) return false;
		switch (kind)
		{
		case AssetKind::Texture:
			return EndsWith(name, ".png") || EndsWith(name, ".jpg") || EndsWith(name, ".jpeg") || EndsWith(name, ".bmp") || EndsWith(name, ".tga");
		case AssetKind::Sound:
			return EndsWith(name, ".wav") || EndsWith(name, ".ogg") || EndsWith(name, ".flac") || EndsWith(name, ".mp3");
		case AssetKind::Font:
			return EndsWith(name, ".ttf") || EndsWith(name, ".otf");
		case AssetKind::Prefab:
			return EndsWith(name, ".prefab.json");
		case AssetKind::Scene:
			return EndsWith(name, ".scene.json");
		case AssetKind::Script:
			return EndsWith(name, ".lua");
		case AssetKind::Any:
			return true;
		case AssetKind::None:
			break;
		}
		return false;
	}
}
