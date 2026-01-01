#include "weapon/bulletShooter/MultiShooter.h"
#include "weapon/BulletShooter.h"
#include "weapon/ThreeWayShooter.h"
#include "weapon/FrontalWiper.h"
#include "framework/Actor.h"
#include "framework/MathUtility.h"

namespace ly
{
	MultiShooter::MultiShooter(Actor* owner,
		std::pair<float, float> cooldownTime,
		const List<ShooterInitializer>& shooterConfigs,
		FireMode fireMode,
		float delayBetweenShots)
		: Shooter{ owner },
		mFireMode{ fireMode },
		mCooldownTimeRange{ cooldownTime },
		mDelayBetweenShots{ delayBetweenShots },
		mIsShooting{ false },
		mCurrentShooterIndex{ 0 },
		mIsReversed{ false },
		mAlternatingReturn{ false },
		mCooldownTime{ RandRange(cooldownTime.first, cooldownTime.second) }
	{
		for (const auto& config : shooterConfigs)
		{
			mShooters.push_back(config(owner));
		}

		mIsReversed = RandRange(0.f, 1.f) < 0.5f;
	}

	MultiShooter::~MultiShooter()
	{
		TimerManager::GetGameTimerManager().ClearTimer(mSequentialTimerHandle);
	}

	void MultiShooter::IncrementLevel(int amt)
	{
		Shooter::IncrementLevel(amt);
		for (auto& shooter : mShooters)
		{
			shooter->IncrementLevel(amt);
		}
	}

	void MultiShooter::SetCurrentLevel(int level)
	{
		Shooter::SetCurrentLevel(level);
		for (auto& shooter : mShooters)
		{
			shooter->SetCurrentLevel(level);
		}
	}

	bool MultiShooter::IsOnCooldown() const
	{
		LOG("cooldown %f", mCooldownTime);

		if (mCooldownClock.getElapsedTime().asSeconds() > mCooldownTime / GetCooldownMultiplier())
		{
			return false;
		}
		return true;
	}

	void MultiShooter::RestartCooldown()
	{
		mCooldownClock.restart();

		mCooldownTime = RandRange(mCooldownTimeRange.first, mCooldownTimeRange.second);
		
	}

	void MultiShooter::SetFireMode(FireMode mode)
	{
		if (mFireMode == mode)
			return;

		TimerManager::GetGameTimerManager().ClearTimer(mSequentialTimerHandle);
		mSequentialTimerHandle = TimerHandle();

		mFireMode = mode;
		mIsShooting = false;
		mCurrentShooterIndex = 0;

	}

	

	void MultiShooter::AddShooter(const ShooterInitializer& shooterConfig)
	{
		mShooters.push_back(shooterConfig(GetOwner()));
	}

	void MultiShooter::RemoveShooter(int index)
	{
		if (index >= 0 && index < mShooters.size())
		{
			mShooters.erase(mShooters.begin() + index);
		}
	}

	void MultiShooter::ClearShooters()
	{
		mShooters.clear();
	}

	void MultiShooter::SetAllBulletDamage(float damage)
	{
		for (auto& shooter : mShooters) shooter->SetBulletDamage(damage);
	}

	void MultiShooter::SetAllBulletSpeed(float speed)
	{
		for (auto& shooter : mShooters) shooter->SetBulletSpeed(speed);
	}

	void MultiShooter::SetAllBulletDefinitions(const BulletDefinition& bulletDef)
	{
		for (auto& shooter : mShooters) shooter->SetBulletDefinition(bulletDef);
	}

	void MultiShooter::ShootImp()
	{

		switch (mFireMode)
		{
		case FireMode::Simultaneous:
			ShootSimultaneous();
			break;
		case FireMode::Sequential:
			ShootSequential();
			break;
		case FireMode::Alternating:
			ShootAlternating();
			break;
		}
	}

	void MultiShooter::ShootSimultaneous()
	{
		for (auto& shooter : mShooters)
		{
			shooter->Shoot();
		}
		RestartCooldown();
	}

	void MultiShooter::ShootSequential()
	{

		if (mIsShooting)
		{
			return;
		}

		mIsReversed = RandRange(0.f, 1.f) < 0.5f;

		TimerManager::GetGameTimerManager().ClearTimer(mSequentialTimerHandle);
		mSequentialTimerHandle = TimerHandle();
		mIsShooting = true;
		mCurrentShooterIndex = 0;

		if (!mShooters.empty())
		{
			int firstIndex = mIsReversed ? (mShooters.size() - 1) : 0;
			mShooters[firstIndex]->Shoot();
			mCurrentShooterIndex = 1;
		}

		if (mCurrentShooterIndex < mShooters.size())
		{
			mSequentialTimerHandle = TimerManager::GetGameTimerManager().
				SetTimer(GetOwner()->GetWeakPtr(),
					[this]()
					{
						OnSequentialTimerTick();
					}, mDelayBetweenShots, true);

		}
		else
		{
			OnPatternComplete();
		}
	}

	void MultiShooter::ShootAlternating()
	{

		if (mIsShooting)
		{
			return;
		}

		mIsReversed = RandRange(0.f, 1.f) < 0.5f;


		TimerManager::GetGameTimerManager().ClearTimer(mSequentialTimerHandle);
		mSequentialTimerHandle = TimerHandle();
		mIsShooting = true;
		mCurrentShooterIndex = 0;

		mAlternatingReturn = false;

		if (!mShooters.empty())
		{
			int actualIndex = mIsReversed ? (mShooters.size() - 1) : 0;
			mShooters[actualIndex]->Shoot();
			mCurrentShooterIndex = 1;
		}

		if (mCurrentShooterIndex < mShooters.size())
		{
			mSequentialTimerHandle = TimerManager::GetGameTimerManager().SetTimer(
				GetOwner()->GetWeakPtr(),
				[this]()
				{
					OnSequentialTimerTick();
				},
				mDelayBetweenShots,
				true
			);
		}
		else
		{
			OnPatternComplete();
		}
	}

	void MultiShooter::OnSequentialTimerTick()
	{
		if (!mIsShooting) return;

		if (mCurrentShooterIndex >= mShooters.size())
		{
			OnPatternComplete();
			return;
		}

		int actualIndex = mIsReversed
			? (mShooters.size() - 1 - mCurrentShooterIndex)
			: mCurrentShooterIndex;

		if (actualIndex >= 0 && actualIndex < mShooters.size())
		{
			mShooters[actualIndex]->Shoot();
		}

		mCurrentShooterIndex++;
	}

	void MultiShooter::OnPatternComplete()
	{
		TimerManager::GetGameTimerManager().ClearTimer(mSequentialTimerHandle);
		mSequentialTimerHandle = TimerHandle();

		if (FireMode::Alternating == mFireMode)
		{
			if(!mAlternatingReturn)
			{
				// Start return trip
				mAlternatingReturn = true;
				mIsReversed = !mIsReversed;
				mCurrentShooterIndex = 0;

				mSequentialTimerHandle = TimerManager::GetGameTimerManager().SetTimer(
					GetOwner()->GetWeakPtr(),
					[this]()
					{
						OnSequentialTimerTick();
					},
					mDelayBetweenShots,
					true
				);
				return;
			}
		}

		mIsShooting = false;
		mCurrentShooterIndex = 0;
		RestartCooldown();
	}

}