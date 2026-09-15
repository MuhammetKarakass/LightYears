#pragma once

#include <cmath>
#include <cstddef>
#include <limits>

namespace ly::math
{
	inline bool IsFiniteNonNegativeMultiplier(float value) noexcept
	{
		return std::isfinite(value) && value >= 0.f;
	}

	inline bool TryResolveMultiplierProduct(
		const float* values,
		std::size_t count,
		float& result
	) noexcept
	{
		if (count > 0 && values == nullptr)
		{
			return false;
		}

		long double logarithmicProduct = 0.0L;
		for (std::size_t index = 0; index < count; ++index)
		{
			const float value = values[index];
			if (!IsFiniteNonNegativeMultiplier(value))
			{
				return false;
			}
			if (value == 0.f)
			{
				result = 0.f;
				return true;
			}
			logarithmicProduct += std::log(static_cast<long double>(value));
		}

		const long double maximum = static_cast<long double>(std::numeric_limits<float>::max());
		const long double minimum = static_cast<long double>(std::numeric_limits<float>::denorm_min());
		const long double maximumLog = std::log(maximum);
		const long double minimumLog = std::log(minimum);
		if (std::isnan(logarithmicProduct))
		{
			return false;
		}
		if (logarithmicProduct == std::numeric_limits<long double>::infinity() || logarithmicProduct >= maximumLog)
		{
			result = std::numeric_limits<float>::max();
			return true;
		}
		if (logarithmicProduct == -std::numeric_limits<long double>::infinity())
		{
			result = 0.f;
			return true;
		}
		if (logarithmicProduct <= minimumLog)
		{
			result = 0.f;
			return true;
		}

		const long double product = std::exp(logarithmicProduct);
		if (!std::isfinite(product) || product >= maximum)
		{
			result = std::numeric_limits<float>::max();
			return true;
		}
		result = static_cast<float>(product);
		return std::isfinite(result) && result >= 0.f;
	}
}
