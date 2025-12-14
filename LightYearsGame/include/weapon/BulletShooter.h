#pragma once

#include "weapon/Shooter.h"
#include <SFML/System.hpp>

class Actor;
namespace ly
{
	class BulletShooter: public Shooter
	{
	public:
		BulletShooter(Actor* owner,const std::string& texturePath,
			float cooldownTime = 0.5f,
			const sf::Vector2f& localPositionOffset = {0.f,0.f},
			float localRotationOffset= 0.f,
			float bulletDamage = 10.f,
			float bulletSpeed = 600.f
		);

		virtual bool IsOnCooldown() const override;

		void SetBulletSpeed(float speed);
		float GetBulletSpeed() const { return mBulletSpeed; };
		void SetBulletDamage(float damage);
		float GetBulletDamage() const { return mBulletDamage; };
		void SetTexturePath(const std::string& texturePath) { mTexturePath = texturePath; }
		void SetCooldownTime(float cooldownTime);
		float GetCooldownTime() const { return mCooldownTime; };

		virtual void IncrementLevel(int amt = 1) override;

	protected:
		// Allow derived classes to restart cooldown
		void RestartCooldown();

	private:
		virtual void ShootImp() override;
		sf::Clock mCooldownClock;
		float mCooldownTime;
		std::string mTexturePath;
		
		float mBulletSpeed;
		float mBulletDamage;
	};
}