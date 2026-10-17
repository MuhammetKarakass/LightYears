#include "editor/ContentWatch.h"
#include "editor/JsonDocument.h"
#include <framework/debug/Log.h>
#include <algorithm>
#include <cctype>
#include <fstream>

namespace vega
{
	void ContentWatch::Report(const std::string& message)
	{
		// Persistent filesystem errors must not flood the Console every poll.
		if (mReportedErrors.insert(message).second)
			vega::debug::Log::Write(vega::debug::LogLevel::Warning, vega::debug::LogChannel::Editor, {}, "Content watch: " + message);
	}

	void ContentWatch::Begin(const std::filesystem::path& root)
	{
		mFiles.clear();
		mReportedErrors.clear();
		mRoot.clear();
		try
		{
			std::error_code error;
			mRoot = std::filesystem::weakly_canonical(root, error);
			if (error) { mRoot.clear(); Report(error.message()); return; }
			Poll();
		}
		catch (const std::exception& error) { Report(error.what()); }
	}

	std::vector<std::filesystem::path> ContentWatch::Poll()
	{
		std::vector<std::filesystem::path> changed;
		if (mRoot.empty()) return changed;
		// Keep unreadable/missing entries at their last valid stamp. Commit every
		// successful read even if another entry or directory cannot be enumerated.
		auto current = mFiles;
		try
		{
			JsonDocument resolver{ mRoot };
			std::error_code error;
			std::filesystem::recursive_directory_iterator it{ mRoot, std::filesystem::directory_options::skip_permission_denied, error }, end;
			if (error) Report(error.message());
			while (it != end)
			{
				try
				{
					auto name = it->path().filename().u8string();
					std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
					const auto inspect = [&]()
					{
						std::error_code status;
						if (it->is_symlink(status) || name == ".vega") { it.disable_recursion_pending(); return; }
						if (status) { Report(it->path().generic_u8string() + ": " + status.message()); return; }
						const bool regular = it->is_regular_file(status);
						if (status) { Report(it->path().generic_u8string() + ": " + status.message()); return; }
						if (!regular || it->path().extension() != ".json" || name == "vega.project.json" ||
							(name.size() >= 11 && name.substr(name.size() - 11) == ".scene.json")) return;
						std::filesystem::path path;
						if (!resolver.Resolve(it->path().lexically_relative(mRoot), path)) return;
						Stamp stamp{};
						if (mReadStamp) mReadStamp(*it, stamp.time, stamp.size, status);
						else
						{
							stamp.time = it->last_write_time(status);
							if (!status) stamp.size = it->file_size(status);
						}
						if (status) { Report(it->path().generic_u8string() + ": " + status.message()); return; }
						const auto old = mFiles.find(path);
						if (old != mFiles.end() && old->second.time == stamp.time && old->second.size == stamp.size) return;
						// Truncate/write saves may leave incomplete JSON until the next poll.
						try
						{
							std::ifstream input{ path };
							const auto value = Json::parse(input);
							(void)value;
							current[path] = stamp;
							changed.push_back(path);
						}
						catch (const std::exception&) {}
					};
					inspect();
				}
				catch (const std::exception& failure) { Report(failure.what()); it.disable_recursion_pending(); }
				error.clear();
				it.increment(error);
				if (error) Report(error.message());
			}
		}
		catch (const std::exception& error) { Report(error.what()); }
		mFiles = std::move(current);
		return changed;
	}
}
