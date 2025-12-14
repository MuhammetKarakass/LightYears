#include "weapon/ThreeWayShooter.h"

namespace ly
{
	ThreeWayShooter::ThreeWayShooter(Actor* owner, const std::string& texturePath,
		float cooldownTime,
		const sf::Vector2f& localPositionOffset,
		float localRotationOffset,
		float damage,
		float speed) :
		Shooter{owner},
		mLeftShooter{ new BulletShooter{owner, texturePath,cooldownTime ,localPositionOffset + sf::Vector2f{-20.f,-5.f},localRotationOffset-30.f,damage,speed}},
		mMidShooter{ new BulletShooter{owner, texturePath,cooldownTime ,localPositionOffset,localRotationOffset,damage,speed} },
		mRightShooter{ new BulletShooter{owner, texturePath,cooldownTime ,localPositionOffset + sf::Vector2f{20.f,-5.f},localRotationOffset+30.f,damage,speed} },
		mLeftTopLevelShooter{ new BulletShooter{owner, texturePath,cooldownTime ,localPositionOffset + sf::Vector2f{-10.f,-5.f},localRotationOffset-15.f,damage,speed} },
		mRightTopLevelShooter{ new BulletShooter{owner, texturePath,cooldownTime ,localPositionOffset + sf::Vector2f{+10.f,-5.f},localRotationOffset+15.f,damage,speed}}
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
	void ThreeWayShooter::SetCurrentLevel(int level)
	{
		Shooter::SetCurrentLevel(level);
		mLeftShooter->SetCurrentLevel(level);
		mMidShooter->SetCurrentLevel(level);
		mRightShooter->SetCurrentLevel(level);
		mLeftTopLevelShooter->SetCurrentLevel(level);
		mRightTopLevelShooter->SetCurrentLevel(level);
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