#include "weapon/BulletShooter.h"
#include "framework/Core.h"
#include <framework/World.h>
#include "weapon/Bullet.h"

namespace ly{
	BulletShooter::BulletShooter(Actor* owner, float cooldownTime)
		: Shooter{owner},
		mCooldownTime{cooldownTime},
		mCooldownClock{}
	{
	}
	bool BulletShooter::IsOnCooldown() const
	{
		if (mCooldownClock.getElapsedTime().asSeconds() > mCooldownTime) return false;

		return true;
	}

	void BulletShooter::ShootImp()
	{
		mCooldownClock.restart();
		weak_ptr<Bullet> newBullet = GetOwner()->GetWorld()->SpawnActor<Bullet>(GetOwner(), "SpaceShooterRedux/PNG/Lasers/laserBlue01.png");
		newBullet.lock()->SetActorLocation(GetOwner()->GetActorLocation()+sf::Vector2f(0.f,-50.f));
		newBullet.lock()->SetActorRotation(GetOwner()->GetActorRotation());

	}
}	
