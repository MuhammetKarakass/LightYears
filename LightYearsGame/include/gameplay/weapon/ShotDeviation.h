#pragma once

namespace vega
{
	class Actor;

	namespace ShotDeviationIds
	{
		// Timed debuff on a shooter; its MaxDegrees attribute holds the deviation bound.
		inline constexpr char EffectId[] = "Effect.Debuff.ShotDeviation";
		inline constexpr char MaxDegreesAttributeId[] = "Effect.Debuff.ShotDeviation.MaxDegrees";
	}

	// Makes every projectile the shooter fires from now on turn by a random angle in [-maxDegrees, maxDegrees] for seconds.
	// Already flying projectiles are not changed. Reapplying keeps the larger bound and the longer remaining time.
	bool ApplyShotDeviation(Actor& shooter, float maxDegrees, float seconds, Actor* source);
	// Current deviation bound of the shooter (0 when it has none).
	float GetShotDeviationDegrees(const Actor& shooter);
	// One random deviation sample for a newly fired projectile (0 when the shooter has no deviation).
	float SampleShotDeviationDegrees(const Actor& shooter);
}
