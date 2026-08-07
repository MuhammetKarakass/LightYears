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
			const GameplayTag& GetTypeTag() const override
			{
				return PrimaryWeaponSchema::Projectile::Standard::TypeTag;
			}

			const List<GameplayTag>& GetOwnedAttributeRoots() const override
			{
				return PrimaryWeaponBuiltIns::ProjectileDeliveryAttributeRoots();
			}

			PrimaryWeaponValidationResult ValidateDefinition(
				const PrimaryWeaponDefinition& definition
			) const override
			{
				for (const GameplayTag& required : {
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

			void FireOnce(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponTypeRuntimeState&
			) const override
			{
				const int projectileCount = 1 + std::max(
					0,
					static_cast<int>(std::round(sas::FindGameplayAttributeValue(
						context.attributes,
						PrimaryWeaponSchema::Projectile::Delivery::AdditionalProjectileCount,
						0.f
					)))
				);
				PrimaryWeaponProjectileSpawner::FireSet(
					context,
					projectileCount,
					0.f
				);
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
