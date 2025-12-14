#include "weapon/BulletShooter.h"
#include "framework/Core.h"
#include <framework/World.h>
#include "weapon/Bullet.h"

namespace ly
{
	BulletShooter::BulletShooter(Actor* owner,
		const std::string& texturePath,
		float cooldownTime,
		const sf::Vector2f& localPositionOffset,
		float localRotationOffset,
		float bulletDamage,
		float bulletSpeed)
		: Shooter{ owner },
		mCooldownClock{},
		mCooldownTime{ cooldownTime },
		mTexturePath{ texturePath },
		mBulletSpeed{ bulletSpeed },
		mBulletDamage{ bulletDamage }
	{
		SetLocalPositionOffset(localPositionOffset);
		SetLocalRotationOffset(localRotationOffset);
	}
	bool BulletShooter::IsOnCooldown() const
	{
		if (mCooldownClock.getElapsedTime().asSeconds() > mCooldownTime/ GetCooldownMultiplier() /*GetCurrentLevel()*/)
		{
			return false;
		}

		return true;
	}
	
	void BulletShooter::SetBulletSpeed(float speed)
	{
		mBulletSpeed = speed;
	}
	void BulletShooter::SetBulletDamage(float damage)
	{
		mBulletDamage = damage;
	}

	void BulletShooter::SetCooldownTime(float cooldownTime)
	{
		mCooldownTime = cooldownTime;
	}

	void BulletShooter::IncrementLevel(int amt)
	{
		Shooter::IncrementLevel(amt);

	}

	void BulletShooter::RestartCooldown()
	{
		mCooldownClock.restart();
	}
	
	void BulletShooter::ShootImp()
	{
		if (!GetOwner()) return;

		mCooldownClock.restart();

		weak_ptr<Bullet> newBullet = GetOwner()->GetWorld()->SpawnActor<Bullet>(GetOwner(), mTexturePath);

		if (auto bullet = newBullet.lock())
		{
			bullet->SetSpeed(mBulletSpeed);
			bullet->SetDamage(mBulletDamage);

			sf::Vector2f worldMuzzlePos = GetOwner()->GetActorLocation() +
				GetOwner()->TransformLocalToWorld(GetLocalPositionOffset());

			bullet->SetActorLocation(worldMuzzlePos);
			bullet->SetActorRotation(GetOwner()->GetActorRotation() + GetLocalRotationOffset());
		}
	}
}
