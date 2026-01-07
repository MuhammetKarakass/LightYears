#pragma once
#include <framework/Core.h>
#include <SFML/Graphics.hpp>
#include "gameConfigs/GameplayConfig.h"
#include "gameConfigs/GameplayStructs.h"
#include "engineConfigs/EngineStructs.h"

namespace ly
{
	enum class ShootSoundMode
	{
		None,
		Single,
		Composite
	};

	class Actor;
	class Shooter
	{
	public:
		void Shoot();

		virtual bool CanShoot() const { return true; }
		virtual bool IsOnCooldown() const { return false; }

		Actor* GetOwner() const { return mOwner; }

		int GetCurrentLevel() const { return mCurrentLevel; }
		int GetMaxLevel() const { return mMaxLevel; }
		float GetCooldownMultiplier() const { return mCooldownMultiplier; }
		void SetCooldownMultiplier(float mult);

		virtual void IncrementLevel(int amt = 1);
		virtual void SetCurrentLevel(int level);

		void SetLocalPositionOffset(const sf::Vector2f& offset);
		sf::Vector2f GetLocalPositionOffset() const { return mLocalPositionOffset; };
		void SetLocalRotationOffset(float rotation);
		float GetLocalRotationOffset() const { return mLocalRotationOffset; };

	protected:
		Shooter(Actor* owner, ShootSoundMode soundMode = ShootSoundMode::Single);

		void PlayShootSound();
		void SetShootSoundProps(const std::string& soundPath, float volume, float pitch);
		void SetMinSoundInterval(float interval) { mMinSoundInterval = interval; }

		ShootSoundMode mSoundMode;

	private:
		Actor* mOwner;
		virtual void ShootImp() = 0;

		int mCurrentLevel;
		int mMaxLevel;
		float mCooldownMultiplier;
		sf::Vector2f mLocalPositionOffset;
		float mLocalRotationOffset;

		std::string mShootSoundPath;
		float mShootSoundVolume;
		float mShootSoundPitch;

		sf::Clock mSoundClock;
		float mMinSoundInterval;
	};
}