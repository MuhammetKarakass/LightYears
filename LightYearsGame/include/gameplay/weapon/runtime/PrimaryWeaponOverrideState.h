#pragma once

#include "content/ContentId.h"
#include "gameConfigs/combat/WeaponStructs.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"

#include <cstddef>
#include <vector>

namespace ly
{
	using PrimaryWeaponOverrideHandle = std::size_t;

	// Temporary primary weapons replace the PrimaryFire payload rather than
	// granting another ability into the same slot. This keeps the equipped weapon
	// intact for transformations, pickups, and future runtime weapon swaps.
	class PrimaryWeaponOverrideState
	{
	public:
		struct ActiveOverride
		{
			PrimaryWeaponOverrideHandle handle = 0;
			sas::ContentId sourceId;
			PrimaryWeaponDefinition weaponDefinition;
			int priority = 0;
			PrimaryWeaponRuntimeState runtime;
		};

		PrimaryWeaponOverrideHandle Push(
			const sas::ContentId& sourceId,
			const PrimaryWeaponDefinition& weaponDefinition,
			int priority = 0
		);
		bool Remove(PrimaryWeaponOverrideHandle handle);
		void Clear();

		ActiveOverride* GetActive();
		const ActiveOverride* GetActive() const;

	private:
		std::vector<ActiveOverride>::iterator FindActive();
		std::vector<ActiveOverride>::const_iterator FindActive() const;

		PrimaryWeaponOverrideHandle mNextHandle = 1;
		std::vector<ActiveOverride> mOverrides;
	};
}
