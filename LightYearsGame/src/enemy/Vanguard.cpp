#include "enemy/Vanguard.h"
#include "weapon/BulletShooter.h"
#include <framework/World.h>

namespace ly
{
	Vanguard::Vanguard(World* ownningWorld, const std::string& texturePath, const sf::Vector2f& velocity):
		EnemySpaceShip(ownningWorld, texturePath, 75.f, GetDefaultRewards()),
		mShooter{ new BulletShooter (this,"SpaceShooterRedux/PNG/Lasers/laserRed01.png",1.f,{0.f,40.f}) }  // Vanguard için özel mermi atýcý
	{
		SetVelocity(velocity);
		SetActorRotation(180.f);
		SetExplosionType(ExplosionType::Medium);
		SetScoreAmt(10);  // Vanguard = 10 points
	}

	Vanguard::~Vanguard()
	{
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

	// Vanguard reward table: Weak enemy, frequent health drops
	List<WeightedReward> Vanguard::GetDefaultRewards()
	{
		return {
			{CreateRewardHealth, 0.15f},        
			{CreateRewardThreeWayShooter, 0.1f}, 
			{CreateRewardFrontalWiper, 0.05f},     
			{CreateRewardLife, 0.02f}

		};
	}
}