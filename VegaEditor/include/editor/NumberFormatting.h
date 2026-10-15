#pragma once

#include <charconv>
#include <cstdio>
#include <string>

namespace vega
{
	inline std::string DoubleInputFormat(double value)
	{
		// Compare the actual text too: "1000" is shorter than "1e+03".
		int shortest = 64;
		int bestPrecision = 17;
		for (int precision = 1; precision <= 17; ++precision)
		{
			char text[64];
			const int size = std::snprintf(text, sizeof(text), "%.*g", precision, value);
			double parsed = 0.;
			const auto result = std::from_chars(text, text + size, parsed);
			if (result.ec == std::errc{} && result.ptr == text + size && parsed == value && size < shortest)
			{
				shortest = size;
				bestPrecision = precision;
			}
		}
		return "%." + std::to_string(bestPrecision) + "g";
	}
}
