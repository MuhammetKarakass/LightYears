#pragma once

#include "abilities/AbilityLifecycleOrchestrator.h"
#include "abilities/AbilityRuntimeEntry.h"

#include <algorithm>
#include <functional>
#include <utility>

namespace sas
{
	struct AbilityInstanceNotifications
	{
		std::function<void(AbilityHandle)> changed;
		std::function<void(AbilityHandle)> activated;
		std::function<void(AbilityHandle, AbilityEndReason)> ended;
		std::function<void(AbilityHandle, int)> levelChanged;
	};

	template <typename Definition, typename Execution>
	class GameplayAbilityInstance
		: public AbilityRuntimeEntry<Definition, Execution>
	{
		using Base = AbilityRuntimeEntry<Definition, Execution>;

	public:
		virtual ~GameplayAbilityInstance() = default;

		GameplayAbilityInstance(
			AbilityHandle handle,
			const Definition& definition,
			AbilityInstanceNotifications notifications = {}
		)
			: Base{ handle, definition, definition.maxCharges },
			mNotifications{ std::move(notifications) }
		{
		}

		void SetInputHeld(bool inputHeld)
		{
			this->mRuntimeState.SetInputHeld(inputHeld);
		}

		void Tick(float deltaTime)
		{
			UpdateCooldown(deltaTime);
			UpdateInputActivation();

			if (this->mRuntimeState.IsActive())
			{
				TickExecution(deltaTime);
				const AbilityLifecycleDecision durationDecision =
					AbilityLifecycleOrchestrator::TickActiveDuration(
						this->mDefinition.lifetimePolicy,
						this->mRuntimeState,
						deltaTime
					);
				if (durationDecision.stateChanged)
				{
					NotifyChanged();
				}
				if (durationDecision.requestEnd)
				{
					EndAbility(durationDecision.endReason);
				}
			}
			else
			{
				TickInactive(deltaTime);
			}

			this->mRuntimeState.CommitInputFrame();
		}

		bool TryActivate()
		{
			if (!this->mRuntimeState.CanActivate(this->mDefinition.maxCharges) ||
				!CanActivateContent() ||
				!ActivateContent())
			{
				return false;
			}

			this->mRuntimeState.BeginActivation(
				ResolveActiveDuration(),
				this->mDefinition.maxCharges
			);
			BeginExecution();
			if (mNotifications.activated)
			{
				mNotifications.activated(this->mHandle);
			}

			if (AbilityLifecycleOrchestrator::ShouldCompleteImmediately(
				this->mDefinition.lifetimePolicy
			))
			{
				EndAbility(AbilityEndReason::Completed);
			}
			return true;
		}

		void Cancel(AbilityEndReason reason)
		{
			if (this->mRuntimeState.IsActive())
			{
				EndAbility(reason);
			}
		}

		bool SetLevel(int level)
		{
			const int maxLevel = GetMaximumLevel();
			const int newLevel = AbilityRuntimeState::ClampLevel(level, maxLevel);
			if (newLevel == this->mRuntimeState.GetLevel())
			{
				return false;
			}

			if (this->mRuntimeState.IsActive())
			{
				EndAbility(AbilityEndReason::Interrupted);
			}
			this->mRuntimeState.SetLevel(newLevel, maxLevel);
			RebuildDefinitionForLevel();
			OnLevelConfigurationChanged();
			if (mNotifications.levelChanged)
			{
				mNotifications.levelChanged(
					this->mHandle,
					this->mRuntimeState.GetLevel()
				);
			}
			return true;
		}

		void ReduceCooldownRemaining(float amount)
		{
			if (this->mRuntimeState.ReduceCooldown(amount))
			{
				NotifyChanged();
			}
		}

		bool RefreshActiveDuration(float activeDuration)
		{
			if (!this->mRuntimeState.RefreshActiveDuration(activeDuration))
			{
				return false;
			}
			NotifyChanged();
			return true;
		}

		bool IsActive() const { return this->mRuntimeState.IsActive(); }
		bool IsOnCooldown() const
		{
			return this->mRuntimeState.IsOnCooldown();
		}
		float GetCooldownRemaining() const
		{
			return this->mRuntimeState.GetCooldownRemaining();
		}
		float GetCooldownDuration() const
		{
			return ResolveCooldownDuration();
		}
		float GetActiveTimeRemaining() const
		{
			return this->mRuntimeState.GetActiveTimeRemaining();
		}
		float GetActiveDuration() const
		{
			return ResolveActiveDuration();
		}
		int GetCharges() const { return this->mRuntimeState.GetCharges(); }
		int GetLevel() const { return this->mRuntimeState.GetLevel(); }
		int GetMaxLevel() const { return GetMaximumLevel(); }
		AbilityHandle GetHandle() const { return this->mHandle; }
		const Definition& GetDefinition() const { return this->mDefinition; }

		AbilityRuntimeSnapshot BuildSnapshot() const
		{
			return this->BuildRuntimeSnapshot(
				GetMaximumLevel(),
				ResolveCooldownDuration(),
				ResolveActiveDuration()
			);
		}

	protected:
		virtual bool CanActivateContent() const { return true; }
		virtual bool ActivateContent() { return true; }
		virtual void BeginExecution() {}
		virtual void TickExecution(float) {}
		virtual void EndExecution(AbilityEndReason) {}
		virtual void TickInactive(float) {}
		virtual void EndContent(AbilityEndReason) {}
		virtual int GetMaximumLevel() const = 0;
		virtual float ResolveCooldownDuration() const = 0;
		virtual float ResolveActiveDuration() const = 0;
		virtual void RebuildDefinitionForLevel() = 0;
		virtual void OnLevelConfigurationChanged() {}

		void NotifyChanged()
		{
			if (mNotifications.changed)
			{
				mNotifications.changed(this->mHandle);
			}
		}

	private:
		void UpdateCooldown(float deltaTime)
		{
			if (this->mRuntimeState.IsOnCooldown() &&
				this->mRuntimeState.TickCooldown(
					deltaTime,
					ResolveCooldownDuration(),
					this->mDefinition.maxCharges
				))
			{
				NotifyChanged();
			}
		}

		void UpdateInputActivation()
		{
			const AbilityLifecycleDecision decision =
				AbilityLifecycleOrchestrator::EvaluateInput(
					this->mDefinition.activationPolicy,
					this->mDefinition.lifetimePolicy,
					this->mRuntimeState
				);
			if (decision.requestEnd)
			{
				EndAbility(decision.endReason);
			}
			else if (decision.requestActivation)
			{
				TryActivate();
			}
		}

		void EndAbility(AbilityEndReason reason)
		{
			if (!this->mRuntimeState.IsActive())
			{
				return;
			}

			EndContent(reason);
			EndExecution(reason);
			this->mRuntimeState.EndActivation(
				ResolveCooldownDuration(),
				this->mDefinition.maxCharges
			);
			if (mNotifications.ended)
			{
				mNotifications.ended(this->mHandle, reason);
			}
		}

		AbilityInstanceNotifications mNotifications;
	};
}
