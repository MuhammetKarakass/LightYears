#include "enemy/Hexagon.h"
#include "weapon/BulletShooter.h"

namespace ly
{
	Hexagon::Hexagon(World* owningWorld, const std::string& texturePath, const sf::Vector2f& velocity)
		: EnemySpaceShip(owningWorld, texturePath)
	{
		SetVelocity(velocity);
		SetActorRotation(180.f);
		SetExplosionType(ExplosionType::Heavy);
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
		// Namlu konumu geminin merkezine yakýn, rotasyon ile yön belirlenir
		
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
}
