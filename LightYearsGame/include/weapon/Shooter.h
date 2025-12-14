#pragma once
#include <framework/Core.h>
#include <SFML/Graphics.hpp>

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
		void SetCooldownMultiplier(float mult);

		virtual void IncrementLevel(int amt = 1);
		virtual void SetCurrentLevel(int level);

		void SetLocalPositionOffset(const sf::Vector2f& offset);
		sf::Vector2f GetLocalPositionOffset() const { return mLocalPositionOffset; };
		void SetLocalRotationOffset(float rotation);
		float GetLocalRotationOffset() const { return mLocalRotationOffset; };

	protected:
		Shooter(Actor* owner);

	private:
		Actor* mOwner;
		virtual void ShootImp() = 0;

		int mCurrentLevel;
		int mMaxLevel;
		float mCooldownMultiplier;
		sf::Vector2f mLocalPositionOffset;
		float mLocalRotationOffset;
	};
}