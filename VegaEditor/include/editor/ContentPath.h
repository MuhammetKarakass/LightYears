#pragma once
#include "editor/JsonDocument.h"
#include <cctype>

namespace vega
{
	inline std::string ContentPointer(const std::string& path)
	{
		if (path.empty()) throw std::runtime_error("Field path is empty");
		if (path.front() == '/') { const Json::json_pointer checked{ path }; return path; }
		std::string result;
		std::size_t cursor = 0;
		while (cursor < path.size())
		{
			const auto start = cursor;
			while (cursor < path.size() && path[cursor] != '.' && path[cursor] != '[' && path[cursor] != ']') ++cursor;
			if (start == cursor) throw std::runtime_error("Empty field path segment");
			result += '/';
			for (char c : path.substr(start, cursor - start)) result += c == '~' ? "~0" : c == '/' ? "~1" : std::string(1, c);
			while (cursor < path.size() && path[cursor] == '[')
			{
				const auto index = ++cursor;
				while (cursor < path.size() && std::isdigit(static_cast<unsigned char>(path[cursor]))) ++cursor;
				if (cursor == index || cursor == path.size() || path[cursor] != ']') throw std::runtime_error("Invalid array index");
				result += '/' + path.substr(index, cursor - index);
				++cursor;
			}
			if (cursor == path.size()) break;
			if (path[cursor++] != '.' || cursor == path.size()) throw std::runtime_error("Invalid field path");
		}
		return result;
	}
}
