#pragma once
#include <filesystem>
#include <map>
#include <vector>
#include <functional>
#include <set>
#include <string>

namespace vega
{
	// The host calls Poll on its half-second authoring cadence, never per frame.
	class ContentWatch
	{
	public:
		using StampReader = std::function<void(const std::filesystem::directory_entry&,
			std::filesystem::file_time_type&, std::uintmax_t&, std::error_code&)>;
		explicit ContentWatch(StampReader reader = {}) : mReadStamp{ std::move(reader) } {}
		void Begin(const std::filesystem::path& root);
		std::vector<std::filesystem::path> Poll();
	private:
		struct Stamp
		{
			std::filesystem::file_time_type time;
			std::uintmax_t size;
		};
		std::filesystem::path mRoot;
		std::map<std::filesystem::path, Stamp> mFiles;
		StampReader mReadStamp;
		std::set<std::string> mReportedErrors;
		void Report(const std::string& message);
	};
}
