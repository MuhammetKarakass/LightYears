#include "editor/ContentWatch.h"
#include "editor/JsonDocument.h"
#include <algorithm>
#include <cctype>
#include <fstream>

namespace vega
{
	void ContentWatch::Begin(const std::filesystem::path& root)
	{
		mRoot = std::filesystem::weakly_canonical(root);
		mFiles.clear();
		Poll();
	}

	std::vector<std::filesystem::path> ContentWatch::Poll()
	{
		std::vector<std::filesystem::path> changed;
		if (mRoot.empty()) return changed;
		std::map<std::filesystem::path, Stamp> current;
		JsonDocument resolver{ mRoot };
		std::error_code error;
		for (std::filesystem::recursive_directory_iterator it{ mRoot, error }, end; !error && it != end; it.increment(error))
		{
			auto name = it->path().filename().string();
			std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			if (it->is_symlink(error) || name == ".vega") { it.disable_recursion_pending(); continue; }
			if (!it->is_regular_file(error) || it->path().extension() != ".json" || name == "vega.project.json" ||
				(name.size() >= 11 && name.substr(name.size() - 11) == ".scene.json")) continue;
			std::filesystem::path path;
			if (!resolver.Resolve(it->path().lexically_relative(mRoot), path)) continue;
			const Stamp stamp{ it->last_write_time(error), it->file_size(error) };
			if (error) break;
			current.emplace(path, stamp);
			const auto old = mFiles.find(path);
			if (old != mFiles.end() && old->second.time == stamp.time && old->second.size == stamp.size) continue;
			// Editors often save via truncate/write. An incomplete JSON document must
			// wait for the next poll instead of clearing a live game's catalog.
			try
			{
				std::ifstream input{ path };
				const auto value = Json::parse(input);
				(void)value;
				changed.push_back(path);
			}
			catch (const std::exception&)
			{
				if (old == mFiles.end()) current.erase(path);
				else current[path] = old->second;
			}
		}
		// A partial enumeration must not manufacture deletions. Missing catalogs
		// keep the last valid runtime state until a replacement appears.
		if (!error) mFiles = std::move(current);
		return changed;
	}
}
