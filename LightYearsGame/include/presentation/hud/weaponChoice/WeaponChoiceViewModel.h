#pragma once

#include "framework/Core.h"
#include "widget/UIViewModel.h"

#include <cstdint>
#include <string>
#include <vector>

namespace vega
{
	struct WeaponChoiceOption
	{
		std::string weaponId;
		std::string label;
		bool operator==(const WeaponChoiceOption& other) const { return weaponId == other.weaponId && label == other.label; }
	};

	// Second Hardpoint weapon choice: open while a module waits for its second Primary weapon.
	struct WeaponChoiceViewModel
	{
		bool open{ false };
		std::uint32_t moduleInstanceId{ 0 };
		std::vector<WeaponChoiceOption> options;
		UIRevision revision;
	};
}
