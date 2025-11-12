#include "weapon/BulletShooter.h"
#include "framework/Core.h"
#include <framework/World.h>
#include "weapon/Bullet.h"

namespace ly
{
	BulletShooter::BulletShooter(Actor* owner, const std::string& texturePath, float cooldownTime,const sf::Vector2f& localPositionOffset, float localRotationOffset)
		: Shooter{ owner },
		mCooldownClock{},
		mCooldownTime{ cooldownTime },
		mTexturePath{ texturePath },
		mLocalPositionOffset{ localPositionOffset },
		mLocalRotationOffset{ localRotationOffset },
		mBulletSpeed{600.f}
	{

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
	void BulletShooter::IncrementLevel(int amt)
	{
		Shooter::IncrementLevel(amt);

	}
	void BulletShooter::ShootImp()
	{
		if (!GetOwner()) return;

		mCooldownClock.restart();

		weak_ptr<Bullet> newBullet = GetOwner()->GetWorld()->SpawnActor<Bullet>(GetOwner(), mTexturePath);

		newBullet.lock()->SetSpeed(mBulletSpeed);

		sf::Vector2f worldMuzzlePos = GetOwner()->GetActorLocation() + GetOwner()->TransformLocalToWorld(mLocalPositionOffset);

		// Mermiyi konumlandýr
		newBullet.lock()->SetActorLocation(worldMuzzlePos);
		newBullet.lock()->SetActorRotation(GetOwner()->GetActorRotation() + mLocalRotationOffset);

	}
}
