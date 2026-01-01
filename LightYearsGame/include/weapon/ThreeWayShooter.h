#pragma once

#include "weapon/BulletShooter.h"
#include <SFML/Graphics.hpp>

namespace ly
{
	class ThreeWayShooter : public Shooter
	{
	public:
		ThreeWayShooter(Actor* owner,
			const BulletDefinition& bulletDef,
			float cooldownTime = 0.5f,
			const sf::Vector2f& localPositionOffset = { 0.f,0.f },
			float localRotationOffset = 0.f);

		virtual void IncrementLevel(int amt = 1) override;
		virtual void SetCurrentLevel(int level) override;
	private:
		unique_ptr<BulletShooter> mLeftShooter;
		unique_ptr<BulletShooter> mMidShooter;
		unique_ptr<BulletShooter> mRightShooter;
		unique_ptr<BulletShooter> mLeftTopLevelShooter;
		unique_ptr<BulletShooter> mRightTopLevelShooter;

		virtual void ShootImp() override;
	};
}