#pragma once

#include <filesystem>
#include <string>

namespace ly::visual
{
	bool ReadTextFile(const std::filesystem::path& file, std::string& text);
	// Last write time, or file_time_type::min() when the file is missing.
	std::filesystem::file_time_type FileStamp(const std::filesystem::path& file);
}
