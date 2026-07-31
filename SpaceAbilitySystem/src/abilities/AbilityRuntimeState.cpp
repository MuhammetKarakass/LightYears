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
		if (maxCharges > 0 && mCharges > 0)
		{
			--mCharges;
		}
		mIsActive = true;
		mActiveTimeRemaining = activeDuration;
	}

	void AbilityRuntimeState::EndActivation(float cooldownDuration, int maxCharges)
	{
		mIsActive = false;
		mActiveTimeRemaining = 0.f;
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
