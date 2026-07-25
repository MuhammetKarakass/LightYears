#pragma once

#include "gameplay/weapon/PrimaryWeaponHandler.h"

namespace ly::PrimaryWeaponBuiltIns
{
	const List<GameplayTag>& ProjectileDeliveryAttributeRoots();
	const List<GameplayTag>& ShotgunAttributeRoots();
	const List<GameplayTag>& ArcAttributeRoots();
	const List<GameplayTag>& BeamDeliveryAttributeRoots();
	const List<GameplayTag>& WaveDeliveryAttributeRoots();
	const List<GameplayTag>& HeatAttributeRoots();

	const GameplayAttribute* FindDefinitionAttribute(
		const PrimaryWeaponDefinition& definition,
		const GameplayTag& attributeId
	);
	PrimaryWeaponValidationResult RequireAttribute(
		const PrimaryWeaponDefinition& definition,
		const GameplayTag& attributeId,
		const char* ownerName
	);

	unique_ptr<PrimaryWeaponHandler> CreateStandardProjectileWeaponHandler();
	unique_ptr<PrimaryWeaponHandler> CreateShotgunWeaponHandler();
	unique_ptr<PrimaryWeaponHandler> CreateElectricArcWeaponHandler();
	unique_ptr<PrimaryWeaponHandler> CreateContinuousBeamWeaponHandler();
	unique_ptr<PrimaryWeaponHandler> CreateExpandingWaveWeaponHandler();
	unique_ptr<PrimaryWeaponFeatureHandler> CreateHeatFeatureHandler();
}
