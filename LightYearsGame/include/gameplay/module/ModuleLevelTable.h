#pragma once

#include "framework/Core.h"

#include <cstddef>

namespace vega
{
	template<typename T>
	struct ModuleLevelTable
	{
		List<T> values;
		// Levels past the authored table extrapolate linearly from its last step.
		T ValueAtLevel(T fallback, int level) const
		{
			if (values.empty() || level <= 0) return fallback;
			const int count = static_cast<int>(values.size());
			if (level <= count) return values[static_cast<std::size_t>(level - 1)];
			if (count == 1) return values.front();
			const T step = values.back() - values[static_cast<std::size_t>(count - 2)];
			return values.back() + step * static_cast<T>(level - count);
		}
	};
}
