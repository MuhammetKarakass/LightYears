#pragma once
#include <string>
#include <algorithm>

namespace vega
{
	inline bool IsSkyHandle(const std::string& handle)
	{
		if (handle == "planet" || handle == "planet.scale" || handle == "band.angle" || handle == "band.offset" || handle == "orbit.disk" || handle == "orbit.horizon" || handle == "orbit.angle" || handle == "orbit.radius") return true;
		if (handle.find("object.") != 0) return false;
		const auto dot = handle.find('.', 7);
		const auto index = handle.substr(7, dot == std::string::npos ? dot : dot - 7);
		if (index.empty() || !std::all_of(index.begin(), index.end(), [](char c) { return c >= '0' && c <= '9'; })) return false;
		return dot == std::string::npos || handle.substr(dot) == ".scale" || handle.substr(dot) == ".rotate";
	}
}
