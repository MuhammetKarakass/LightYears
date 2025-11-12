#include "weapon/ThreeWayShooter.h"

namespace ly
{
	ThreeWayShooter::ThreeWayShooter(Actor* owner, const std::string& texturePath, float cooldownTime, const sf::Vector2f& localPositionOffset) :
		Shooter{owner},
		mLeftShooter{ new BulletShooter{owner, texturePath,cooldownTime ,localPositionOffset + sf::Vector2f{-20.f,-5.f},-30.f}},
		mMidShooter{ new BulletShooter{owner, texturePath,cooldownTime ,localPositionOffset} },
		mRightShooter{ new BulletShooter{owner, texturePath,cooldownTime ,localPositionOffset + sf::Vector2f{20.f,-5.f},+30.f} },
		mLeftTopLevelShooter{ new BulletShooter{owner, texturePath,cooldownTime ,localPositionOffset + sf::Vector2f{-10.f,-5.f},-15.f} },
		mRightTopLevelShooter{ new BulletShooter{owner, texturePath,cooldownTime ,localPositionOffset + sf::Vector2f{+10.f,-5.f},+15.f} }
	{
	}

	void ThreeWayShooter::IncrementLevel(int amt)
	{
		Shooter::IncrementLevel(amt);

		mLeftShooter->IncrementLevel(amt);
		mMidShooter->IncrementLevel(amt);
		mRightShooter->IncrementLevel(amt);
		mLeftTopLevelShooter->IncrementLevel(amt);
		mRightTopLevelShooter->IncrementLevel(amt);

	}
	void ThreeWayShooter::ShootImp()
	{
		mLeftShooter->Shoot();
		mMidShooter->Shoot();
		mRightShooter->Shoot();

		if(GetCurrentLevel()==GetMaxLevel())
		{
			mLeftTopLevelShooter->Shoot();
			mRightTopLevelShooter->Shoot();
		}
	}
}