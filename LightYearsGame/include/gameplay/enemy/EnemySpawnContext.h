#pragma once

#include <cstdint>

namespace ly
{
	struct EnemySpawnContext
	{
		int level = 1;
		uint32_t variationSeed = 0;
	};
}
