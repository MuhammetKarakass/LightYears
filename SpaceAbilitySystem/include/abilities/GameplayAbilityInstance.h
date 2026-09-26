#pragma once

#include "abilities/AbilityLifecycleOrchestrator.h"
#include "abilities/AbilityRuntimeEntry.h"

#include <algorithm>
#include <functional>
#include <exception>
#include <optional>
#include <type_traits>
#include <utility>

namespace sas
{
	struct AbilityInstanceNotifications
	{
		std::function<void(AbilityHandle)> changed;
		std::function<void(AbilityHandle)> activated;
		std::function<void(AbilityHandle, AbilityEndReason)> ended;
		std::function<void(AbilityHandle, int)> levelChanged;
		std::function<void(const std::function<void()>&)> execute;
		std::function<bool()> canExecute;
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
			return RunInstanceOperation([&]()
			{
				if (mExecutionCallbackDepth != 0 || mActivating || mEnding || (mNotifications.canExecute && !mNotifications.canExecute())) return;
				if (!this->mRuntimeState.IsActive() && HasPendingCleanup())
				{
					RetryPendingCleanup();
				}
				UpdateCooldown(deltaTime);
				if (mNotifications.canExecute && !mNotifications.canExecute()) return;
				UpdateInputActivation();
				if (mNotifications.canExecute && !mNotifications.canExecute()) return;

				if (this->mRuntimeState.IsActive())
				{
					// Track elapsed active time for all lifetimes, not only Duration.
					// Toggle input uses this shared clock to enforce the minimum commit
					// window without teaching individual abilities about key timing.
					this->mRuntimeState.TickActiveTime(deltaTime);
					RunExecutionCallback([&] { TickExecution(deltaTime); });
					if (!this->mRuntimeState.IsActive() || !ShouldContinueExecution()) return;
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
					RunExecutionCallback([&] { TickInactive(deltaTime); });
					if (!ShouldContinueExecution()) return;
				}

				this->mRuntimeState.CommitInputFrame();
			});
		}

		bool TryActivate()
		{
			return RunInstanceOperation([&]() -> bool
			{
				if (mExecutionCallbackDepth != 0 || mActivating || mEnding || (mNotifications.canExecute && !mNotifications.canExecute())) return false;
				if (!this->mRuntimeState.IsActive() && HasPendingCleanup())
				{
					RetryPendingCleanup();
				}
				if (!this->mRuntimeState.CanActivate(this->mDefinition.maxCharges)) return false;
				mActivating = true;
				mActivationCancelled = false;
				struct ActivationScope { bool& active; ~ActivationScope() { active = false; } } scope{ mActivating };
				bool contentStarted = false;
				bool committed = false;
				try
				{
					if (!CanActivateContent() || ActivationInterrupted()) return false;
					contentStarted = true;
					if (!ActivateContent() || ActivationInterrupted())
					{
						contentStarted = false; // Abort owns cleanup, including if cleanup throws.
						AbortActivationContent();
						return false;
					}
					const float duration = ResolveActiveDuration();
					const bool deferred = ShouldDeferActiveDurationStart();
					const float cooldown = this->mDefinition.cooldownStartPolicy == AbilityCooldownStartPolicy::OnActivation
						? ResolveCooldownDuration() : 0.f;
					if (ActivationInterrupted())
					{
						contentStarted = false;
						AbortActivationContent();
						return false;
					}
					this->mRuntimeState.BeginActivation(duration, this->mDefinition.maxCharges, deferred);
					committed = true;
					if (this->mDefinition.cooldownStartPolicy == AbilityCooldownStartPolicy::OnActivation)
						this->mRuntimeState.StartCooldown(cooldown);
					OnActivationCommitted();
					if (!ActivationInterrupted())
					{
						mExecutionStarted = true;
						RunExecutionCallback([&] { BeginExecution(); });
					}
					if (this->mRuntimeState.IsActive() &&
						ShouldContinueExecution() && mNotifications.activated)
						mNotifications.activated(this->mHandle);

					if (mActivationCancelled) EndAbility(mActivationCancelReason);
					else if (AbilityLifecycleOrchestrator::ShouldCompleteImmediately(this->mDefinition.lifetimePolicy))
						EndAbility(AbilityEndReason::Completed);
					return true;
				}
				catch (...)
				{
					const auto error = std::current_exception();
					try
					{
						if (committed) EndAbility(AbilityEndReason::Interrupted);
						else if (contentStarted) AbortActivationContent();
					}
					catch (...) {} // Preserve the activation failure after attempting cleanup.
					std::rethrow_exception(error);
				}
			});
		}

		void Cancel(AbilityEndReason reason)
		{
			return RunInstanceOperation([&]()
			{
				if (mEnding) return;
				if (mActivating)
				{
					RecordActivationEndRequest(reason);
					return;
				}
				if (mExecutionCallbackDepth != 0)
				{
					RecordPendingEndRequest(reason);
					return;
				}
				if (this->mRuntimeState.IsActive())
				{
					EndAbility(reason);
				}
				else if (HasPendingCleanup())
				{
					RetryPendingCleanup();
				}
			});
		}

		bool SetLevel(int level)
		{
			return RunInstanceOperation([&]() -> bool
			{
				if (mExecutionCallbackDepth != 0 || mActivating || mEnding || (mNotifications.canExecute && !mNotifications.canExecute())) return false;
				EnsurePendingCleanupDrained();
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
			});
		}

		void ReduceCooldownRemaining(float amount)
		{
			return RunInstanceOperation([&]()
			{
				if (this->mRuntimeState.ReduceCooldown(amount))
				{
					NotifyChanged();
				}
			});
		}

		bool RefreshActiveDuration(float activeDuration)
		{
			return RunInstanceOperation([&]() -> bool
			{
				if (!this->mRuntimeState.RefreshActiveDuration(activeDuration))
				{
					return false;
				}
				NotifyChanged();
				return true;
			});
		}

		bool StartDeferredActiveDuration(float activeDuration)
		{
			return RunInstanceOperation([&]() -> bool
			{
				if (!this->mRuntimeState.StartDeferredActiveDuration(activeDuration))
				{
					return false;
				}
				NotifyChanged();
				return true;
			});
		}

		bool IsActive() const { return this->mRuntimeState.IsActive(); }
		bool HasPendingCleanup() const
		{
			return !this->mRuntimeState.IsActive() &&
				(mExecutionStarted || HasPendingContentCleanup());
		}
		bool IsInCallbackScope() const
		{
			return mActivating || mEnding || mExecutionCallbackDepth != 0;
		}
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
		template<typename Operation>
		auto RunInstanceOperation(Operation&& operation) -> decltype(operation())
		{
			// Copy before entry: completing the outer operation can destroy this
			// instance. Do not read members after the execution boundary returns.
			const auto execute = mNotifications.execute;
			auto run = [&]() -> decltype(operation())
			{
				std::exception_ptr error;
				if constexpr (std::is_void_v<decltype(operation())>)
				{
					try { operation(); }
					catch (...) { error = std::current_exception(); }
					try { OnInstanceOperationCompleted(); }
					catch (...) { if (!error) error = std::current_exception(); }
					if (error) std::rethrow_exception(error);
				}
				else
				{
					std::optional<decltype(operation())> result;
					try { result.emplace(operation()); }
					catch (...) { error = std::current_exception(); }
					try { OnInstanceOperationCompleted(); }
					catch (...) { if (!error) error = std::current_exception(); }
					if (error) std::rethrow_exception(error);
					return std::move(*result);
				}
			};
			if (!execute) return run();
			if constexpr (std::is_void_v<decltype(operation())>) execute(run);
			else
			{
				std::optional<decltype(operation())> result;
				execute([&] { result.emplace(run()); });
				return std::move(*result);
			}
		}
		virtual bool CanActivateContent() const { return true; }
		virtual bool ActivateContent() { return true; }
		// Called for a partially started activation, before charges/cooldown commit.
		// Implementations must release any resources acquired by ActivateContent.
		virtual void AbortActivationContent() { EndContent(AbilityEndReason::Interrupted); }
		virtual void OnActivationCommitted() {}
		virtual void BeginExecution() {}
		virtual void TickExecution(float) {}
		virtual void EndExecution(AbilityEndReason) {}
		virtual void TickInactive(float) {}
		virtual bool HandleInputPressed() { return false; }
		virtual bool ShouldDeferActiveDurationStart() const { return false; }
		virtual void EndContent(AbilityEndReason) {}
		virtual bool HasPendingContentCleanup() const { return false; }
		virtual void RetryContentCleanup() {}
		virtual void OnExecutionCallbackCompleted() {}
		virtual void OnInstanceOperationCompleted() {}
		void EnsurePendingCleanupDrained()
		{
			if (HasPendingCleanup()) RetryPendingCleanup();
		}
		virtual int GetMaximumLevel() const = 0;
		virtual float ResolveCooldownDuration() const = 0;
		// Behaviors with an explicit end-state reward can override this without
		// editing the runtime state after its cooldown has already begun.
		virtual float ResolveCooldownDurationOnEnd(AbilityEndReason)
		{
			return ResolveCooldownDuration();
		}
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

		bool ActivationInterrupted() const
		{
			return mActivationCancelled || mPendingEndReason.has_value() ||
				(mNotifications.canExecute && !mNotifications.canExecute());
		}

		bool ShouldContinueExecution() const
		{
			return !mActivationCancelled && !mPendingEndReason.has_value() &&
				(!mNotifications.canExecute || mNotifications.canExecute());
		}

		template<typename Operation>
		void RunExecutionCallback(Operation&& operation)
		{
			++mExecutionCallbackDepth;
			std::exception_ptr error;
			try { operation(); }
			catch (...) { error = std::current_exception(); }
			--mExecutionCallbackDepth;

			if (mExecutionCallbackDepth == 0)
			{
				try { OnExecutionCallbackCompleted(); }
				catch (...) { if (!error) error = std::current_exception(); }
				if (mPendingEndReason && this->mRuntimeState.IsActive() && !mEnding)
				{
					const AbilityEndReason reason = *mPendingEndReason;
					mPendingEndReason.reset();
					try { EndAbility(reason); }
					catch (...) { if (!error) error = std::current_exception(); }
				}
			}
			if (error) std::rethrow_exception(error);
		}

	private:
		void RecordActivationEndRequest(AbilityEndReason reason)
		{
			if (!mActivationCancelled || reason == AbilityEndReason::OwnerDestroyed)
			{
				mActivationCancelled = true;
				mActivationCancelReason = reason;
			}
		}

		void RecordPendingEndRequest(AbilityEndReason reason)
		{
			if (!mPendingEndReason || reason == AbilityEndReason::OwnerDestroyed)
			{
				mPendingEndReason = reason;
			}
		}

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
			if (this->mRuntimeState.IsActive() &&
				this->mRuntimeState.IsPressedThisFrame() &&
				RunInputPressedCallback())
			{
				return;
			}

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
			if (mEnding || !this->mRuntimeState.IsActive())
			{
				return;
			}

			mEnding = true;
			struct EndScope { bool& ending; ~EndScope() { ending = false; } } scope{ mEnding };
			std::exception_ptr error;
			const auto finish = [&error](auto&& operation)
			{
				try { operation(); }
				catch (...) { if (!error) error = std::current_exception(); }
			};
			finish([&] { EndContent(reason); });
			if (mExecutionStarted)
			{
				mExecutionEndReason = reason;
				try
				{
					RunExecutionCallback([&] { EndExecution(reason); });
					mExecutionStarted = false;
				}
				catch (...)
				{
					if (!error) error = std::current_exception();
				}
			}
			float cooldownOnEnd = this->mRuntimeState.GetCooldownRemaining();
			finish([&]
			{
				if (this->mDefinition.cooldownStartPolicy != AbilityCooldownStartPolicy::OnActivation)
					cooldownOnEnd = ResolveCooldownDurationOnEnd(reason);
			});
			this->mRuntimeState.EndActivation(
				cooldownOnEnd,
				this->mDefinition.maxCharges
			);
			finish([&] { if (mNotifications.ended) mNotifications.ended(this->mHandle, reason); });
			if (error) std::rethrow_exception(error);
		}

		void RetryPendingCleanup()
		{
			if (!HasPendingCleanup() ||
				mExecutionCallbackDepth != 0 || mActivating || mEnding)
			{
				return;
			}
			mEnding = true;
			struct EndScope { bool& ending; ~EndScope() { ending = false; } } scope{ mEnding };
			std::exception_ptr error;
			try { RetryContentCleanup(); }
			catch (...) { error = std::current_exception(); }
			if (mExecutionStarted)
			{
				try
				{
					RunExecutionCallback([&] { EndExecution(mExecutionEndReason); });
					mExecutionStarted = false;
				}
				catch (...)
				{
					if (!error) error = std::current_exception();
				}
			}
			if (error) std::rethrow_exception(error);
		}

		bool RunInputPressedCallback()
		{
			bool handled = false;
			RunExecutionCallback([&] { handled = HandleInputPressed(); });
			return handled;
		}

		AbilityInstanceNotifications mNotifications;
		bool mEnding = false;
		bool mActivating = false;
		bool mExecutionStarted = false;
		bool mActivationCancelled = false;
		AbilityEndReason mActivationCancelReason = AbilityEndReason::Cancelled;
		AbilityEndReason mExecutionEndReason = AbilityEndReason::Interrupted;
		std::size_t mExecutionCallbackDepth = 0;
		std::optional<AbilityEndReason> mPendingEndReason;
	};
}
