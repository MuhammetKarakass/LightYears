#include "enemy/Hexagon.h"
#include "weapon/BulletShooter.h"

namespace ly
{
	Hexagon::Hexagon(World* owningWorld, const std::string& texturePath, const sf::Vector2f& velocity)
		: EnemySpaceShip(owningWorld, texturePath, 75.f, GetDefaultRewards())
	{
		SetVelocity(velocity);
		SetActorRotation(180.f);
		SetExplosionType(ExplosionType::Heavy);
		SetScoreAmt(30);  // Hexagon = 30 points (strong)
		SetupShooters();
	}

	void Hexagon::Tick(float deltaTime)
	{
		EnemySpaceShip::Tick(deltaTime);
		Shoot();
	}

	void Hexagon::Shoot()
	{
		for (auto& shooter : mShooters)
		{
			if (shooter)
			{
				shooter->Shoot();
			}
		}
	}

	void Hexagon::SetupShooters()
	{
		// 6 yönlü ateþ sistemi - Altýgen þeklinde (60 derece aralýklarla)
		mShooters.push_back(std::make_unique<BulletShooter>(this, "SpaceShooterRedux/PNG/Lasers/laserRed01.png", 1.f, sf::Vector2f{0.f, 50.f}, 0.f));
		mShooters.push_back(std::make_unique<BulletShooter>(this, "SpaceShooterRedux/PNG/Lasers/laserRed01.png", 1.f, sf::Vector2f{ 0.f, -50.f }, 180.f));

		mShooters.push_back(std::make_unique<BulletShooter>(this, "SpaceShooterRedux/PNG/Lasers/laserRed01.png", 2.f, sf::Vector2f{50.f, 50.f}, 45.f));
		mShooters.push_back(std::make_unique<BulletShooter>(this, "SpaceShooterRedux/PNG/Lasers/laserRed01.png", 2.f, sf::Vector2f{-50.f, 50.f}, -45.f));

		mShooters.push_back(std::make_unique<BulletShooter>(this, "SpaceShooterRedux/PNG/Lasers/laserRed01.png", 2.f, sf::Vector2f{ -50.f, -50.f}, -135.f));
		mShooters.push_back(std::make_unique<BulletShooter>(this, "SpaceShooterRedux/PNG/Lasers/laserRed01.png", 2.f, sf::Vector2f{ 50.f, -50.f}, 135.f));

		for (auto& shooter : mShooters) 
		{
			shooter->SetBulletSpeed(400.f);
		}
	}

	List<WeightedReward> Hexagon::GetDefaultRewards()
	{
		return {
			{CreateRewardHealth, 0.3f},
			{CreateRewardThreeWayShooter, 0.15f},
			{CreateRewardFrontalWiper, 0.1f},
			{CreateRewardLife, 0.05f}

		};
	}
}
