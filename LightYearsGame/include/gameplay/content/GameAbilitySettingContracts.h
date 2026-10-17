#pragma once

#include <string>

namespace vega::content
{
	// Registers shipped family contracts before ability JSON is parsed. New
	// families extend their own contract and this composition list, never loader
	// parsing logic.
	bool RegisterGameAbilitySettingContracts(std::string* failureReason = nullptr);
}
