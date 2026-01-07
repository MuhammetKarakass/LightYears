#include "weapon/BulletShooter.h"
#include "framework/Core.h"
#include "framework/AudioManager.h"
#include <framework/World.h>
#include "weapon/Bullet.h"

namespace ly
{
	BulletShooter::BulletShooter(Actor* owner,
		const BulletDefinition& bulletDef,
		float cooldownTime,
		const sf::Vector2f& localPositionOffset,
		float localRotationOffset,
		ShootSoundMode soundMode
		)
		: Shooter{ owner, soundMode },
		mCooldownClock{},
		mCooldownTime{ cooldownTime },
		mBulletDef{ bulletDef }
	{
		if (soundMode == ShootSoundMode::Single)
		{
			SetShootSoundProps("SpaceShooterRedux/Bonus/sfx_laser1.ogg", 20.0f, 1.f);
			UpdateSoundInterval();
		}
		
		SetLocalPositionOffset(localPositionOffset);
		SetLocalRotationOffset(localRotationOffset);
	}

	bool BulletShooter::IsOnCooldown() const
	{
		if (mCooldownClock.getElapsedTime().asSeconds() > mCooldownTime / GetCooldownMultiplier())
		{
			return false;
		}

		return true;
	}
	
	void BulletShooter::SetBulletSpeed(float speed)
	{
		mBulletDef.speed = speed;
	}

	void BulletShooter::SetBulletDamage(float damage)
	{
		mBulletDef.damage = damage;
	}

	void BulletShooter::SetCooldownTime(float cooldownTime)
	{
		mCooldownTime = cooldownTime;
		UpdateSoundInterval();
	}

	void BulletShooter::IncrementLevel(int amt)
	{
		Shooter::IncrementLevel(amt);
		UpdateSoundInterval();
	}

	void BulletShooter::RestartCooldown()
	{
		mCooldownClock.restart();
	}

	void BulletShooter::UpdateSoundInterval()
	{
		float effectiveCooldown = mCooldownTime / GetCooldownMultiplier();
		SetMinSoundInterval(effectiveCooldown);
	}
	
	void BulletShooter::ShootImp()
	{
		if (!GetOwner()) return;

		mCooldownClock.restart();
		 
		weak_ptr<Bullet> newBullet = GetOwner()->GetWorld()->SpawnActor<Bullet>(GetOwner(), mBulletDef);

		if (auto bullet = newBullet.lock())
		{
			bullet->SetSpeed(mBulletDef.speed);
			bullet->SetDamage(mBulletDef.damage);

			sf::Vector2f worldMuzzlePos = GetOwner()->GetActorLocation() +
				GetOwner()->TransformLocalToWorld(GetLocalPositionOffset());

			bullet->SetActorLocation(worldMuzzlePos);
			bullet->SetActorRotation(GetOwner()->GetActorRotation() + GetLocalRotationOffset());

			if (mSoundMode == ShootSoundMode::Single)
			{
				if (GetOwner()->GetCollisionLayer() == CollisionLayer::Player)
				{
					PlayShootSound();
				}
			}
		}
	}
}
