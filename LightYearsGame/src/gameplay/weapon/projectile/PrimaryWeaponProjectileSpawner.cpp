#include "attributes/AttributeSystem.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileSpawner.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/module/rules/EpicPrimaryRules.h"

#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/weapon/impact/ProjectileImpactBehavior.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h"
#include "framework/MathUtility.h"

#include <algorithm>

namespace vega::ProjectileMotion
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

namespace vega::PrimaryWeaponProjectileSpawner
{
	namespace
	{
		bool FireProjectile(const PrimaryWeaponExecutionContext& context, const WeaponMuzzleDefinition& muzzle, float volleyRotation, float localRotationOffset, const sf::Vector2f& carrierVelocity, const shared_ptr<ProjectileImpactBehavior>& impactBehavior, float damageScale, const sf::Vector2f& lateralOffset)
		{
			if (!context.owner.GetWorld())
			{
				return false;
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
				const float shotDamage = ResolvePrimaryWeaponShotDamage(context);
				spawnedProjectile->SetDamage(shotDamage * damageScale);
				spawnedProjectile->SetDamageTags(context.damageTags);
				spawnedProjectile->SetShotMetadata(context.shotMetadata);
				const sf::Vector2f location = context.owner.GetActorLocation() +
					context.owner.TransformLocalToWorld(muzzle.offset) + lateralOffset;
				spawnedProjectile->SetActorLocation(location);
				spawnedProjectile->SetActorRotation(volleyRotation + muzzle.rotationOffset + localRotationOffset);
				const float projectileSpeed = sas::FindAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Projectile::Delivery::Speed,
					500.f
				);
				spawnedProjectile->SetLaunchVelocity(
					spawnedProjectile->GetActorForwardDirection() * projectileSpeed + carrierVelocity
				);
				return true;
			}
			else if (impactBehavior)
			{
				impactBehavior->OnProjectileFinished();
			}
			return false;
		}
	}

	int FireSet(
		const PrimaryWeaponExecutionContext& context,
		int projectileCount,
		float spreadAngle,
		const shared_ptr<ProjectileImpactBehavior>& impactBehavior
	)
	{
		int spawnedCount = 0;
		const int count = std::max(1, projectileCount);
		const float angleStep =
			count > 1 ? spreadAngle / static_cast<float>(count - 1) : 0.f;
		const float startAngle = count > 1 ? -spreadAngle * 0.5f : 0.f;
		const PrimaryWeaponMountProfile& mount = context.mountProfile;
		// Turret Mount points the forward axis at the chosen enemy; Broadside replaces the forward volley with one per side.
		const float forwardRotation = EpicPrimaryMounts::FindTurretRotation(context.owner, mount, sas::FindAttributeValue(context.attributes, CommonAttributeIds::Range, 1600.f)).value_or(context.owner.GetActorRotation());
		const List<float> volleySides = mount.broadside ? List<float>{ 90.f, -90.f } : List<float>{ 0.f };
		const float damageScale = mount.broadside ? mount.broadsideDamageMultiplier : 1.f;
		const auto secondRotation = mount.splitFire && context.runtime && context.runtime->splitFireShot ? EpicPrimaryMounts::FindTurretRotation(context.owner, mount, sas::FindAttributeValue(context.attributes, CommonAttributeIds::Range, 1600.f), 1) : std::nullopt;
		const auto fireFromMuzzle = [&](const WeaponMuzzleDefinition& muzzle)
		{
			List<float> directions = volleySides;
			if (secondRotation) directions.push_back(*secondRotation - forwardRotation);
			for (const float sideAngle : directions)
			{
				const float volleyRotation = forwardRotation + sideAngle;
				// A side volley starts one lateral offset from the ship centre, on the side it fires from.
				const sf::Vector2f lateralOffset = mount.broadside && sideAngle != 0.f ? RotationToVector(volleyRotation) * EpicPrimaryMounts::kBroadsideSpawnOffset : sf::Vector2f{};
				const sf::Vector2f muzzleFireDirection = RotationToVector(volleyRotation + muzzle.rotationOffset - 90.f);
				// Every pellet in this muzzle's volley receives the same ship-motion term.
				const sf::Vector2f carrierVelocity = ProjectileMotion::ResolveCarrierVelocity(context.owner.GetVelocity(), muzzleFireDirection);
				for (int index = 0; index < count; ++index)
				{
					if (FireProjectile(context, muzzle, volleyRotation, startAngle + angleStep * static_cast<float>(index), carrierVelocity, impactBehavior, damageScale, lateralOffset))
					{
						++spawnedCount;
					}
				}
			}
		};

		if (context.definition.muzzleDefinitions.empty())
		{
			fireFromMuzzle(WeaponMuzzleDefinition{});
			return spawnedCount;
		}
		const auto& muzzles = context.definition.muzzleDefinitions;
		if (context.definition.alternateMuzzles && muzzles.size() > 1 && context.runtime)
		{
			// One muzzle per shot, in order, so a twin gun keeps one hit per shot.
			fireFromMuzzle(muzzles[context.runtime->successfulFireCount % muzzles.size()]);
			return spawnedCount;
		}
		for (const WeaponMuzzleDefinition& muzzle : muzzles)
		{
			fireFromMuzzle(muzzle);
		}
		return spawnedCount;
	}
}
