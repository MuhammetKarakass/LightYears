#include "enemy/TwinBlade.h"
#include "weapon/BulletShooter.h"
#include <framework/World.h>

namespace ly
{
	TwinBlade::TwinBlade(World* owningWorld, const std::string& texturePath,const sf::Vector2f& velocity)
		: EnemySpaceShip(owningWorld, texturePath),
		mShooterLeft( new BulletShooter(this,"SpaceShooterRedux/PNG/Lasers/laserRed01.png", 1.f, {-20.f, 40.f}) ),
		mShooterRight( new BulletShooter(this,"SpaceShooterRedux/PNG/Lasers/laserRed01.png", 1.f, {20.f, 40.f}) )
	{
		SetVelocity(velocity);
		SetActorRotation(180.f);
		SetExplosionType(ExplosionType::Heavy);
		mShooterLeft->SetBulletSpeed(400.f);
		mShooterRight->SetBulletSpeed(400.f);
	}
	TwinBlade::~TwinBlade()
	{
	}
	void TwinBlade::Tick(float deltaTime)
	{
		EnemySpaceShip::Tick(deltaTime);
		Shoot();
	}
	void TwinBlade::Shoot()
	{
		if (mShooterLeft && mShooterRight)
		{
			mShooterLeft->Shoot();
			mShooterRight->Shoot();
		}
	}
}