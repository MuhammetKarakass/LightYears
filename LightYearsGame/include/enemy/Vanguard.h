#pragma once
#include "enemy/EnemySpaceShip.h"
#include <framework/Core.h>

namespace ly
{
	class BulletShooter;
	class Vanguard : public EnemySpaceShip
	{
	public:
		Vanguard(World* ownningWorld, const std::string& texturePath = "SpaceShooterRedux/PNG/Enemies/enemyBlack1.png",const sf::Vector2f& velocity={0.f,50.f});

        ~Vanguard();

		virtual void Tick(float deltaTime) override;

	private:
		unique_ptr<BulletShooter> mShooter;

		virtual void Shoot() override;
	};
}