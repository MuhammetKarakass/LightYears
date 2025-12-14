#include "weapon/Shooter.h"

namespace ly
{
	void Shooter::SetCooldownMultiplier(float mult)
	{
		mCooldownMultiplier = 1.f + (float(mult - 1) * 0.2f);

	}
	void Shooter::IncrementLevel(int amt)
	{
		mCurrentLevel = std::min(mCurrentLevel + amt, mMaxLevel);
		SetCooldownMultiplier(mCurrentLevel);
	}

	void Shooter::SetCurrentLevel(int level)
	{
		mCurrentLevel = level;
		if (level > mMaxLevel)
			mCurrentLevel = mMaxLevel;
		SetCooldownMultiplier(mCurrentLevel);
	}

	void Shooter::SetLocalPositionOffset(const sf::Vector2f& offset)
	{
		mLocalPositionOffset = offset;
	}
	void Shooter::SetLocalRotationOffset(float rotation)
	{
		mLocalRotationOffset = rotation;
	}

	Shooter::Shooter(Actor* owner)
		: mOwner{ owner },
		mCurrentLevel{ 1 },
		mMaxLevel{ 4 },
		mCooldownMultiplier{ 1.f },
		mLocalPositionOffset{ 0.f, 0.f },
		mLocalRotationOffset{ 0.f }
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