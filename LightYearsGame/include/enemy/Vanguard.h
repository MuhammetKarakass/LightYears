#pragma once
#include "enemy/EnemySpaceShip.h"

namespace ly
{
	class BulletShooter;
	class Vanguard : public EnemySpaceShip
	{
	public:
		Vanguard(World* ownningWorld, const std::string& texturePath = "SpaceShooterRedux/PNG/Enemies/enemyBlack1.png",const sf::Vector2f& velocity={0.f,150.f});

        ~Vanguard();

		virtual void Tick(float deltaTime) override;

		virtual void Shoot() override;


	private:
		unique_ptr<BulletShooter> mShooter;

		
	};
}