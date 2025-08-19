#pragma once

#include "weapon/Shooter.h"
#include <SFML/System.hpp>

class Actor;
namespace ly
{
	class BulletShooter: public Shooter
	{
	public:
		BulletShooter(Actor * owner, float cooldownTime = 0.5f);

		virtual bool IsOnCooldown() const override;

	private:
		virtual void ShootImp() override;
		sf::Clock mCooldownClock;
		float mCooldownTime;
	};
}