#pragma once

#include <sfml/Graphics.hpp>
#include "weapon/BulletShooter.h"

namespace ly
{
	class FrontalWiper : public Shooter
	{
	public:
		FrontalWiper(Actor* owner, const std::string& texturePath,
			float cooldownTime = 0.25f,
			const sf::Vector2f& localPositionOffset = { 0.f,0.f },
			float localRotationOffset=0.f,
			float damage = 10.f,
			float speed = 600.f,
			float width= 20.f);

		virtual void IncrementLevel(int amt = 1) override;
		virtual void SetCurrentLevel(int level) override;
	private:
		float mWidth;
		unique_ptr<BulletShooter> mShooter1;
		unique_ptr<BulletShooter> mShooter2;
		unique_ptr<BulletShooter> mShooter3;
		unique_ptr<BulletShooter> mShooter4;
		unique_ptr<BulletShooter> mShooter5;
		unique_ptr<BulletShooter> mShooter6;
		virtual void ShootImp() override;
	};
}