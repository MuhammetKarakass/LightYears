#include "gameplay/ability/AbilityController.h"
#include <algorithm>

namespace ly
{
	AbilityController::AbilityController(Actor* owner, const AbilityDefinition& definition)
		: mOwner{ owner },
		mDefinition{ definition },
		mInputHeld{ false },
		mWasInputHeld{ false },
		mIsActive{ false },
		mCooldownRemaining{ 0.f },
		mActiveTimeRemaining{ 0.f },
		mCharges{ definition.maxCharges }
	{
	}

	void AbilityController::SetInputHeld(bool inputHeld)
	{
		mInputHeld = inputHeld;
	}

	void AbilityController::Tick(float deltaTime)
	{
		UpdateCooldown(deltaTime);
		UpdateActivationFromInput();

		if (mIsActive)
		{
			OnTickActive(deltaTime);

			if (mDefinition.duration > 0.f)
			{
				mActiveTimeRemaining -= deltaTime;
				if (mActiveTimeRemaining <= 0.f)
				{
					EndAbility();
				}
			}
		}

		mWasInputHeld = mInputHeld;
	}

	bool AbilityController::TryActivate()
	{
		if (mIsActive || !CanActivate())
		{
			return false;
		}

		if (mDefinition.maxCharges > 0 && mCharges <= 0)
		{
			return false;
		}

		if (mDefinition.maxCharges > 0)
		{
			--mCharges;
		}

		mIsActive = true;
		mActiveTimeRemaining = mDefinition.duration;
		OnActivate();

		if (mDefinition.activationPolicy == AbilityActivationPolicy::OnPressed && mDefinition.duration <= 0.f)
		{
			EndAbility();
		}

		return true;
	}

	void AbilityController::Cancel()
	{
		if (mIsActive)
		{
			EndAbility();
		}
	}

	bool AbilityController::CanActivate() const
	{
		return mOwner != nullptr && !IsOnCooldown();
	}

	void AbilityController::OnTickActive(float deltaTime)
	{
		(void)deltaTime;
	}

	void AbilityController::OnEnd()
	{
	}

	void AbilityController::EndAbility()
	{
		if (!mIsActive)
		{
			return;
		}

		mIsActive = false;
		mActiveTimeRemaining = 0.f;
		mCooldownRemaining = std::max(0.f, mDefinition.cooldown);
		OnEnd();
	}

	void AbilityController::UpdateCooldown(float deltaTime)
	{
		if (mCooldownRemaining > 0.f)
		{
			mCooldownRemaining = std::max(0.f, mCooldownRemaining - deltaTime);
		}
	}

	void AbilityController::UpdateActivationFromInput()
	{
		switch (mDefinition.activationPolicy)
		{
		case AbilityActivationPolicy::OnPressed:
			if (IsPressedThisFrame())
			{
				TryActivate();
			}
			break;
		case AbilityActivationPolicy::WhileHeld:
			if (mInputHeld)
			{
				TryActivate();
			}
			else if (mIsActive)
			{
				EndAbility();
			}
			break;
		case AbilityActivationPolicy::Toggle:
			if (IsPressedThisFrame())
			{
				if (mIsActive)
				{
					EndAbility();
				}
				else
				{
					TryActivate();
				}
			}
			break;
		case AbilityActivationPolicy::Passive:
			TryActivate();
			break;
		}
	}

	bool AbilityController::IsPressedThisFrame() const
	{
		return mInputHeld && !mWasInputHeld;
	}
}
