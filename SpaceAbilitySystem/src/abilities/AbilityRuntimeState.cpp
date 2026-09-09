#include "abilities/AbilityRuntimeState.h"

#include <algorithm>

namespace sas
{
	AbilityRuntimeState::AbilityRuntimeState(int initialCharges)
		: mCharges{ initialCharges }
	{
	}

	int AbilityRuntimeState::ClampLevel(int requestedLevel, int maxLevel)
	{
		return std::clamp(requestedLevel, 1, std::max(1, maxLevel));
	}

	bool AbilityRuntimeState::SetLevel(int requestedLevel, int maxLevel)
	{
		const int newLevel = ClampLevel(requestedLevel, maxLevel);
		if (newLevel == mLevel)
		{
			return false;
		}
		mLevel = newLevel;
		return true;
	}

	bool AbilityRuntimeState::CanActivate(int maxCharges) const
	{
		return !mIsActive &&
			!IsOnCooldown() &&
			(maxCharges <= 0 || mCharges > 0);
	}

	void AbilityRuntimeState::BeginActivation(float activeDuration, int maxCharges)
	{
		BeginActivation(activeDuration, maxCharges, false);
	}

	void AbilityRuntimeState::BeginActivation(
		float activeDuration,
		int maxCharges,
		bool deferActiveDuration
	)
	{
		if (maxCharges > 0 && mCharges > 0)
		{
			--mCharges;
		}
		mIsActive = true;
		mActiveTimeElapsed = 0.f;
		mActiveDurationDeferred = deferActiveDuration;
		mActiveTimeRemaining = deferActiveDuration
			? 0.f
			: activeDuration;
	}

	bool AbilityRuntimeState::StartDeferredActiveDuration(float activeDuration)
	{
		if (!mIsActive || !mActiveDurationDeferred)
		{
			return false;
		}

		mActiveDurationDeferred = false;
		mActiveTimeRemaining = std::max(0.f, activeDuration);
		return true;
	}

	void AbilityRuntimeState::TickActiveTime(float deltaTime)
	{
		if (!mIsActive || deltaTime <= 0.f)
		{
			return;
		}
		mActiveTimeElapsed += deltaTime;
	}

	bool AbilityRuntimeState::RefreshActiveDuration(float activeDuration)
	{
		if (!mIsActive || activeDuration < 0.f)
		{
			return false;
		}

		if (mActiveTimeRemaining == activeDuration)
		{
			return false;
		}

		mActiveTimeRemaining = activeDuration;
		return true;
	}

	void AbilityRuntimeState::StartCooldown(float cooldownDuration)
	{
		mCooldownRemaining = std::max(0.f, cooldownDuration);
	}

	void AbilityRuntimeState::EndActivation(float cooldownDuration, int maxCharges)
	{
		mIsActive = false;
		mActiveTimeElapsed = 0.f;
		mActiveTimeRemaining = 0.f;
		mActiveDurationDeferred = false;
		mCooldownRemaining = cooldownDuration;
		if (mCooldownRemaining <= 0.f && maxCharges > 0)
		{
			mCharges = maxCharges;
		}
	}

	bool AbilityRuntimeState::TickCooldown(
		float deltaTime,
		float cooldownDuration,
		int maxCharges
	)
	{
		if (mCooldownRemaining <= 0.f)
		{
			return false;
		}

		const float previousCooldownRemaining = mCooldownRemaining;
		const int previousCharges = mCharges;
		mCooldownRemaining = std::min(mCooldownRemaining, cooldownDuration);
		mCooldownRemaining = std::max(0.f, mCooldownRemaining - deltaTime);
		if (mCooldownRemaining <= 0.f && maxCharges > 0)
		{
			mCharges = maxCharges;
		}
		return previousCooldownRemaining != mCooldownRemaining || previousCharges != mCharges;
	}

	bool AbilityRuntimeState::TickActiveDuration(float deltaTime)
	{
		if (mActiveTimeRemaining <= 0.f)
		{
			return false;
		}
		const float previousActiveTimeRemaining = mActiveTimeRemaining;
		mActiveTimeRemaining = std::max(0.f, mActiveTimeRemaining - deltaTime);
		return previousActiveTimeRemaining != mActiveTimeRemaining;
	}

	bool AbilityRuntimeState::ReduceCooldown(float amount)
	{
		if (amount <= 0.f || mCooldownRemaining <= 0.f)
		{
			return false;
		}
		const float previousCooldownRemaining = mCooldownRemaining;
		mCooldownRemaining = std::max(0.f, mCooldownRemaining - amount);
		return previousCooldownRemaining != mCooldownRemaining;
	}
}
