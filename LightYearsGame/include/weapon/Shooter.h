#pragma once
#include <framework/Core.h>

namespace ly
{
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

		virtual void IncrementLevel(int amt = 1);

	protected:
		Shooter(Actor* owner);

	private:
		Actor* mOwner;
		virtual void ShootImp() = 0;

		int mCurrentLevel;
		int mMaxLevel;
		float mCooldownMultiplier;
	};
}