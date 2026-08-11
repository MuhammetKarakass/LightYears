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
			PrimaryWeaponType weaponType
		);
		static const PrimaryWeaponFeatureHandler* FindFeature(
			PrimaryWeaponFeatureType featureType
		);

	private:
		static void EnsureBuiltIns();
	};
}
