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

	int AbilityRuntimeState::GetChargeCapacity(int maxCharges) const
	{
		return maxCharges > 0 ? maxCharges + mChargeBonus : maxCharges;
	}

	bool AbilityRuntimeState::CanActivate(int maxCharges) const
	{
		if (mIsActive)
		{
			return false;
		}
		if (maxCharges <= 0)
		{
			return !IsOnCooldown();
		}
		if (mCharges <= 0)
		{
			return false;
		}
		// Single-charge abilities keep the original cooldown gate. Multi-charge
		// abilities spend available charges without waiting for the recharge
		// timer, but keep a short gap between uses.
		if (GetChargeCapacity(maxCharges) > 1)
		{
			return mUseIntervalRemaining <= 0.f;
		}
		return !IsOnCooldown();
	}

	void AbilityRuntimeState::BeginActivation(float activeDuration, int maxCharges)
	{
		BeginActivation(activeDuration, maxCharges, false, 0.f);
	}

	void AbilityRuntimeState::BeginActivation(
		float activeDuration,
		int maxCharges,
		bool deferActiveDuration,
		float chargeUseInterval
	)
	{
		if (maxCharges > 0 && mCharges > 0)
		{
			--mCharges;
		}
		mUseIntervalRemaining = maxCharges > 0 && GetChargeCapacity(maxCharges) > 1
			? std::max(0.f, chargeUseInterval)
			: 0.f;
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

	void AbilityRuntimeState::StartCooldown(float cooldownDuration, int maxCharges)
	{
		if (maxCharges <= 0)
		{
			mCooldownRemaining = std::max(0.f, cooldownDuration);
			return;
		}
		// Starts the single recharge timer only when a charge is missing and no
		// timer is already running for an earlier spend.
		if (mCharges < GetChargeCapacity(maxCharges) && mCooldownRemaining <= 0.f)
		{
			mCooldownRemaining = std::max(0.f, cooldownDuration);
		}
	}

	void AbilityRuntimeState::EndActivation(float cooldownDuration, int maxCharges)
	{
		mIsActive = false;
		mActiveTimeElapsed = 0.f;
		mActiveTimeRemaining = 0.f;
		mActiveDurationDeferred = false;
		if (maxCharges <= 0)
		{
			mCooldownRemaining = cooldownDuration;
			return;
		}

		const int capacity = GetChargeCapacity(maxCharges);
		if (cooldownDuration <= 0.f)
		{
			// A zero cooldown returns the spent charge at once. A timer that is
			// already running for other missing charges keeps its time.
			mCharges = std::min(capacity, mCharges + 1);
			SyncRecharge(0.f, capacity);
			return;
		}
		SyncRecharge(cooldownDuration, capacity);
	}

	// Keeps the recharge invariant: a full pool has no timer, and a missing charge
	// always has a running timer (or is restored at once when no cycle can run).
	void AbilityRuntimeState::SyncRecharge(float cooldownDuration, int capacity)
	{
		if (mCharges >= capacity)
		{
			mCharges = capacity;
			mCooldownRemaining = 0.f;
			return;
		}
		if (mCooldownRemaining > 0.f)
		{
			return;
		}
		if (cooldownDuration > 0.f)
		{
			mCooldownRemaining = cooldownDuration;
			return;
		}
		mCharges = capacity;
	}

	// Shared by every path that drains the recharge timer (tick, cooldown
	// reduction, refunds). Each completed cycle restores exactly one charge.
	void AbilityRuntimeState::AdvanceRecharge(float amount, float cooldownDuration, int maxCharges)
	{
		if (maxCharges <= 0)
		{
			mCooldownRemaining = std::max(0.f, mCooldownRemaining - amount);
			return;
		}

		const int capacity = GetChargeCapacity(maxCharges);
		float remaining = amount;
		while (mCooldownRemaining > 0.f && remaining >= mCooldownRemaining)
		{
			remaining -= mCooldownRemaining;
			mCooldownRemaining = 0.f;
			mCharges = std::min(capacity, mCharges + 1);
			SyncRecharge(cooldownDuration, capacity);
		}
		if (mCooldownRemaining > 0.f)
		{
			mCooldownRemaining = std::max(0.f, mCooldownRemaining - remaining);
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
		// A timer longer than the current duration is clamped down to it. The
		// clamped time still counts as elapsed, so the clamp cannot strand a charge.
		const float clampedTime = std::max(0.f, mCooldownRemaining - cooldownDuration);
		AdvanceRecharge(deltaTime + clampedTime, cooldownDuration, maxCharges);
		return previousCooldownRemaining != mCooldownRemaining || previousCharges != mCharges;
	}

	void AbilityRuntimeState::TickUseInterval(float deltaTime)
	{
		if (mUseIntervalRemaining <= 0.f)
		{
			return;
		}
		mUseIntervalRemaining = std::max(0.f, mUseIntervalRemaining - std::max(0.f, deltaTime));
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

	bool AbilityRuntimeState::ReduceCooldown(float amount, float cooldownDuration, int maxCharges)
	{
		if (amount <= 0.f || mCooldownRemaining <= 0.f)
		{
			return false;
		}
		const float previousCooldownRemaining = mCooldownRemaining;
		const int previousCharges = mCharges;
		AdvanceRecharge(amount, cooldownDuration, maxCharges);
		return previousCooldownRemaining != mCooldownRemaining || previousCharges != mCharges;
	}

	bool AbilityRuntimeState::SetChargeBonus(int bonus, float cooldownDuration, int maxCharges)
	{
		// Unlimited abilities have no charge capacity to extend.
		if (maxCharges <= 0)
		{
			return false;
		}

		const int requested = std::max(0, bonus);
		if (requested == mChargeBonus)
		{
			return false;
		}

		const int previousCapacity = GetChargeCapacity(maxCharges);
		mChargeBonus = requested;
		const int capacity = GetChargeCapacity(maxCharges);
		if (capacity > previousCapacity)
		{
			// Charges added by a bonus arrive full instead of waiting on the timer.
			mCharges += capacity - previousCapacity;
		}
		// Removing capacity clips surplus charges; the timer follows the new state.
		mCharges = std::min(mCharges, capacity);
		if (capacity <= 1)
		{
			mUseIntervalRemaining = 0.f;
		}
		SyncRecharge(cooldownDuration, capacity);
		return true;
	}

	int AbilityRuntimeState::GrantCharges(int amount, float cooldownDuration, int maxCharges)
	{
		// Unlimited abilities have no pool to fill, and a non-positive grant changes nothing.
		if (maxCharges <= 0 || amount <= 0)
		{
			return 0;
		}

		// A grant only fills missing charges. It never raises capacity, and the running recharge timer keeps its time.
		const int capacity = GetChargeCapacity(maxCharges);
		const int granted = std::min(amount, std::max(0, capacity - mCharges));
		if (granted <= 0)
		{
			return 0;
		}
		mCharges += granted;
		SyncRecharge(cooldownDuration, capacity);
		return granted;
	}
}
