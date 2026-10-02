#include "FileUtil.h"

#include <fstream>
#include <sstream>

namespace ly::visual
{
	bool ReadTextFile(const std::filesystem::path& file, std::string& text)
	{
		std::ifstream stream(file, std::ios::binary);
		if (!stream)
		{
			return false;
		}
		std::ostringstream buffer;
		buffer << stream.rdbuf();
		text = buffer.str();
		return true;
	}

	std::filesystem::file_time_type FileStamp(const std::filesystem::path& file)
	{
		std::error_code ec;
		auto stamp = std::filesystem::last_write_time(file, ec);
		return ec ? std::filesystem::file_time_type::min() : stamp;
	}
}
