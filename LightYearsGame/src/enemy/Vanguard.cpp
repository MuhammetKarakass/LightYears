#include "enemy/Vanguard.h"
#include "weapon/BulletShooter.h"
#include <framework/World.h>

namespace ly
{
	Vanguard::Vanguard(World* ownningWorld, const std::string& texturePath, const sf::Vector2f& velocity):
		EnemySpaceShip(ownningWorld, texturePath),
		mShooter{ new BulletShooter(this,"SpaceShooterRedux/PNG/Lasers/laserRed01.png",0.5f) }  // Vanguard için özel mermi atýcý
	{
		SetVelocity(velocity);
		SetActorRotation(180.f);
		SetExplosionType(ExplosionType::Boss);
	}

	Vanguard::~Vanguard()
	{
		LOG("Vanguard destroyed");  // Yýkýcý mesajý
	}



	void Vanguard::Tick(float deltaTime)
	{
		EnemySpaceShip::Tick(deltaTime);
		Shoot();
	}

	void Vanguard::Shoot()
	{
		if(mShooter)
		{
			mShooter->Shoot();  // Mermi atma iþlemi
		}
		else
		{
			LOG("Vanguard mShooter is not initialized!");  // Hata mesajý
		}
	}
}