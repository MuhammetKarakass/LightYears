#pragma once

#include "attributes/AttributeSystem.h"
#include "gameConfigs/combat/WeaponStructs.h"

namespace ly
{
	struct PrimaryWeaponValidationResult
	{
		bool isValid = false;
		std::string reason;
	};

	using PrimaryWeaponDefinitionValidationFn = PrimaryWeaponValidationResult (*)(const PrimaryWeaponDefinition& definition);

	struct PrimaryWeaponTypeValidationContract
	{
		PrimaryWeaponType weaponType;
		List<sas::AttributeId> ownedAttributeRoots;
		List<sas::AttributeId> inheritedAttributeRoots;
		bool usesIntervalFire = true;
		PrimaryWeaponDefinitionValidationFn validate = nullptr;
	};

	struct PrimaryWeaponFeatureValidationContract
	{
		PrimaryWeaponFeatureType featureType;
		List<sas::AttributeId> attributeRoots;
		PrimaryWeaponDefinitionValidationFn validate = nullptr;
	};

	class PrimaryWeaponValidationContractRegistry
	{
	public:
		static const PrimaryWeaponTypeValidationContract* FindType(PrimaryWeaponType weaponType);
		static const PrimaryWeaponFeatureValidationContract* FindFeature(PrimaryWeaponFeatureType featureType);
	};
}
