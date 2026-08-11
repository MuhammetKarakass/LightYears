#include "attributes/AttributeSystem.h"
#include "PrimaryWeaponBuiltIns.h"

namespace ly::PrimaryWeaponBuiltIns
{
	const List<sas::AttributeId>& ProjectileDeliveryAttributeRoots()
	{
		static const List<sas::AttributeId> roots{
			PrimaryWeaponSchema::Projectile::Delivery::Root
		};
		return roots;
	}

	const List<sas::AttributeId>& ShotgunAttributeRoots()
	{
		static const List<sas::AttributeId> roots{
			PrimaryWeaponSchema::Projectile::Shotgun::Root
		};
		return roots;
	}

	const List<sas::AttributeId>& ArcAttributeRoots()
	{
		static const List<sas::AttributeId> roots{
			PrimaryWeaponSchema::Arc::Electric::Root
		};
		return roots;
	}

	const List<sas::AttributeId>& BeamDeliveryAttributeRoots()
	{
		static const List<sas::AttributeId> roots{
			PrimaryWeaponSchema::Beam::Delivery::Root
		};
		return roots;
	}

	const List<sas::AttributeId>& WaveDeliveryAttributeRoots()
	{
		static const List<sas::AttributeId> roots{
			PrimaryWeaponSchema::Wave::Delivery::Root
		};
		return roots;
	}

	const List<sas::AttributeId>& HeatAttributeRoots()
	{
		static const List<sas::AttributeId> roots{
			PrimaryWeaponSchema::Feature::Heat::Root
		};
		return roots;
	}

	const sas::GameplayAttribute* FindDefinitionAttribute(
		const PrimaryWeaponDefinition& definition,
		const sas::AttributeId& attributeId
	)
	{
		return sas::FindAttribute(definition.attributes, attributeId);
	}

	PrimaryWeaponValidationResult RequireAttribute(
		const PrimaryWeaponDefinition& definition,
		const sas::AttributeId& attributeId,
		const char* ownerName
	)
	{
		return FindDefinitionAttribute(definition, attributeId)
			? PrimaryWeaponValidationResult{ true, {} }
			: PrimaryWeaponValidationResult{
				false,
				std::string{ ownerName } + " requires attribute '" + std::string{ attributeId.GetName() } + "'."
			};
	}
}
