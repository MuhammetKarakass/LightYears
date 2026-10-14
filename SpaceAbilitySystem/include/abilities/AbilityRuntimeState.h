#pragma once

namespace sas
{
	class AbilityRuntimeState
	{
	public:
		explicit AbilityRuntimeState(int initialCharges = 0);

		void SetInputHeld(bool inputHeld) { mInputHeld = inputHeld; }
		bool IsInputHeld() const { return mInputHeld; }
		bool WasInputHeld() const { return mWasInputHeld; }
		bool IsPressedThisFrame() const { return mInputHeld && !mWasInputHeld; }
		void CommitInputFrame() { mWasInputHeld = mInputHeld; }

		bool IsActive() const { return mIsActive; }
		bool IsActiveDurationDeferred() const { return mActiveDurationDeferred; }
		// For multi-charge abilities this is the single recharge timer: it runs while a charge is missing.
		bool IsOnCooldown() const { return mCooldownRemaining > 0.f; }
		float GetCooldownRemaining() const { return mCooldownRemaining; }
		float GetUseIntervalRemaining() const { return mUseIntervalRemaining; }
		float GetActiveTimeElapsed() const { return mActiveTimeElapsed; }
		float GetActiveTimeRemaining() const { return mActiveTimeRemaining; }
		int GetCharges() const { return mCharges; }
		int GetChargeBonus() const { return mChargeBonus; }
		// Effective capacity: definition maxCharges plus the runtime bonus. Unlimited (<= 0) stays unlimited.
		int GetChargeCapacity(int maxCharges) const;
		int GetLevel() const { return mLevel; }

		static int ClampLevel(int requestedLevel, int maxLevel);
		bool SetLevel(int requestedLevel, int maxLevel);
		bool CanActivate(int maxCharges) const;
		void BeginActivation(float activeDuration, int maxCharges);
		void BeginActivation(
			float activeDuration,
			int maxCharges,
			bool deferActiveDuration,
			float chargeUseInterval = 0.f
		);
		bool StartDeferredActiveDuration(float activeDuration);
		void TickActiveTime(float deltaTime);
		bool RefreshActiveDuration(float activeDuration);
		void StartCooldown(float cooldownDuration, int maxCharges);
		void EndActivation(float cooldownDuration, int maxCharges);
		bool TickCooldown(float deltaTime, float cooldownDuration, int maxCharges);
		void TickUseInterval(float deltaTime);
		bool TickActiveDuration(float deltaTime);
		bool ReduceCooldown(float amount, float cooldownDuration, int maxCharges);
		bool SetChargeBonus(int bonus, float cooldownDuration, int maxCharges);
		// Fills missing charges up to the effective capacity; returns how many were added. Never raises capacity.
		int GrantCharges(int amount, float cooldownDuration, int maxCharges);

	private:
		void AdvanceRecharge(float amount, float cooldownDuration, int maxCharges);
		void SyncRecharge(float cooldownDuration, int capacity);

		int mLevel = 1;
		bool mInputHeld = false;
		bool mWasInputHeld = false;
		bool mIsActive = false;
		bool mActiveDurationDeferred = false;
		float mCooldownRemaining = 0.f;
		float mUseIntervalRemaining = 0.f;
		float mActiveTimeElapsed = 0.f;
		float mActiveTimeRemaining = 0.f;
		int mCharges = 0;
		int mChargeBonus = 0;
	};
}
