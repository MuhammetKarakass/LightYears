#pragma once

namespace ly
{
	// Closed implementation selector for actor-handler dispatch. It is not a
	// gameplay tag and does not represent a content identity.
	enum class AbilityActorType
	{
		Generic,
		RocketProjectile,
		OverdriveCoreProjectile,
		GravityAnomalyProjectile,
		GravityAnomalyField,
		InfernoSprayFlameCone,
		SunBeamStrike
	};
}
