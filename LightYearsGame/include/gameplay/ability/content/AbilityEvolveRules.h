#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include <array>
#include <algorithm>

namespace vega
{
	inline constexpr std::array<int, 2> AbilityEvolveTierLevels{ 8, 15 };

	struct PendingAbilityEvolve
	{
		int tierLevel = 0;
		List<AbilityEvolveOption> options;
	};

	// Earned choices depend on the real ability level, never module level bonuses.
	inline PendingAbilityEvolve QueryPendingAbilityEvolve(const AbilityEvolveContent* content, int level, const List<std::string>& selection)
	{
		PendingAbilityEvolve pending;
		if (!content || content->options.size() != 3 || selection.size() >= AbilityEvolveTierLevels.size()) return pending;
		for (std::size_t index = 0; index < selection.size(); ++index)
		{
			if (std::find(selection.begin(), selection.begin() + index, selection[index]) != selection.begin() + index) return pending;
			if (std::none_of(content->options.begin(), content->options.end(), [&](const auto& option) { return option.id == selection[index]; })) return pending;
		}
		const int tier = AbilityEvolveTierLevels[selection.size()];
		if (level < tier) return pending;
		pending.tierLevel = tier;
		for (const auto& option : content->options)
			if (std::find(selection.begin(), selection.end(), option.id) == selection.end()) pending.options.push_back(option);
		return pending;
	}
}
