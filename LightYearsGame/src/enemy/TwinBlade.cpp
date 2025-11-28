#include "enemy/TwinBlade.h"
#include "weapon/BulletShooter.h"
#include <framework/World.h>

namespace ly
{
	TwinBlade::TwinBlade(World* owningWorld, const std::string& texturePath,const sf::Vector2f& velocity)
		: EnemySpaceShip(owningWorld, texturePath, 75.f, GetDefaultRewards()),
		mShooterLeft( new BulletShooter(this,"SpaceShooterRedux/PNG/Lasers/laserRed01.png", 1.f, {-20.f, 40.f}) ),
		mShooterRight( new BulletShooter(this,"SpaceShooterRedux/PNG/Lasers/laserRed01.png", 1.f, {20.f, 40.f}) )
	{
		SetVelocity(velocity);
		SetActorRotation(180.f);
		SetExplosionType(ExplosionType::Heavy);
		SetScoreAmt(20);  // TwinBlade = 20 points (stronger)
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

	// TwinBlade reward table: Medium enemy, balanced drops
	List<WeightedReward> TwinBlade::GetDefaultRewards()
	{
		return {
			{CreateRewardHealth, 0.20f},
			{CreateRewardThreeWayShooter, 0.1f},
			{CreateRewardFrontalWiper, 0.75f},
			{CreateRewardLife, 0.03f}
		};
	}
}