#include "weapon/ThreeWayShooter.h"

namespace ly
{
	ThreeWayShooter::ThreeWayShooter(Actor* owner,	
		const BulletDefinition& bulletDef,
		float cooldownTime,
		const sf::Vector2f& localPositionOffset,
		float localRotationOffset) :
		Shooter{owner},
		mLeftShooter{ new BulletShooter{owner, bulletDef,cooldownTime ,localPositionOffset + sf::Vector2f{-20.f,-5.f},localRotationOffset-30.f}},
		mMidShooter{ new BulletShooter{owner, bulletDef,cooldownTime ,localPositionOffset,localRotationOffset} },
		mRightShooter{ new BulletShooter{owner, bulletDef,cooldownTime ,localPositionOffset + sf::Vector2f{20.f,-5.f},localRotationOffset+30.f}},
		mLeftTopLevelShooter{ new BulletShooter{owner, bulletDef,cooldownTime ,localPositionOffset + sf::Vector2f{-10.f,-5.f},localRotationOffset-15.f}},
		mRightTopLevelShooter{ new BulletShooter{owner, bulletDef,cooldownTime ,localPositionOffset + sf::Vector2f{+10.f,-5.f},localRotationOffset+15.f}}
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