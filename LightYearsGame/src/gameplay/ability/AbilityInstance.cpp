#include "gameplay/ability/AbilityInstance.h"
#include "gameplay/ability/AbilityExecutor.h"
#include "gameplay/ability/AbilitySystem.h"
#include "gameplay/attributes/AttributeSystem.h"
#include <algorithm>
#include <variant>

namespace ly
{
	AbilityInstance::AbilityInstance(AbilitySystem& abilitySystem, AbilityHandle handle, const AbilityDefinition& definition)
		: mAbilitySystem{ &abilitySystem },
		mHandle{ handle },
		mBaseDefinition{ definition },
		mDefinition{ definition },
		mLevel{ 1 },
		mCharges{ definition.maxCharges }
	{
	}

	void AbilityInstance::SetInputHeld(bool inputHeld)
	{
		mInputHeld = inputHeld;
	}

	void AbilityInstance::SetLevel(int level)
	{
		const int newLevel = std::max(1, level);
		if (newLevel == mLevel)
		{
			return;
		}

		if (mIsActive)
		{
			EndAbility(AbilityEndReason::Interrupted);
		}
		mLevel = newLevel;
		RebuildDefinitionForLevel();
		if (mAbilitySystem)
		{
			mAbilitySystem->NotifyAbilityLevelChanged(mHandle, mLevel);
		}
	}

	void AbilityInstance::Tick(float deltaTime)
	{
		UpdateCooldown(deltaTime);
		UpdateInputActivation();

		if (mIsActive)
		{
			TickActiveExecution(deltaTime);

			if (mDefinition.lifetimePolicy == AbilityLifetimePolicy::Duration && mActiveTimeRemaining > 0.f)
			{
				const float previousActiveTimeRemaining = mActiveTimeRemaining;
				mActiveTimeRemaining = std::max(0.f, mActiveTimeRemaining - deltaTime);
				if (mAbilitySystem && previousActiveTimeRemaining != mActiveTimeRemaining)
				{
					mAbilitySystem->NotifyAbilityChanged(mHandle);
				}
				if (mActiveTimeRemaining <= 0.f)
				{
					EndAbility(AbilityEndReason::DurationExpired);
				}
			}
		}

		mWasInputHeld = mInputHeld;
	}

	bool AbilityInstance::TryActivate()
	{
		if (mIsActive || IsOnCooldown())
		{
			return false;
		}

		if (mDefinition.maxCharges > 0 && mCharges <= 0)
		{
			return false;
		}
		if (mAbilitySystem &&
			(!mAbilitySystem->HasAllOwnerTags(mDefinition.requiredOwnerTags) ||
				mAbilitySystem->HasAnyOwnerTags(mDefinition.blockedOwnerTags)))
		{
			return false;
		}

		if (mDefinition.maxCharges > 0)
		{
			--mCharges;
		}

		mIsActive = true;
		mActiveTimeRemaining = mDefinition.duration;
		mExecution.actions.clear();
		if (mAbilitySystem)
		{
			mAbilitySystem->NotifyAbilityActivated(mHandle);
		}

		AbilityExecutionContext context{ mAbilitySystem, &mDefinition, nullptr };
		AbilityExecutor::BeginExecution(mExecution, context);

		if (mDefinition.lifetimePolicy == AbilityLifetimePolicy::Instant)
		{
			EndAbility(AbilityEndReason::Completed);
		}

		return true;
	}

	void AbilityInstance::Cancel(AbilityEndReason reason)
	{
		if (mIsActive)
		{
			EndAbility(reason);
		}
	}

	float AbilityInstance::GetCooldownDuration() const
	{
		/*
		 * Effective cooldown order:
		 * 1. Start with this AbilityInstance's private definition copy.
		 * 2. Ability-specific levels mutate only this copy: base cooldown + summed Cooldown value modifiers.
		 * 3. Ship-wide AttributeSystem then applies AbilityHaste as sequential percentage reductions:
		 *    10s with +10% and +10% haste becomes 10 * 0.9 * 0.9 = 8.1s.
		 *
		 * This mirrors AttributeSystem::Recalculate's deterministic layering:
		 * local/base value first, broader modifiers after it.
		 */
		const float leveledCooldown = CalculateModifiedAttributeValue(
			GameplayAttribute{ CommonAttributeIds::Cooldown, mDefinition.cooldown, 0.f },
			mDefinition.attributeModifiers
		);
		const float hasteMultiplier = mAbilitySystem
			? mAbilitySystem->GetAttributes().GetSequentialReductionMultiplier(OwnerAttributeIds::AbilityHaste)
			: 1.f;
		return std::max(0.f, leveledCooldown * hasteMultiplier);
	}

	float AbilityInstance::GetActiveDuration() const
	{
		return mDefinition.duration;
	}

	AbilityRuntimeSnapshot AbilityInstance::BuildSnapshot() const
	{
		return AbilityRuntimeSnapshot{
			mHandle,
			&mDefinition,
			mIsActive,
			mCooldownRemaining,
			GetCooldownDuration(),
			mActiveTimeRemaining,
			GetActiveDuration(),
			mCharges
		};
	}

	void AbilityInstance::UpdateCooldown(float deltaTime)
	{
		if (mCooldownRemaining > 0.f)
		{
			const float previousCooldownRemaining = mCooldownRemaining;
			const int previousCharges = mCharges;
			mCooldownRemaining = std::min(mCooldownRemaining, GetCooldownDuration());
			mCooldownRemaining = std::max(0.f, mCooldownRemaining - deltaTime);
			if (mCooldownRemaining <= 0.f && mDefinition.maxCharges > 0)
			{
				mCharges = mDefinition.maxCharges;
			}
			if (mAbilitySystem && (previousCooldownRemaining != mCooldownRemaining || previousCharges != mCharges))
			{
				mAbilitySystem->NotifyAbilityChanged(mHandle);
			}
		}
	}

	void AbilityInstance::UpdateInputActivation()
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
			if (!mInputHeld && mIsActive && mDefinition.lifetimePolicy == AbilityLifetimePolicy::WhileInputHeld)
			{
				EndAbility(AbilityEndReason::InputReleased);
			}
			break;
		case AbilityActivationPolicy::Toggle:
			if (IsPressedThisFrame())
			{
				mIsActive ? EndAbility(AbilityEndReason::Cancelled) : TryActivate();
			}
			break;
		case AbilityActivationPolicy::Passive:
			TryActivate();
			break;
		case AbilityActivationPolicy::GameplayEvent:
			break;
		}
	}

	void AbilityInstance::TickActiveExecution(float deltaTime)
	{
		AbilityExecutionContext context{ mAbilitySystem, &mDefinition, nullptr };
		AbilityExecutor::TickExecution(mExecution, context, deltaTime);
	}

	void AbilityInstance::EndAbility(AbilityEndReason reason)
	{
		if (!mIsActive)
		{
			return;
		}

		AbilityExecutionContext context{ mAbilitySystem, &mDefinition, nullptr };
		AbilityExecutor::EndExecution(mExecution, context, reason);
		mExecution.actions.clear();
		mIsActive = false;
		mActiveTimeRemaining = 0.f;
		mCooldownRemaining = GetCooldownDuration();
		if (mCooldownRemaining <= 0.f && mDefinition.maxCharges > 0)
		{
			mCharges = mDefinition.maxCharges;
		}
		if (mAbilitySystem)
		{
			mAbilitySystem->NotifyAbilityEnded(mHandle, reason);
		}
	}

	void AbilityInstance::RebuildDefinitionForLevel()
	{
		mDefinition = mBaseDefinition;

		const int stepsToApply = std::min(
			std::max(0, mLevel - 1),
			static_cast<int>(mBaseDefinition.levelProgression.size())
		);

		for (int stepIndex = 0; stepIndex < stepsToApply; ++stepIndex)
		{
			const AbilityLevelStep& step = mBaseDefinition.levelProgression[stepIndex];
			for (const AttributeModifier& modifier : step.modifiers)
			{
				mDefinition.attributeModifiers.push_back(modifier);
			}
		}
	}

	bool AbilityInstance::IsPressedThisFrame() const
	{
		return mInputHeld && !mWasInputHeld;
	}
}


