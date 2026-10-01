#pragma once

namespace ly
{
	// The single cadence for every periodic or continuous damage source: area and
	// trail fields, beams, cones, infections and status effects such as Ignite.
	// Damage authored as "per tick" is applied once per Interval; damage authored
	// as damage-per-second is applied as DPS * Interval once per Interval. No source
	// owns a private interval, so changing this value retimes all of them together.
	// Same-target hit cooldowns (drones, lances, barricade contact) are not combat
	// ticks and keep their own authored values.
	namespace CombatTick
	{
		inline constexpr float Interval = 0.25f;
	}
}
