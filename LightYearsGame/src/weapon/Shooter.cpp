#include "weapon/Shooter.h"
#include "framework/AudioManager.h"
#include "framework/Actor.h"

namespace ly
{
	void Shooter::SetCooldownMultiplier(float mult)
	{
		mCooldownMultiplier = 1.f + (float(mult - 1) * 0.15f);

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

	Shooter::Shooter(Actor* owner, ShootSoundMode soundMode)
		: mOwner{ owner },
		mSoundMode{ soundMode },
		mCurrentLevel{ 1 },
		mMaxLevel{ 6 },
		mCooldownMultiplier{ 1.f },
		mLocalPositionOffset{ 0.f, 0.f },
		mLocalRotationOffset{ 0.f },
		mShootSoundPath{ "" },
		mShootSoundVolume{ 100.0f },
		mShootSoundPitch{ 1.0f },
		mSoundClock{},
		mMinSoundInterval{ 0.05f }
	{
	}
	void Shooter::PlayShootSound()
	{
		if (mSoundMode == ShootSoundMode::None)
			return;

		if (mShootSoundPath.empty())
			return;

		if (mSoundClock.getElapsedTime().asSeconds() < mMinSoundInterval)
			return;

		mSoundClock.restart();
		AudioManager::GetAudioManager().PlaySound(mShootSoundPath, AudioType::SFX_World, mShootSoundVolume, mShootSoundPitch);
	}
	void Shooter::SetShootSoundProps(const std::string& soundPath, float volume, float pitch)
	{
		mShootSoundPath = soundPath;
		mShootSoundVolume = volume;
		mShootSoundPitch = pitch;
	}
	void Shooter::Shoot()
	{
		if (CanShoot() && !IsOnCooldown())
		{
			ShootImp();
		}
	}
}