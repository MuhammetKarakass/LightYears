#pragma once

#include "enemy/EnemySpaceShip.h"

namespace ly
{
	class BulletShooter;
	class TwinBlade : public EnemySpaceShip
	{
	public:
		TwinBlade(World* owningWorld, const std::string& texturePath = "SpaceShooterRedux/PNG/Enemies/enemyBlack3.png", const sf::Vector2f& velocity= {0.f,80.f});

		~TwinBlade();

		virtual void Tick(float deltaTime) override;

		virtual void Shoot() override;

	private:
		unique_ptr<BulletShooter> mShooterLeft;
		unique_ptr<BulletShooter> mShooterRight;
	};
}