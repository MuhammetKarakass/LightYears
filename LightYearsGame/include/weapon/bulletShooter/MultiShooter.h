#pragma once

#include "weapon/Shooter.h"
#include <functional>
#include "framework/TimerManager.h"

namespace ly
{
	class ThreeWayShooter;
	class FrontalWiper;
	class BulletShooter;
	using ShooterInitializer = std::function<unique_ptr<BulletShooter>(Actor*)>;

	enum class FireMode
	{
		Simultaneous,
		Sequential,
		Alternating
	};

	class MultiShooter : public Shooter
	{
	public:
		MultiShooter(Actor* owner, 
			std::pair<float, float> cooldownTime,
			const List<ShooterInitializer>& shooterConfigs,
			FireMode fireMode = FireMode::Simultaneous,
			float delayBetweenShots = 0.1f);

		virtual ~MultiShooter();

		virtual void IncrementLevel(int amt = 1) override;
		virtual void SetCurrentLevel(int level) override;

		virtual bool IsOnCooldown() const override;

		void AddShooter(const ShooterInitializer& shooterConfig);
		void RemoveShooter(int index);
		void ClearShooters();

		void SetAllTexture(const std::string& texturePath);
		void SetAllBulletDamage(float damage);
		void SetAllBulletSpeed(float speed);

		int GetShooterCount() const { return mShooters.size(); }

		void SetFireMode(FireMode mode);
		FireMode GetFireMode() const { return mFireMode; }

	private:
		virtual void ShootImp() override;

		void ShootSimultaneous();
		void ShootSequential();
		void ShootAlternating();

		void OnSequentialTimerTick();
		void OnPatternComplete();
		void RestartCooldown();

		List<unique_ptr<BulletShooter>> mShooters;
		FireMode mFireMode;
		float mDelayBetweenShots;

		sf::Clock mCooldownClock;
		std::pair<float, float> mCooldownTimeRange;
		float mCooldownTime;

		bool mIsShooting;
		int mCurrentShooterIndex;

		bool mAlternatingReturn;
		bool mIsReversed;

		TimerHandle mSequentialTimerHandle;
	};
}