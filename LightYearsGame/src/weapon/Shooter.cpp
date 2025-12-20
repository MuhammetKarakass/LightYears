#include "weapon/Shooter.h"
#include "framework/AudioManager.h"
#include "framework/Actor.h"

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
	void Shooter::PlayShootSound()
	{
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
			if(GetOwner()->GetCollisionLayer() == CollisionLayer::Player)
				PlayShootSound();
			ShootImp();
		}
	}
}