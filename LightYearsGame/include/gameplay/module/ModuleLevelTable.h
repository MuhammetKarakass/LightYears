#pragma once

#include "framework/Core.h"

#include <cstddef>
#include <cmath>

namespace vega
{
	enum class ModuleLevelGrowth { Linear, Compound, Step };

	template<typename T>
	struct ModuleLevelTable
	{
		List<T> values;
		ModuleLevelGrowth growth = ModuleLevelGrowth::Linear;
		double ratio = 0.0; // Zero derives the compound ratio from the last two authored values.
		int every = 1;
		double amount = 0.0; // Compound: added after multiplication. Step: added every N levels, starting at L1.

		T ValueAtLevel(T fallback, int level) const
		{
			if (values.empty() || level <= 0) return fallback;
			const int count = static_cast<int>(values.size());
			if (level <= count) return values[static_cast<std::size_t>(level - 1)];
			if (growth == ModuleLevelGrowth::Step)
				return static_cast<T>(values.back() + amount * ((level - 1) / every - (count - 1) / every));
			if (growth == ModuleLevelGrowth::Compound)
			{
				const double factor = ratio > 0.0 ? ratio : (count > 1 ? static_cast<double>(values.back()) / values[static_cast<std::size_t>(count - 2)] : 1.0);
				const double power = std::pow(factor, level - count);
				return static_cast<T>(values.back() * power + amount * (factor == 1.0 ? level - count : (power - 1.0) / (factor - 1.0)));
			}
			if (count == 1) return values.front();
			const T step = values.back() - values[static_cast<std::size_t>(count - 2)];
			return values.back() + step * static_cast<T>(level - count);
		}

		bool operator==(const ModuleLevelTable& other) const { return values == other.values && growth == other.growth && ratio == other.ratio && every == other.every && amount == other.amount; }
	};
}
