#pragma once

#include <cstdint>

namespace ly
{
	// Actors opt into a gameplay time domain instead of changing the engine-wide
	// clock. RealTime keeps the existing behavior; hostile actors and projectile
	// actors can be slowed independently without affecting UI/camera time.
	enum class SimulationTimeDomain : uint8_t
	{
		RealTime,
		HostileGameplay,
		ProjectileGameplay
	};

	// A modifier is source-owned so multiple future temporal effects can coexist
	// and remove only the modifier they created.
	using SimulationTimeModifierSourceId = std::uint64_t;
}
