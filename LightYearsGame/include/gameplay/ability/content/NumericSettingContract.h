#pragma once

#include <set>
#include <string>

namespace ly::content
{
	// Declares the numeric JSON keys a concrete ability family owns. The loader
	// uses this data generically and never needs to know individual settings.
	struct NumericSettingContract
	{
		std::set<std::string> allowed;
		std::set<std::string> required;

		bool operator==(const NumericSettingContract& other) const
		{
			return allowed == other.allowed && required == other.required;
		}
	};
}
