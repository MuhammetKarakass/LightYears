#include "weapon/Shooter.h"

namespace ly
{
	void Shooter::IncrementLevel(int amt)
	{
		mCurrentLevel = std::min(mCurrentLevel + amt, mMaxLevel);
		mCooldownMultiplier = 1.f + (float(mCurrentLevel - 1) * 0.2f);
	}

	Shooter::Shooter(Actor* owner)
		: mOwner{ owner },
		mCurrentLevel{ 1 },
		mMaxLevel{ 4 },
		mCooldownMultiplier{ 1.f }
	{
	}
	void Shooter::Shoot()
	{
		if (CanShoot() && !IsOnCooldown())
		{
			ShootImp();
		}
	}
}