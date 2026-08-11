#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/weapon/PrimaryWeaponHandler.h"

namespace ly::PrimaryWeaponBuiltIns
{
	const List<sas::AttributeId>& ProjectileDeliveryAttributeRoots();
	const List<sas::AttributeId>& ShotgunAttributeRoots();
	const List<sas::AttributeId>& ArcAttributeRoots();
	const List<sas::AttributeId>& BeamDeliveryAttributeRoots();
	const List<sas::AttributeId>& WaveDeliveryAttributeRoots();
	const List<sas::AttributeId>& HeatAttributeRoots();

	const sas::GameplayAttribute* FindDefinitionAttribute(
		const PrimaryWeaponDefinition& definition,
		const sas::AttributeId& attributeId
	);
	PrimaryWeaponValidationResult RequireAttribute(
		const PrimaryWeaponDefinition& definition,
		const sas::AttributeId& attributeId,
		const char* ownerName
	);

	unique_ptr<PrimaryWeaponHandler> CreateStandardProjectileWeaponHandler();
	unique_ptr<PrimaryWeaponHandler> CreateShotgunWeaponHandler();
	unique_ptr<PrimaryWeaponHandler> CreateElectricArcWeaponHandler();
	unique_ptr<PrimaryWeaponHandler> CreateContinuousBeamWeaponHandler();
	unique_ptr<PrimaryWeaponHandler> CreateExpandingWaveWeaponHandler();
	unique_ptr<PrimaryWeaponFeatureHandler> CreateHeatFeatureHandler();
}
