#include "attributes/AttributeSystem.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileSpawner.h"

#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/weapon/impact/ProjectileImpactBehavior.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h"
#include "framework/MathUtility.h"

#include <algorithm>

namespace ly::ProjectileMotion
{
	sf::Vector2f ResolveCarrierVelocity(
		const sf::Vector2f& ownerVelocity,
		const sf::Vector2f& fireDirection
	)
	{
		sf::Vector2f normalizedFireDirection = fireDirection;
		const float fireDirectionLength = GetVectorLength(normalizedFireDirection);
		if (fireDirectionLength <= 0.001f)
		{
			return {};
		}
		normalizedFireDirection /= fireDirectionLength;

		const sf::Vector2f lateralDirection{
			-normalizedFireDirection.y,
			normalizedFireDirection.x
		};
		const float forwardVelocity =
			ownerVelocity.x * normalizedFireDirection.x +
			ownerVelocity.y * normalizedFireDirection.y;
		const float lateralVelocity =
			ownerVelocity.x * lateralDirection.x +
			ownerVelocity.y * lateralDirection.y;

		return
			normalizedFireDirection * std::max(0.f, forwardVelocity) * ForwardVelocityInheritance +
			lateralDirection * lateralVelocity * LateralVelocityInheritance;
	}
}

namespace ly::PrimaryWeaponProjectileSpawner
{
	namespace
	{
		void FireProjectile(
			const PrimaryWeaponExecutionContext& context,
			const WeaponMuzzleDefinition& muzzle,
			float localRotationOffset,
			const sf::Vector2f& carrierVelocity,
			const shared_ptr<ProjectileImpactBehavior>& impactBehavior
		)
		{
			if (!context.owner.GetWorld())
			{
				return;
			}
			if (impactBehavior)
			{
				impactBehavior->OnProjectileSpawned();
			}

			weak_ptr<PrimaryWeaponProjectileActor> projectile =
				context.owner.GetWorld()->SpawnActor<PrimaryWeaponProjectileActor>(
					&context.owner,
					context.definition.presentationDefinition,
					context.attributes
				);
			if (const shared_ptr<PrimaryWeaponProjectileActor> spawnedProjectile =
				projectile.lock())
			{
				if (impactBehavior)
				{
					spawnedProjectile->SetImpactBehavior(impactBehavior);
				}
				spawnedProjectile->SetDamageTags(context.damageTags);
				const sf::Vector2f location = context.owner.GetActorLocation() +
					context.owner.TransformLocalToWorld(muzzle.offset);
				spawnedProjectile->SetActorLocation(location);
				spawnedProjectile->SetActorRotation(
					context.owner.GetActorRotation() +
						muzzle.rotationOffset +
						localRotationOffset
				);
				const float projectileSpeed = sas::FindGameplayAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Projectile::Delivery::Speed,
					500.f
				);
				spawnedProjectile->SetLaunchVelocity(
					spawnedProjectile->GetActorForwardDirection() * projectileSpeed + carrierVelocity
				);
			}
			else if (impactBehavior)
			{
				impactBehavior->OnProjectileFinished();
			}
		}
	}

	void FireSet(
		const PrimaryWeaponExecutionContext& context,
		int projectileCount,
		float spreadAngle,
		const shared_ptr<ProjectileImpactBehavior>& impactBehavior
	)
	{
		const int count = std::max(1, projectileCount);
		const float angleStep =
			count > 1 ? spreadAngle / static_cast<float>(count - 1) : 0.f;
		const float startAngle = count > 1 ? -spreadAngle * 0.5f : 0.f;
		const auto fireFromMuzzle = [&](const WeaponMuzzleDefinition& muzzle)
		{
			const sf::Vector2f muzzleFireDirection = RotationToVector(
				context.owner.GetActorRotation() + muzzle.rotationOffset - 90.f
			);
			// Every pellet in this muzzle's volley receives the same ship-motion term.
			const sf::Vector2f carrierVelocity = ProjectileMotion::ResolveCarrierVelocity(
				context.owner.GetVelocity(),
				muzzleFireDirection
			);
			for (int index = 0; index < count; ++index)
			{
				FireProjectile(
					context,
					muzzle,
					startAngle + angleStep * static_cast<float>(index),
					carrierVelocity,
					impactBehavior
				);
			}
		};

		if (context.definition.muzzleDefinitions.empty())
		{
			fireFromMuzzle(WeaponMuzzleDefinition{});
			return;
		}
		for (const WeaponMuzzleDefinition& muzzle : context.definition.muzzleDefinitions)
		{
			fireFromMuzzle(muzzle);
		}
	}
}
