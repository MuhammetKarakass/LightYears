#include "gameplay/ability/controllers/PrimaryWeaponController.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "weapon/Bullet.h"
#include <algorithm>
#include <cmath>

namespace ly
{
	PrimaryWeaponController::PrimaryWeaponController(Actor* owner, const PrimaryWeaponDefinition& weaponDefinition)
		: AbilityController{ owner, MakeAbilityDefinition(weaponDefinition) },
		mWeaponDefinition{ weaponDefinition },
		mRuntimeAttributes{ weaponDefinition.attributes },
		mTimeUntilNextShot{ 0.f }
	{
	}

	bool PrimaryWeaponController::CanActivate() const
	{
		return AbilityController::CanActivate()
			&& mWeaponDefinition.automaticFire
			&& mWeaponDefinition.deliveryType == PrimaryWeaponDeliveryType::Projectile;
	}

	void PrimaryWeaponController::OnActivate()
	{
		mTimeUntilNextShot = 0.f;
	}

	void PrimaryWeaponController::OnTickActive(float deltaTime)
	{
		mTimeUntilNextShot -= deltaTime;
		TryFire();
	}

	void PrimaryWeaponController::OnEnd()
	{
		mTimeUntilNextShot = 0.f;
	}

	void PrimaryWeaponController::TryFire()
	{
		const float fireInterval = GetFireInterval();

		while (mTimeUntilNextShot <= 0.f)
		{
			FireShot();
			mTimeUntilNextShot += fireInterval;
		}
	}

	void PrimaryWeaponController::FireShot()
	{
		Actor* owner = GetOwner();
		if (!owner || !owner->GetWorld())
		{
			return;
		}

		const int projectilesPerShot = GetProjectilesPerShot();
		const float spreadAngle = std::max(0.f, mRuntimeAttributes.spreadAngle.currentValue);
		const float angleStep = projectilesPerShot > 1 ? spreadAngle / static_cast<float>(projectilesPerShot - 1) : 0.f;
		const float startAngle = projectilesPerShot > 1 ? -spreadAngle * 0.5f : 0.f;

		const List<WeaponMuzzleDefinition>& muzzleDefinitions = mWeaponDefinition.muzzleDefinitions;
		if (muzzleDefinitions.empty())
		{
			for (int projectileIndex = 0; projectileIndex < projectilesPerShot; ++projectileIndex)
			{
				FireProjectile(WeaponMuzzleDefinition{}, startAngle + angleStep * projectileIndex);
			}
			return;
		}

		for (const WeaponMuzzleDefinition& muzzleDefinition : muzzleDefinitions)
		{
			for (int projectileIndex = 0; projectileIndex < projectilesPerShot; ++projectileIndex)
			{
				FireProjectile(muzzleDefinition, startAngle + angleStep * projectileIndex);
			}
		}
	}

	void PrimaryWeaponController::FireProjectile(const WeaponMuzzleDefinition& muzzleDefinition, float localRotationOffset)
	{
		Actor* owner = GetOwner();
		if (!owner || !owner->GetWorld())
		{
			return;
		}

		weak_ptr<Bullet> newBullet = owner->GetWorld()->SpawnActor<Bullet>(
			owner,
			mWeaponDefinition.presentationDefinition,
			mRuntimeAttributes
		);

		if (auto bullet = newBullet.lock())
		{
			const sf::Vector2f worldMuzzlePosition = owner->GetActorLocation() + owner->TransformLocalToWorld(muzzleDefinition.offset);

			bullet->SetActorLocation(worldMuzzlePosition);
			bullet->SetActorRotation(owner->GetActorRotation() + muzzleDefinition.rotationOffset + localRotationOffset);
		}
	}

	float PrimaryWeaponController::GetFireInterval() const
	{
		const float shotsPerSecond = std::max(0.01f, mRuntimeAttributes.shotsPerSecond.currentValue);
		return 1.f / shotsPerSecond;
	}

	int PrimaryWeaponController::GetProjectilesPerShot() const
	{
		return std::max(1, static_cast<int>(std::round(mRuntimeAttributes.projectilesPerShot.currentValue)));
	}

	AbilityDefinition PrimaryWeaponController::MakeAbilityDefinition(const PrimaryWeaponDefinition& weaponDefinition)
	{
		return AbilityDefinition{
			weaponDefinition.weaponId,
			AbilitySlot::PrimaryFire,
			AbilityControllerType::PrimaryWeapon,
			AbilityActivationPolicy::WhileHeld,
			0.f,
			0.f,
			0,
			{
				GameplayTag{ "Ability.Primary" },
				GameplayTag{ "Ability.Offense" },
				GameplayTag{ "Ability.Projectile" }
			}
		};
	}
}
