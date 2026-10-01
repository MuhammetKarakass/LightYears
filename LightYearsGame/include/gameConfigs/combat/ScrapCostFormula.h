#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

namespace ly
{
	// Linear upgrade price shared by abilities and primary weapons. Reaching
	// level L (L >= 2) costs base + step * (L - 2) scrap for every level, with no
	// authored ceiling. base == 0 means the progression is not purchasable.
	struct ScrapCostFormula
	{
		unsigned int base = 0;
		unsigned int step = 0;

		bool IsPurchasable() const
		{
			return base > 0;
		}

		bool IsPurchasableToLevel(int targetLevel) const
		{
			return targetLevel >= 2 && IsPurchasable();
		}

		unsigned int GetCostToReachLevel(int targetLevel) const
		{
			if (!IsPurchasableToLevel(targetLevel))
			{
				return 0;
			}
			const std::uint64_t cost =
				static_cast<std::uint64_t>(base) +
				static_cast<std::uint64_t>(step) * static_cast<std::uint64_t>(targetLevel - 2);
			return static_cast<unsigned int>(std::min<std::uint64_t>(
				cost,
				std::numeric_limits<unsigned int>::max()
			));
		}
	};
}
