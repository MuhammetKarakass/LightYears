#include "PrimaryWeaponBuiltIns.h"

namespace ly::PrimaryWeaponBuiltIns
{
	const List<GameplayTag>& ProjectileDeliveryAttributeRoots()
	{
		static const List<GameplayTag> roots{
			PrimaryWeaponSchema::Projectile::Delivery::AttributeRoot
		};
		return roots;
	}

	const List<GameplayTag>& ShotgunAttributeRoots()
	{
		static const List<GameplayTag> roots{
			PrimaryWeaponSchema::Projectile::Shotgun::AttributeRoot
		};
		return roots;
	}

	const List<GameplayTag>& ArcAttributeRoots()
	{
		static const List<GameplayTag> roots{
			PrimaryWeaponSchema::Arc::Electric::AttributeRoot
		};
		return roots;
	}

	const List<GameplayTag>& BeamDeliveryAttributeRoots()
	{
		static const List<GameplayTag> roots{
			PrimaryWeaponSchema::Beam::Delivery::AttributeRoot
		};
		return roots;
	}

	const List<GameplayTag>& WaveDeliveryAttributeRoots()
	{
		static const List<GameplayTag> roots{
			PrimaryWeaponSchema::Wave::Delivery::AttributeRoot
		};
		return roots;
	}

	const List<GameplayTag>& HeatAttributeRoots()
	{
		static const List<GameplayTag> roots{
			PrimaryWeaponSchema::Feature::Heat::AttributeRoot
		};
		return roots;
	}

	const GameplayAttribute* FindDefinitionAttribute(
		const PrimaryWeaponDefinition& definition,
		const GameplayTag& attributeId
	)
	{
		return FindGameplayAttribute(definition.attributes, attributeId);
	}

	PrimaryWeaponValidationResult RequireAttribute(
		const PrimaryWeaponDefinition& definition,
		const GameplayTag& attributeId,
		const char* ownerName
	)
	{
		return FindDefinitionAttribute(definition, attributeId)
			? PrimaryWeaponValidationResult{ true, {} }
			: PrimaryWeaponValidationResult{
				false,
				std::string{ ownerName } + " requires attribute '" + attributeId.ToString() + "'."
			};
	}
}
