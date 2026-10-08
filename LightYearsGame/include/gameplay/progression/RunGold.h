#pragma once

#include <algorithm>
#include <limits>

namespace ly
{
	class RunGold final
	{
	public:
		unsigned int GetAmount() const { return mAmount; }
		void Award(unsigned int amount) { mAmount += std::min(amount, std::numeric_limits<unsigned int>::max() - mAmount); }
	private:
		unsigned int mAmount = 0;
	};
}
