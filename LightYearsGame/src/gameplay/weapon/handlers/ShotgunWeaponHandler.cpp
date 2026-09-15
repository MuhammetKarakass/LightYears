#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"

#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/weapon/impact/ShotgunVolleyImpactGroup.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileSpawner.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		class ShotgunWeaponHandler final : public PrimaryWeaponHandler
		{
		public:
			PrimaryWeaponType GetType() const override
			{
				return PrimaryWeaponType::ProjectileShotgun;
			}

			bool FireOnce(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponTypeRuntimeState&
			) const override
			{
				const int pelletCount = std::max(
					1,
					static_cast<int>(std::round(sas::FindAttributeValue(
						context.attributes,
						PrimaryWeaponSchema::Projectile::Shotgun::PelletCount,
						1.f
					)))
				);
				const int additionalProjectiles = std::max(
					0,
					static_cast<int>(std::round(sas::FindAttributeValue(
						context.attributes,
						PrimaryWeaponSchema::Projectile::Delivery::AdditionalProjectileCount,
						0.f
					)))
				);
				const int totalPelletCount = pelletCount + additionalProjectiles;
				const float spreadAngle = std::max(0.f, sas::FindAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Projectile::Shotgun::SpreadAngle,
					0.f
				));
				const float damageReduction = sas::FindAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Projectile::Shotgun::DamageReductionPerAdditionalHit,
					0.f
				);
				if (damageReduction <= 0.f)
				{
					return PrimaryWeaponProjectileSpawner::FireSet(
						context,
						totalPelletCount,
						spreadAngle
					) > 0;
				}

				const float minimumDamageMultiplier = sas::FindAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Projectile::Shotgun::MinimumDamageMultiplier,
					1.f
				);
				const float baseDamage = std::max(0.f, sas::FindAttributeValue(
					context.attributes,
					CommonAttributeIds::Damage,
					0.f
				));
				const shared_ptr<ShotgunVolleyImpactGroup> impactGroup =
					std::make_shared<ShotgunVolleyImpactGroup>(
						context.owner,
						context.damageTags,
						baseDamage,
						damageReduction,
						minimumDamageMultiplier,
						DamageTypeSystem::BuildPayload(
							context.damageTags,
							context.attributes
						)
					);
				return PrimaryWeaponProjectileSpawner::FireSet(
					context,
					totalPelletCount,
					spreadAngle,
					impactGroup
				) > 0;
			}
		};
	}

	namespace PrimaryWeaponBuiltIns
	{
		unique_ptr<PrimaryWeaponHandler> CreateShotgunWeaponHandler()
		{
			return std::make_unique<ShotgunWeaponHandler>();
		}
	}
}
