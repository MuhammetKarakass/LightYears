#include "weapon/FrontalWiper.h"

namespace ly
{
	FrontalWiper::FrontalWiper(Actor* owner, const std::string& texturePath, float cooldownTime, const sf::Vector2f& localPositionOffset,float localRotationOffset,float damage, float speed, float width)
		: Shooter{ owner },
		mWidth{ width },
		mShooter1{new BulletShooter {owner,texturePath,cooldownTime,{localPositionOffset.x+width, localPositionOffset.y-15.f}, 4.5f,damage,speed}},
		mShooter2{new BulletShooter {owner,texturePath,cooldownTime,{localPositionOffset.x+width/1.5f, localPositionOffset.y-5.f}, 3.f,damage,speed}},
		mShooter3{new BulletShooter {owner,texturePath,cooldownTime,{localPositionOffset.x-width/1.5f, localPositionOffset.y-5.f}, -3.f,damage,speed}},
		mShooter4{new BulletShooter {owner,texturePath,cooldownTime,{localPositionOffset.x-width, localPositionOffset.y-15.f}, -4.5f,damage,speed}},
		mShooter5{new BulletShooter {owner,texturePath,cooldownTime,{localPositionOffset.x+width/2.f, localPositionOffset.y}, 1.5f,damage,speed}},
		mShooter6{new BulletShooter {owner,texturePath,cooldownTime,{localPositionOffset.x-width/2.f, localPositionOffset.y}, -1.5f,damage,speed}}

	{

	}

	void FrontalWiper::IncrementLevel(int amt)
	{
		Shooter::IncrementLevel(amt);

		mShooter1->IncrementLevel(amt);
		mShooter2->IncrementLevel(amt);
		mShooter3->IncrementLevel(amt);
		mShooter4->IncrementLevel(amt);
		mShooter5->IncrementLevel(amt);
		mShooter6->IncrementLevel(amt);
	}

	void FrontalWiper::SetCurrentLevel(int level)
	{
		Shooter::SetCurrentLevel(level);
		mShooter1->SetCurrentLevel(level);
		mShooter2->SetCurrentLevel(level);
		mShooter3->SetCurrentLevel(level);
		mShooter4->SetCurrentLevel(level);
		mShooter5->SetCurrentLevel(level);
		mShooter6->SetCurrentLevel(level);
	}

	void FrontalWiper::ShootImp()
	{
		mShooter1->Shoot();
		mShooter2->Shoot();
		mShooter3->Shoot();
		mShooter4->Shoot();

		if(GetCurrentLevel()==GetMaxLevel())
		{
			mShooter5->Shoot();
			mShooter6->Shoot();
		}
	}
}