#pragma once
#include <filesystem>
#include <map>
#include <vector>

namespace vega
{
	// The host calls Poll on its half-second authoring cadence, never per frame.
	class ContentWatch
	{
	public:
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
	};
}
