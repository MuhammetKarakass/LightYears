#pragma once

namespace vega::sky
{
	// Brightness order is the readability contract: a lower tier never out-shines a higher one.
	enum class LuminanceTier
	{
		Background,
		Environment,
		Decorative,
		EnemyProjectile,
		Player
	};

	constexpr int LuminanceTierCount = 5;

	constexpr int TierIndex(LuminanceTier tier)
	{
		return static_cast<int>(tier);
	}

	inline const char* TierName(LuminanceTier tier)
	{
		switch (tier)
		{
		case LuminanceTier::Background: return "background";
		case LuminanceTier::Environment: return "environment";
		case LuminanceTier::Decorative: return "decorative";
		case LuminanceTier::EnemyProjectile: return "enemyProjectile";
		case LuminanceTier::Player: return "player";
		}
		return "unknown";
	}
}
