#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"

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
