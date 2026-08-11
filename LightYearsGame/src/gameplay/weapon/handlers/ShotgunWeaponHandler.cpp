#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "../internal/PrimaryWeaponBuiltIns.h"

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

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return PrimaryWeaponBuiltIns::ShotgunAttributeRoots();
			}

			const List<sas::AttributeId>& GetInheritedAttributeRoots() const override
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
					PrimaryWeaponSchema::Projectile::Delivery::Lifetime,
					PrimaryWeaponSchema::Projectile::Shotgun::PelletCount,
					PrimaryWeaponSchema::Projectile::Shotgun::SpreadAngle
				})
				{
					const PrimaryWeaponValidationResult result =
						PrimaryWeaponBuiltIns::RequireAttribute(
							definition,
							required,
							"Shotgun weapon"
						);
					if (!result.isValid)
					{
						return result;
					}
				}

				const float pelletCount = PrimaryWeaponBuiltIns::FindDefinitionAttribute(
					definition,
					PrimaryWeaponSchema::Projectile::Shotgun::PelletCount
				)->baseValue;
				if (pelletCount < 2.f || std::round(pelletCount) != pelletCount)
				{
					return {
						false,
						"Shotgun pellet count must be an integer of at least two."
					};
				}
				if (PrimaryWeaponBuiltIns::FindDefinitionAttribute(
					definition,
					PrimaryWeaponSchema::Projectile::Shotgun::SpreadAngle
				)->baseValue < 0.f)
				{
					return { false, "Shotgun spread angle cannot be negative." };
				}

				const sas::GameplayAttribute* damageReduction =
					PrimaryWeaponBuiltIns::FindDefinitionAttribute(
						definition,
						PrimaryWeaponSchema::Projectile::Shotgun::DamageReductionPerAdditionalHit
					);
				const sas::GameplayAttribute* minimumMultiplier =
					PrimaryWeaponBuiltIns::FindDefinitionAttribute(
						definition,
						PrimaryWeaponSchema::Projectile::Shotgun::MinimumDamageMultiplier
					);
				if (damageReduction || minimumMultiplier)
				{
					if (!damageReduction || !minimumMultiplier)
					{
						return {
							false,
							"Shotgun pellet falloff requires both reduction and minimum multiplier attributes."
						};
					}
					if (damageReduction->baseValue <= 0.f ||
						damageReduction->baseValue >= 1.f)
					{
						return {
							false,
							"Shotgun pellet damage reduction must be between zero and one."
						};
					}
					if (minimumMultiplier->baseValue <= 0.f ||
						minimumMultiplier->baseValue > 1.f)
					{
						return {
							false,
							"Shotgun minimum damage multiplier must be greater than zero and at most one."
						};
					}
					if (sas::FindAttributeValue(
							definition.attributes,
							AreaAttributeIds::Radius,
							0.f
						) > 0.f ||
						sas::FindAttributeValue(
							definition.attributes,
							PrimaryWeaponSchema::Projectile::Delivery::PierceCount,
							0.f
						) > 0.f)
					{
						return {
							false,
							"Shotgun pellet falloff only supports direct, non-piercing pellets."
						};
					}
				}
				return { true, {} };
			}

			void FireOnce(
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
					PrimaryWeaponProjectileSpawner::FireSet(
						context,
						totalPelletCount,
						spreadAngle
					);
					return;
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
				PrimaryWeaponProjectileSpawner::FireSet(
					context,
					totalPelletCount,
					spreadAngle,
					impactGroup
				);
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
