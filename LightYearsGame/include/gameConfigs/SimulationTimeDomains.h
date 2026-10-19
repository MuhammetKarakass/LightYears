#pragma once

#include <framework/SimulationTime.h>

namespace vega::GameTimeDomains
{
	// The engine only defines RealTime; these values are persisted in prefab test
	// fixtures, so keep Hostile = 1 and Projectile = 2.
	inline constexpr vega::SimulationTimeDomain HostileGameplay{ 1 };
	inline constexpr vega::SimulationTimeDomain ProjectileGameplay{ 2 };
}
