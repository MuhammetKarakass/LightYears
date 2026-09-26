#include "gameplay/ability/LightYearsAbilitySystemComponent.h"

#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/validation/GameAbilityDefinitionValidator.h"
#include "gameplay/ability/validation/GameplayEffectDefinitionValidator.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "attributes/AttributeMath.h"

#include <algorithm>
#include <exception>
#include <utility>

namespace ly
{
	namespace
	{
		bool IsEffectBehaviorRegistered(const sas::GameplayEffectBehaviorKey& behaviorKey)
		{
			return ::ly::GetEffectBehaviorRuntime().IsRegistered(behaviorKey);
		}

		bool ValidateEffectForRuntime(
			const sas::GameplayEffectDefinition& definition,
			std::string* failureReason
		)
		{
			return GameplayEffectDefinitionValidator::Validate(
				definition,
				IsEffectBehaviorRegistered,
				failureReason
			);
		}

		void ExecuteTriggeredActions(
			LightYearsAbilitySystemComponent& abilitySystem,
			GameAbility& ability,
			const GameAbilityDefinition& definition,
			const AbilityTriggerSpec& trigger,
			const sas::AbilityEvent& event
		)
		{
			GameAbilityDefinition triggerDefinition = definition;
			triggerDefinition.actions = trigger.actions;

			GameAbilityExecution execution;
			AbilityExecutionContext context{
				&abilitySystem,
				&triggerDefinition,
				&event,
				&ability
			};
			GameAbilityActionExecutor::BeginExecution(execution, context);
			GameAbilityActionExecutor::TickExecution(execution, context, 0.f);
			GameAbilityActionExecutor::EndExecution(
				execution,
				context,
				sas::AbilityEndReason::Completed
			);
		}
	}

	LightYearsAbilitySystemComponent::LightYearsAbilitySystemComponent(Actor& owner)
		: mOwner{ owner },
		mComponentRuntime{
			InitializeAbilitySystem<GameAbilityDefinition, GameAbility>(MaxPassiveAbilities)
		},
		mAbilityInvocationRuntime{ *this }
	{
		sas::AbilityRuntimeSystem<GameAbilityDefinition, GameAbility>::Callbacks callbacks;
		callbacks.validate =
			[](const GameAbilityDefinition& definition, std::string* failureReason)
			{
				LY_GAME_INFO("Ability validate begin: %s", definition.abilityId.c_str());
				const bool result = GameAbilityDefinitionValidator::Validate(
					definition,
					ValidateEffectForRuntime,
					failureReason
				);
				LY_GAME_INFO("Ability validate returned: %s", result ? "valid" : "invalid");
				return result;
			};
		callbacks.create =
			[this](
				sas::AbilityHandle handle,
				const GameAbilityDefinition& definition,
				std::string* failureReason
			) -> unique_ptr<GameAbility>
			{
				LY_GAME_INFO("Ability create begin: %s", definition.abilityId.c_str());
				unique_ptr<GameAbilityBehavior> behavior =
					GameAbilityBehaviorRegistry::Create(definition.behaviorType);
				if (!behavior)
				{
					if (failureReason)
					{
						*failureReason = "Ability behavior could not be created.";
					}
					return {};
				}
				unique_ptr<GameAbility> ability = std::make_unique<GameAbility>(
					*this,
					handle,
					definition,
					std::move(behavior)
				);
				LY_GAME_INFO("Ability instance constructed: %s", definition.abilityId.c_str());
				// Scoped progression rules may exist before a content ability is
				// granted. Materialize their level effect immediately so future
				// abilities receive the same category-based rule as existing ones.
				ability->RefreshScopedConfiguration();
				LY_GAME_INFO("Ability create returned: %s", definition.abilityId.c_str());
				return ability;
			};
		callbacks.cancel = [](GameAbility& ability, sas::AbilityEndReason reason)
		{
			ability.Cancel(reason);
		};
		callbacks.tick = [](GameAbility& ability, float deltaTime)
		{
			ability.Tick(deltaTime);
		};
		callbacks.setInput = [](GameAbility& ability, bool inputHeld)
		{
			ability.SetInputHeld(inputHeld);
		};
		callbacks.snapshot = [](const GameAbility& ability)
		{
			return ability.BuildSnapshot();
		};
		ConfigureAbilityRuntime(mComponentRuntime, std::move(callbacks));
		SetGameplayEventHandler<sas::AbilityEvent>(
			[this](const sas::AbilityEvent& event)
			{
				ProcessGameGameplayEvent(event);
			}
		);
	}

	bool LightYearsAbilitySystemComponent::TryEquipAttachment(
		sas::AbilityHandle handle,
		const AttachmentDefinition& definition,
		AttachmentHostKind hostKind,
		std::string* failureReason
	)
	{
		GameAbility* ability = GetAbility(handle);
		if (!ability)
		{
			if (failureReason)
			{
				*failureReason = "Attachment host ability was not found.";
			}
			return false;
		}
		return ability->TryEquipAttachment(definition, hostKind, failureReason);
	}

	bool LightYearsAbilitySystemComponent::TryEquipAttachment(
		sas::AbilitySlot slot,
		const AttachmentDefinition& definition,
		AttachmentHostKind hostKind,
		std::string* failureReason
	)
	{
		GameAbility* ability = GetAbility(slot);
		if (!ability)
		{
			if (failureReason)
			{
				*failureReason = "Attachment host ability was not found.";
			}
			return false;
		}
		return ability->TryEquipAttachment(definition, hostKind, failureReason);
	}

	bool LightYearsAbilitySystemComponent::RemoveAttachment(
		sas::AbilityHandle handle,
		const std::string& attachmentId,
		AttachmentHostKind hostKind
	)
	{
		GameAbility* ability = GetAbility(handle);
		return ability && ability->RemoveAttachment(attachmentId, hostKind);
	}

	GameAbility* LightYearsAbilitySystemComponent::GetAbility(sas::AbilitySlot slot)
	{
		return FindAbility<GameAbility>(slot);
	}

	const GameAbility* LightYearsAbilitySystemComponent::GetAbility(sas::AbilitySlot slot) const
	{
		return FindAbility<GameAbility>(slot);
	}

	GameAbility* LightYearsAbilitySystemComponent::GetAbility(sas::AbilityHandle handle)
	{
		return FindAbility<GameAbility>(handle);
	}

	const GameAbility* LightYearsAbilitySystemComponent::GetAbility(sas::AbilityHandle handle) const
	{
		return FindAbility<GameAbility>(handle);
	}

	GameAbility* LightYearsAbilitySystemComponent::GetAbilityById(const std::string& abilityId)
	{
		return FindAbilityById<GameAbility>(abilityId);
	}

	const GameAbility* LightYearsAbilitySystemComponent::GetAbilityById(
		const std::string& abilityId
	) const
	{
		return FindAbilityById<GameAbility>(abilityId);
	}

	void LightYearsAbilitySystemComponent::AddOwnedTag(const GameplayTag& tag)
	{
		if (IsClearPending()) return;
		return RunOperation([&]()
		{
			sas::AbilitySystemComponent::AddOwnedTag(tag);

			if (tag != GameplayTagSchema::BlockPrimaryWeaponFire)
			{
				return;
			}

			// The primary weapon may already be active when another ability grants
			// this shared lock. EndAbility() runs the primary weapon's EndExecution,
			// which clears its isFiring state and prevents another projectile from
			// being emitted on the next frame.
			GameAbility* primaryWeapon = GetAbility(sas::AbilitySlot::PrimaryFire);
			if (primaryWeapon && primaryWeapon->IsActive())
			{
				primaryWeapon->Cancel(sas::AbilityEndReason::Interrupted);
			}
		});
	}

	void LightYearsAbilitySystemComponent::SetAbilitySlotInput(
		sas::AbilitySlot slot,
		bool inputHeld
	)
	{
		return RunOperation([&]()
		{
			sas::AbilitySystemComponent::SetAbilitySlotInput(slot, inputHeld);
			mAbilityInvocationRuntime.SetControlInput(slot, inputHeld);
		});
	}

	void LightYearsAbilitySystemComponent::Tick(float deltaTime)
	{
		if (IsClearPending() || mTicking) return;
		return RunOperation([&]()
		{
			mTicking = true;
			struct TickScope { bool& ticking; ~TickScope() { ticking = false; } } scope{ mTicking };
			// Effects are ticked before abilities by the generic runtime. Cancel any
			// ability that was already stunned on the previous frame, then let the
			// ability tick guard handle a stun applied during this frame's effect pass.
			CancelActiveAbilitiesForStun();
			sas::AbilitySystemComponent::Tick(deltaTime);
			if (IsClearPending()) return;
			mAbilityInvocationRuntime.Tick(deltaTime);
			CancelActiveAbilitiesForStun();
		});
	}

	void LightYearsAbilitySystemComponent::ClearAdditionalState(
		bool preserveRuntimeDependencies
	)
	{
		std::exception_ptr error;
		try { mAbilityInvocationRuntime.Clear(); }
		catch (...) { error = std::current_exception(); }
		mAbilityUseHistory.Clear();
		mLifecycleDispatcher.Clear();
		if (!preserveRuntimeDependencies && !error)
		{
			mScopedAbilityRules.clear();
			mDeferredScopedAbilityRules.clear();
			mHasDeferredScopedAbilityRules = false;
			mPrimaryWeaponOverrides.Clear();
		}
		if (error) std::rethrow_exception(error);
	}

	void LightYearsAbilitySystemComponent::OnClearCompleted()
	{
		if (mOwnerClearCompletion) mOwnerClearCompletion();
	}

	void LightYearsAbilitySystemComponent::OnAbilityInstanceOperationsCompleted()
	{
		FlushDeferredScopedAbilityRules();
	}

	PrimaryWeaponOverrideHandle LightYearsAbilitySystemComponent::PushPrimaryWeaponOverride(
		const sas::ContentId& sourceId,
		const PrimaryWeaponDefinition& weaponDefinition,
		int priority
	)
	{
		if (IsClearPending()) return {};
		return mPrimaryWeaponOverrides.Push(sourceId, weaponDefinition, priority);
	}

	bool LightYearsAbilitySystemComponent::RemovePrimaryWeaponOverride(
		PrimaryWeaponOverrideHandle handle
	)
	{
		return mPrimaryWeaponOverrides.Remove(handle);
	}

	PrimaryWeaponOverrideState::ActiveOverride*
	LightYearsAbilitySystemComponent::GetActivePrimaryWeaponOverride()
	{
		return mPrimaryWeaponOverrides.GetActive();
	}

	const PrimaryWeaponOverrideState::ActiveOverride*
	LightYearsAbilitySystemComponent::GetActivePrimaryWeaponOverride() const
	{
		return mPrimaryWeaponOverrides.GetActive();
	}

	bool LightYearsAbilitySystemComponent::InvokeRecordedAbility(
		const AbilityUseRecord& record,
		const List<sas::AttributeScalingRule>& scalingRules,
		float outputMultiplier,
		sas::AbilitySlot controlSlot,
		bool inputHeld,
		std::string* failureReason
	)
	{
		return RunOperation([&]() -> bool
		{
			if (IsClearPending()) return false;
			return mAbilityInvocationRuntime.Invoke(
				record,
				scalingRules,
				outputMultiplier,
				controlSlot,
				inputHeld,
				failureReason
			);
		});
	}

	AbilityLifecycleObserverHandle
		LightYearsAbilitySystemComponent::RegisterAbilityLifecycleObserver(
			AbilityLifecycleObserverFilter filter,
			AbilityLifecycleObserver callback,
			int priority
		)
	{
		if (IsClearPending()) return {};
		return mLifecycleDispatcher.RegisterObserver(
			std::move(filter),
			std::move(callback),
			priority
		);
	}

	bool LightYearsAbilitySystemComponent::UnregisterAbilityLifecycleObserver(
		AbilityLifecycleObserverHandle handle
	)
	{
		return mLifecycleDispatcher.UnregisterObserver(handle);
	}

	AbilityActivationGuardHandle
		LightYearsAbilitySystemComponent::RegisterAbilityActivationGuard(
			AbilityActivationGuard callback,
			int priority
		)
	{
		if (IsClearPending()) return {};
		return mLifecycleDispatcher.RegisterActivationGuard(
			std::move(callback),
			priority
		);
	}

	bool LightYearsAbilitySystemComponent::UnregisterAbilityActivationGuard(
		AbilityActivationGuardHandle handle
	)
	{
		return mLifecycleDispatcher.UnregisterActivationGuard(handle);
	}

	bool LightYearsAbilitySystemComponent::EvaluateAbilityActivation(
		const sas::AbilityLifecycleEvent& event
	) const
	{
		return mLifecycleDispatcher.CanActivate(event);
	}

	void LightYearsAbilitySystemComponent::CancelActiveAbilitiesForStun()
	{
		if (!HasOwnedTag(GameplayTags::State::Effect::Control::Stunned))
		{
			return;
		}

		for (const sas::AbilityRuntimeSnapshot& snapshot : BuildAbilitySnapshots())
		{
			if (!snapshot.active)
			{
				continue;
			}

			if (GameAbility* ability = GetAbility(snapshot.handle))
			{
				ability->Cancel(sas::AbilityEndReason::Interrupted);
			}
		}
	}

	std::size_t LightYearsAbilitySystemComponent::AddScopedAbilityRule(
		ScopedAbilityRule rule
	)
	{
		const std::size_t handle = mNextScopedAbilityRuleHandle++;
		if (HasAbilityCallbackInProgress() || mRefreshingScopedAbilityRules)
		{
			if (!mHasDeferredScopedAbilityRules)
			{
				mDeferredScopedAbilityRules = mScopedAbilityRules;
				mHasDeferredScopedAbilityRules = true;
			}
			mDeferredScopedAbilityRules.emplace_back(handle, std::move(rule));
			return handle;
		}
		FlushDeferredScopedAbilityRules();
		mScopedAbilityRules.emplace_back(handle, std::move(rule));
		RefreshScopedAbilityRules();
		FlushDeferredScopedAbilityRules();
		return handle;
	}

	bool LightYearsAbilitySystemComponent::RemoveScopedAbilityRule(
		std::size_t ruleHandle
	)
	{
		const bool defer = HasAbilityCallbackInProgress() || mRefreshingScopedAbilityRules;
		if (!defer) FlushDeferredScopedAbilityRules();
		if (defer && !mHasDeferredScopedAbilityRules)
		{
			mDeferredScopedAbilityRules = mScopedAbilityRules;
			mHasDeferredScopedAbilityRules = true;
		}
		auto& rules = defer ? mDeferredScopedAbilityRules : mScopedAbilityRules;
		const auto found = std::find_if(
			rules.begin(),
			rules.end(),
			[&](const auto& entry)
			{
				return entry.first == ruleHandle;
			}
		);
		if (found == rules.end())
		{
			return false;
		}
		rules.erase(found);
		if (defer) return true;
		RefreshScopedAbilityRules();
		FlushDeferredScopedAbilityRules();
		return true;
	}

	void LightYearsAbilitySystemComponent::ClearScopedAbilityRules()
	{
		const bool defer = HasAbilityCallbackInProgress() || mRefreshingScopedAbilityRules;
		if (!defer) FlushDeferredScopedAbilityRules();
		if (defer && !mHasDeferredScopedAbilityRules)
		{
			mDeferredScopedAbilityRules = mScopedAbilityRules;
			mHasDeferredScopedAbilityRules = true;
		}
		auto& rules = defer ? mDeferredScopedAbilityRules : mScopedAbilityRules;
		if (rules.empty())
		{
			return;
		}
		rules.clear();
		if (defer) return;
		RefreshScopedAbilityRules();
		FlushDeferredScopedAbilityRules();
	}

	sas::GameplayAttribute LightYearsAbilitySystemComponent::ApplyScopedAbilityModifiers(
		const GameAbilityDefinition& definition,
		const sas::GameplayAttribute& attribute
	) const
	{
		sas::GameplayAttribute result = attribute;
		for (const auto& entry : mScopedAbilityRules)
		{
			const ScopedAbilityRule& rule = entry.second;
			if (!MatchesScopedAbilityRule(rule, definition))
			{
				continue;
			}
			result.currentValue = sas::CalculateModifiedAttributeValue(
				sas::GameplayAttribute{
					result.id,
					result.currentValue,
					result.minValue,
					result.maxValue
				},
				rule.attributeModifiers
			);
		}
		return result;
	}

	int LightYearsAbilitySystemComponent::GetScopedAbilityLevelBonus(
		const GameAbilityDefinition& definition
	) const
	{
		int bonus = 0;
		for (const auto& entry : mScopedAbilityRules)
		{
			const ScopedAbilityRule& rule = entry.second;
			if (MatchesScopedAbilityRule(rule, definition))
			{
				bonus += rule.levelBonus;
			}
		}
		return bonus;
	}

	bool LightYearsAbilitySystemComponent::MatchesScopedAbilityRule(
		const ScopedAbilityRule& rule,
		const GameAbilityDefinition& definition
	) const
	{
		GameplayTagContainer abilityTags;
		for (const GameplayTag& tag : definition.abilityTags)
		{
			abilityTags.AddTag(tag);
		}
		return abilityTags.HasAll(rule.requiredAbilityTags) &&
			!abilityTags.HasAny(rule.blockedAbilityTags);
	}

	void LightYearsAbilitySystemComponent::RefreshScopedAbilityRules()
	{
		if (mRefreshingScopedAbilityRules || IsClearPending()) return;
		mRefreshingScopedAbilityRules = true;
		struct RefreshScope
		{
			bool& refreshing;
			~RefreshScope() { refreshing = false; }
		} scope{ mRefreshingScopedAbilityRules };
		std::exception_ptr error;
		RunOperation([&]()
		{
			for (const sas::AbilityRuntimeSnapshot& snapshot : BuildAbilitySnapshots())
			{
				if (IsClearPending()) break;
				if (GameAbility* ability = GetAbilityById(snapshot.abilityId))
				{
					const sas::AbilityHandle handle = snapshot.handle;
					try
					{
						ability->RefreshScopedConfiguration();
						if (IsClearPending()) break;
						NotifyAbilityChanged(handle);
						if (IsClearPending()) break;
					}
					catch (...)
					{
						if (!error) error = std::current_exception();
						if (IsClearPending()) break;
					}
				}
			}
		});
		if (error) std::rethrow_exception(error);
	}

	bool LightYearsAbilitySystemComponent::HasAbilityCallbackInProgress() const
	{
		if (IsExecutingAbilityInstanceOperation()) return true;
		for (const sas::AbilityRuntimeSnapshot& snapshot : BuildAbilitySnapshots())
		{
			if (const GameAbility* ability = GetAbility(snapshot.handle);
				ability && ability->IsInCallbackScope())
			{
				return true;
			}
		}
		return false;
	}

	void LightYearsAbilitySystemComponent::FlushDeferredScopedAbilityRules()
	{
		if (!mHasDeferredScopedAbilityRules || mRefreshingScopedAbilityRules ||
			IsClearPending() || HasAbilityCallbackInProgress())
		{
			return;
		}
		std::exception_ptr error;
		while (mHasDeferredScopedAbilityRules && !IsClearPending() &&
			!HasAbilityCallbackInProgress())
		{
			mScopedAbilityRules.swap(mDeferredScopedAbilityRules);
			mDeferredScopedAbilityRules.clear();
			mHasDeferredScopedAbilityRules = false;
			try { RefreshScopedAbilityRules(); }
			catch (...) { if (!error) error = std::current_exception(); }
		}
		if (error) std::rethrow_exception(error);
	}

	void LightYearsAbilitySystemComponent::ProcessGameGameplayEvent(
		const sas::AbilityEvent& event
	)
	{
		return RunOperation([&]()
		{
			if (HasOwnedTag(GameplayTags::State::Effect::Control::Stunned))
			{
				return;
			}

			mComponentRuntime.HandleGameplayEvent(
				event,
				GetOwnedTags(),
				[](GameAbility& ability, const sas::AbilityEvent& abilityEvent)
				{
					ability.HandleGameplayEvent(abilityEvent);
					ability.HandleAttachmentEvent(abilityEvent);
				},
				[this](
					GameAbility& ability,
					const GameAbilityDefinition& definition,
					const AbilityTriggerSpec& trigger,
					const sas::AbilityEvent& abilityEvent
				)
				{
					ExecuteTriggeredActions(
						*this,
						ability,
						definition,
						trigger,
						abilityEvent
					);
				}
			);
		});
	}

	void LightYearsAbilitySystemComponent::HandleAbilityLifecycleEvent(
		const sas::AbilityLifecycleEvent& event
	)
	{
		return RunOperation([&]()
		{
			onGameplayEvent.Broadcast(event);
			ProcessAbilityLifecycleEvent(event);
		});
	}

	void LightYearsAbilitySystemComponent::ProcessAbilityLifecycleEvent(
		const sas::AbilityLifecycleEvent& event
	)
	{
		// History is populated before observers run so an observer can consume the
		// newly recorded action immediately. Echo/system invocations are excluded
		// by origin, preventing recursive history pollution.
		RecordAbilityActivation(event);
		mLifecycleDispatcher.Publish(event);

		if (HasOwnedTag(GameplayTags::State::Effect::Control::Stunned))
		{
			return;
		}

		mComponentRuntime.HandleGameplayEvent(
			event,
			GetOwnedTags(),
			[](GameAbility& ability, const sas::AbilityLifecycleEvent& abilityEvent)
			{
				ability.HandleAbilityLifecycleEvent(abilityEvent);
			},
			[this](
				GameAbility& ability,
				const GameAbilityDefinition& definition,
				const AbilityTriggerSpec& trigger,
				const sas::AbilityLifecycleEvent& abilityEvent
			)
			{
				ExecuteTriggeredActions(
					*this,
					ability,
					definition,
					trigger,
					abilityEvent
				);
			}
		);
	}

	void LightYearsAbilitySystemComponent::RecordAbilityActivation(
		const sas::AbilityLifecycleEvent& event
	)
	{
		if (event.eventTag != GameplayTags::Event::Ability::Activated ||
			event.activationOrigin != sas::AbilityActivationOrigin::NormalInput ||
			!event.abilityHandle.IsValid())
		{
			return;
		}

		const GameAbility* ability = GetAbility(event.abilityHandle);
		if (!ability)
		{
			return;
		}

		const GameAbilityDefinition& definition = ability->GetDefinition();
		if (!definition.recordInAbilityHistory ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy == sas::AbilityActivationPolicy::Passive ||
			definition.activationPolicy == sas::AbilityActivationPolicy::GameplayEvent)
		{
			return;
		}

		AbilityUseRecord record;
		record.abilityHandle = event.abilityHandle;
		record.abilityId = event.abilityId;
		record.slot = definition.slot;
		record.level = ability->GetLevel();
		record.maxLevel = ability->GetMaxLevel();
		record.abilityTags = definition.abilityTags;
		record.unlockedUpgradeIds = definition.unlockedUpgradeIds;
		record.activationOrigin = event.activationOrigin;
		for (const sas::AttributeScalingRule& rule : definition.scalingRules)
		{
			record.scalingChannels.push_back(
				AbilityScalingChannel{
					rule.targetAttributeId,
					rule.sourceAttributeId,
					rule.operation,
					rule.coefficient
				}
			);
		}
		mAbilityUseHistory.Record(std::move(record));
	}
}
