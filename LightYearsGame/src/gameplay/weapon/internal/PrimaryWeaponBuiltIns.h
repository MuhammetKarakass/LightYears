#pragma once

#include "gameplay/weapon/PrimaryWeaponHandler.h"

namespace ly::PrimaryWeaponBuiltIns
{
	unique_ptr<PrimaryWeaponHandler> CreateStandardProjectileWeaponHandler();
	unique_ptr<PrimaryWeaponHandler> CreateShotgunWeaponHandler();
	unique_ptr<PrimaryWeaponHandler> CreateElectricArcWeaponHandler();
	unique_ptr<PrimaryWeaponHandler> CreateContinuousBeamWeaponHandler();
	unique_ptr<PrimaryWeaponHandler> CreateExpandingWaveWeaponHandler();
	unique_ptr<PrimaryWeaponFeatureHandler> CreateHeatFeatureHandler();
}
