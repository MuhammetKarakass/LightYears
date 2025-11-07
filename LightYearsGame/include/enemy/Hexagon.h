#pragma once

#include "enemy/EnemySpaceShip.h"

namespace ly 
{
	class BulletShooter;

	class Hexagon : public EnemySpaceShip
	{
	public:

		Hexagon(World* owningWorld, const std::string& texturePath = "SpaceShooterRedux/PNG/Enemies/enemyBlack4.png", const sf::Vector2f& velocity = sf::Vector2{ 0.f,100.f });

		virtual void Tick(float deltaTime) override;

	private:
		virtual void Shoot() override;
		void SetupShooters();

		List<unique_ptr<BulletShooter>> mShooters;
	};

}