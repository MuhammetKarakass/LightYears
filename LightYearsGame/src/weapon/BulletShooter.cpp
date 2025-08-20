#include "weapon/BulletShooter.h"
#include "framework/Core.h"
#include <framework/World.h>
#include "weapon/Bullet.h"

namespace ly
{
	BulletShooter::BulletShooter(Actor* owner, const std::string& texturePath, float cooldownTime)
		: Shooter{ owner },
		mCooldownClock{},
		mCooldownTime{ cooldownTime },
		mTexturePath{ texturePath }
	{

	}
	bool BulletShooter::IsOnCooldown() const
	{
		if (mCooldownClock.getElapsedTime().asSeconds() > mCooldownTime)
		{
			return false;
		}

		return true;
	}
	void BulletShooter::ShootImp()
	{
		mCooldownClock.restart();
		weak_ptr<Bullet> newBullet = GetOwner()->GetWorld()->SpawnActor<Bullet>(GetOwner(), mTexturePath);
		
		// Sprite'ýn local coordinate'inde {0, -40} offset (sprite'ýn önünden çýksýn)
		sf::Vector2f localMuzzleOffset{0.f, +40.f}; // y = forward direction (sprite'ýn önü)
		sf::Vector2f worldMuzzlePos = GetOwner()->GetActorLocation() + GetOwner()->TransformLocalToWorld(localMuzzleOffset);
		
		newBullet.lock()->SetActorLocation(worldMuzzlePos);
		newBullet.lock()->SetActorRotation(GetOwner()->GetActorRotation());

	}
}
