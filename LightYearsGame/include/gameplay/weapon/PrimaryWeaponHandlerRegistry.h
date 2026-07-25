#pragma once

#include "gameplay/weapon/PrimaryWeaponHandler.h"

namespace ly
{
	class PrimaryWeaponHandlerRegistry
	{
	public:
		static bool RegisterHandler(unique_ptr<PrimaryWeaponHandler> handler);
		static bool RegisterFeature(unique_ptr<PrimaryWeaponFeatureHandler> feature);

		static const PrimaryWeaponHandler* FindHandler(
			const GameplayTag& weaponTypeTag
		);
		static const PrimaryWeaponFeatureHandler* FindFeature(
			const GameplayTag& featureTag
		);

	private:
		static void EnsureBuiltIns();
	};
}
