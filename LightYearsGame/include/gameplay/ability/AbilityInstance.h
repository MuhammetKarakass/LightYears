#pragma once

#include "gameConfigs/AbilityStructs.h"
#include "gameplay/ability/AbilityExecution.h"

namespace ly
{
	class AbilitySystem;

	struct AbilityRuntimeSnapshot
	{
		AbilityHandle handle;
		const AbilityDefinition* definition = nullptr;
		bool active = false;
		float cooldownRemaining = 0.f;
		float cooldownDuration = 0.f;
		float activeRemaining = 0.f;
		float activeDuration = 0.f;
		int charges = 0;
	};

	class AbilityInstance
	{
	public:
		AbilityInstance(AbilitySystem& abilitySystem, AbilityHandle handle, const AbilityDefinition& definition);

		void SetInputHeld(bool inputHeld);
		void SetLevel(int level);
		void Tick(float deltaTime);
		bool TryActivate();
		void Cancel(AbilityEndReason reason);

		bool IsActive() const { return mIsActive; }
		bool IsOnCooldown() const { return mCooldownRemaining > 0.f; }
		float GetCooldownRemaining() const { return mCooldownRemaining; }
		float GetCooldownDuration() const;
		float GetActiveTimeRemaining() const { return mActiveTimeRemaining; }
		float GetActiveDuration() const;
		int GetCharges() const { return mCharges; }
		int GetLevel() const { return mLevel; }
		AbilityHandle GetHandle() const { return mHandle; }
		const AbilityDefinition& GetDefinition() const { return mDefinition; }

		AbilityRuntimeSnapshot BuildSnapshot() const;

	private:
		void UpdateCooldown(float deltaTime);
		void UpdateInputActivation();
		void TickActiveExecution(float deltaTime);
		void EndAbility(AbilityEndReason reason);
		void RebuildDefinitionForLevel();
		bool IsPressedThisFrame() const;

		AbilitySystem* mAbilitySystem = nullptr;
		AbilityHandle mHandle;
		AbilityDefinition mBaseDefinition;
		AbilityDefinition mDefinition;
		int mLevel = 1;
		bool mInputHeld = false;
		bool mWasInputHeld = false;
		bool mIsActive = false;
		float mCooldownRemaining = 0.f;
		float mActiveTimeRemaining = 0.f;
		int mCharges = 0;
		AbilityExecution mExecution;
	};
}


