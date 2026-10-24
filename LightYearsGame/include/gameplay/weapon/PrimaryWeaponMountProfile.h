#pragma once

namespace vega
{
	// Mount shape of the Primary fire, resolved from installed modules and copied into each shot's context. The default is the
	// plain forward shot. Turret and Broadside are mutually exclusive (conflictsWith); if both were ever set, Broadside wins.
	struct PrimaryWeaponMountProfile
	{
		// Turret Mount: the forward axis points at the nearest valid enemy in range, independent of the ship's facing.
		bool turret = false;
		// Broadside Mount: each volley fires from both sides of the ship instead of forward.
		bool broadside = false;
		// Execution Priority (Turret side feature): the valid target with the lowest health fraction is preferred.
		bool executionPriority = false;
		// Coefficient of the damage of each Broadside side volley.
		float broadsideDamageMultiplier = 1.f;
	};
}
