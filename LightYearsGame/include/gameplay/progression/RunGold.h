#pragma once

#include <algorithm>
#include <limits>

namespace vega
{
	class RunGold final
	{
	public:
		unsigned int GetAmount() const { return mAmount; }
		bool TrySpend(unsigned int amount) { if (amount > mAmount) return false; mAmount -= amount; return true; }
		void Award(unsigned int amount) { mAmount += std::min(amount, std::numeric_limits<unsigned int>::max() - mAmount); }
	private:
		unsigned int mAmount = 0;
	};
}
