#pragma once

#include <cstdint>

namespace ly
{
	struct EnemySpawnContext
	{
		int level = 1;
		uint32_t variationSeed = 0;
		// Free-roaming enemies keep the legacy camera-view cull. Spawn policies that
		// own an enemy's relevance (for example an encounter wave) disable it so
		// camera visibility never determines that enemy's lifetime.
		bool windowCullEnabled = true;
	};
}
