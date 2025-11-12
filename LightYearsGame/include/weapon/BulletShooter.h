#pragma once

#include "weapon/Shooter.h"
#include <SFML/System.hpp>

class Actor;
namespace ly
{
	class BulletShooter: public Shooter
	{
	public:
		BulletShooter(Actor* owner,const std::string& texturePath, float cooldownTime = 0.5f, const sf::Vector2f& localPositionOffset = {0.f,0.f}, float localRotationOffset= 0.f);

		virtual bool IsOnCooldown() const override;
		void SetBulletSpeed(float speed);
		void SetTexturePath(const std::string& texturePath) { mTexturePath = texturePath; }

		virtual void IncrementLevel(int amt = 1) override;

	private:
		virtual void ShootImp() override;
		sf::Clock mCooldownClock;
		float mCooldownTime;
		std::string mTexturePath;
		sf::Vector2f mLocalPositionOffset;
		float mBulletSpeed;
		float mLocalRotationOffset;
	};
}