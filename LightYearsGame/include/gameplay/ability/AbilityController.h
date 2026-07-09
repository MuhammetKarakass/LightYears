#pragma once

#include "gameplay/ability/AbilityDefinition.h"

namespace ly
{
	class Actor;

	class AbilityController
	{
	public:
		AbilityController(Actor* owner, const AbilityDefinition& definition);
		virtual ~AbilityController() = default;

		void SetInputHeld(bool inputHeld);
		void Tick(float deltaTime);
		bool TryActivate();
		void Cancel();

		bool IsActive() const { return mIsActive; }
		bool IsOnCooldown() const { return mCooldownRemaining > 0.f; }
		const AbilityDefinition& GetDefinition() const { return mDefinition; }

	protected:
		Actor* GetOwner() const { return mOwner; }

		virtual bool CanActivate() const;
		virtual void OnActivate() = 0;
		virtual void OnTickActive(float deltaTime);
		virtual void OnEnd();

	private:
		void EndAbility();
		void UpdateCooldown(float deltaTime);
		void UpdateActivationFromInput();
		bool IsPressedThisFrame() const;

		Actor* mOwner;
		AbilityDefinition mDefinition;
		bool mInputHeld;
		bool mWasInputHeld;
		bool mIsActive;
		float mCooldownRemaining;
		float mActiveTimeRemaining;
		int mCharges;
	};
}
