#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "../internal/PrimaryWeaponBuiltIns.h"

#include "gameplay/weapon/projectile/PrimaryWeaponProjectileSpawner.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		class StandardProjectileWeaponHandler final : public PrimaryWeaponHandler
		{
		public:
			PrimaryWeaponType GetType() const override
			{
				return PrimaryWeaponType::ProjectileStandard;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return PrimaryWeaponBuiltIns::ProjectileDeliveryAttributeRoots();
			}

			PrimaryWeaponValidationResult ValidateDefinition(
				const PrimaryWeaponDefinition& definition
			) const override
			{
				for (const sas::AttributeId& required : {
					CommonAttributeIds::Damage,
					PrimaryWeaponSchema::Projectile::Delivery::Speed,
					PrimaryWeaponSchema::Projectile::Delivery::Lifetime
				})
				{
					const PrimaryWeaponValidationResult result =
						PrimaryWeaponBuiltIns::RequireAttribute(
							definition,
							required,
							"Projectile weapon"
						);
					if (!result.isValid)
					{
						return result;
					}
				}
				return { true, {} };
			}

			bool FireOnce(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponTypeRuntimeState&
			) const override
			{
				const int projectileCount = 1 + std::max(
					0,
					static_cast<int>(std::round(sas::FindAttributeValue(
						context.attributes,
						PrimaryWeaponSchema::Projectile::Delivery::AdditionalProjectileCount,
						0.f
					)))
				);
				return PrimaryWeaponProjectileSpawner::FireSet(
					context,
					projectileCount,
					0.f
				) > 0;
			}
		};
	}

	namespace PrimaryWeaponBuiltIns
	{
		unique_ptr<PrimaryWeaponHandler> CreateStandardProjectileWeaponHandler()
		{
			return std::make_unique<StandardProjectileWeaponHandler>();
		}
	}
}
