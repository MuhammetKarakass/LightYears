#pragma once

#include "weapon/Shooter.h"
#include <SFML/System.hpp>
#include "../gameConfigs/GameplayStructs.h"

class Actor;
namespace ly
{
	class BulletShooter: public Shooter
	{
	public:
		BulletShooter(Actor* owner,
			const BulletDefinition& bulletDef,
			float cooldownTime = 0.5f,
			const sf::Vector2f& localPositionOffset = {0.f,0.f},
			float localRotationOffset= 0.f
		);

		virtual bool IsOnCooldown() const override;

		void SetBulletSpeed(float speed);
		float GetBulletSpeed() const { return mBulletDef.speed; };
		void SetBulletDamage(float damage);
		float GetBulletDamage() const { return mBulletDef.damage; };
		void SetCooldownTime(float cooldownTime);
		float GetCooldownTime() const { return mCooldownTime; };

		void SetBulletDefinition(const BulletDefinition& bulletDef) { mBulletDef = bulletDef; }
		virtual void IncrementLevel(int amt = 1) override;

	protected:
		// Allow derived classes to restart cooldown
		void RestartCooldown();

	private:
		virtual void ShootImp() override;
		sf::Clock mCooldownClock;
		float mCooldownTime;

		BulletDefinition mBulletDef;

	};
}