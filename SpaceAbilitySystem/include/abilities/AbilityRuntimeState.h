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
		bool IsOnCooldown() const { return mCooldownRemaining > 0.f; }
		float GetCooldownRemaining() const { return mCooldownRemaining; }
		float GetActiveTimeRemaining() const { return mActiveTimeRemaining; }
		int GetCharges() const { return mCharges; }
		int GetLevel() const { return mLevel; }

		static int ClampLevel(int requestedLevel, int maxLevel);
		bool SetLevel(int requestedLevel, int maxLevel);
		bool CanActivate(int maxCharges) const;
		void BeginActivation(float activeDuration, int maxCharges);
		bool RefreshActiveDuration(float activeDuration);
		void EndActivation(float cooldownDuration, int maxCharges);
		bool TickCooldown(float deltaTime, float cooldownDuration, int maxCharges);
		bool TickActiveDuration(float deltaTime);
		bool ReduceCooldown(float amount);

	private:
		int mLevel = 1;
		bool mInputHeld = false;
		bool mWasInputHeld = false;
		bool mIsActive = false;
		float mCooldownRemaining = 0.f;
		float mActiveTimeRemaining = 0.f;
		int mCharges = 0;
	};
}
