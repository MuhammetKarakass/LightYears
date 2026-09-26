#include "AbilitySystemComponent.h"

#include <utility>
#include <exception>
#include <stdexcept>

namespace sas
{
	// Keep this translation unit coupled to the effect-runtime layout because
	// AbilitySystemComponent stores GameplayEffectSystem by value.
	AbilitySystemComponent::AbilitySystemComponent()
		: mAttributes{},
		mOwnedTags{},
		mEffects{ mAttributes, mOwnedTags }
	{
		RebuildEffectRuntimeCallbacks();
	}

	bool AbilitySystemComponent::RemoveAbility(
		AbilityHandle handle,
		AbilityEndReason reason
	)
	{
		return RunOperation([&]() -> bool
		{
			return mAbilityRuntime &&
				mAbilityRuntime->RemoveAbility(handle, reason);
		});
	}

	bool AbilitySystemComponent::RebindAbility(
		AbilityHandle handle,
		AbilitySlot targetSlot,
		std::string* failureReason
	)
	{
		const bool rebound = RunOperation([&]() -> bool
		{
			return mAbilityRuntime &&
				mAbilityRuntime->RebindAbility(
					handle,
					AbilityRuntimeBinding{ targetSlot },
					failureReason
				);
		});
		return rebound && !IsClearPending() && mAbilityRuntime && mAbilityRuntime->FindAbility(handle);
	}

	void AbilitySystemComponent::ClearAbilitySlot(AbilitySlot slot)
	{
		return RunOperation([&]()
		{
			if (mAbilityRuntime)
			{
				mAbilityRuntime->ClearSlot(slot);
			}
		});
	}

	void AbilitySystemComponent::SetAbilitySlotInput(
		AbilitySlot slot,
		bool inputHeld
	)
	{
		return RunOperation([&]()
		{
			if (mAbilityRuntime)
			{
				mAbilityRuntime->SetSlotInput(slot, inputHeld);
			}
		});
	}

	bool AbilitySystemComponent::SetAbilityLevel(
		AbilityHandle handle,
		int level
	)
	{
		return RunOperation([&]() -> bool
		{
			return mAbilityRuntime &&
				mAbilityRuntime->SetAbilityLevel(handle, level);
		});
	}

	bool AbilitySystemComponent::SetAbilityLevel(
		AbilitySlot slot,
		int level
	)
	{
		return RunOperation([&]() -> bool
		{
			return mAbilityRuntime &&
				mAbilityRuntime->SetAbilityLevel(slot, level);
		});
	}

	bool AbilitySystemComponent::LevelUpAbility(AbilityHandle handle)
	{
		return RunOperation([&]() -> bool
		{
			return mAbilityRuntime &&
				mAbilityRuntime->LevelUpAbility(handle);
		});
	}

	bool AbilitySystemComponent::LevelUpAbility(AbilitySlot slot)
	{
		return RunOperation([&]() -> bool
		{
			return mAbilityRuntime &&
				mAbilityRuntime->LevelUpAbility(slot);
		});
	}

	void AbilitySystemComponent::ReduceAbilityCooldowns(
		float amount,
		bool includePrimaryFire
	)
	{
		return RunOperation([&]()
		{
			if (mAbilityRuntime)
			{
				mAbilityRuntime->ReduceCooldowns(
					amount,
					includePrimaryFire
				);
			}
		});
	}

	ly::List<AbilityRuntimeSnapshot>
	AbilitySystemComponent::BuildAbilitySnapshots() const
	{
		return mAbilityRuntime
			? mAbilityRuntime->BuildSnapshots()
			: ly::List<AbilityRuntimeSnapshot>{};
	}

	const ly::List<AbilityHandle>&
	AbilitySystemComponent::GetPassiveAbilities() const
	{
		static const ly::List<AbilityHandle> Empty;
		return mAbilityRuntime
			? mAbilityRuntime->GetPassiveAbilities()
			: Empty;
	}

	AbilityInstanceNotifications
	AbilitySystemComponent::CreateAbilityInstanceNotifications(bool emitNotifications)
	{
		AbilityInstanceNotifications notifications{
			[this](AbilityHandle handle)
			{
				NotifyAbilityChanged(handle);
			},
			[this](AbilityHandle handle)
			{
				onAbilityActivated.Broadcast(handle);
				NotifyAbilityChanged(handle);
			},
			[this](AbilityHandle handle, AbilityEndReason reason)
			{
				onAbilityEnded.Broadcast(handle, reason);
				NotifyAbilityChanged(handle);
			},
			[this](AbilityHandle handle, int level)
			{
				onAbilityLevelChanged.Broadcast(handle, level);
				NotifyAbilityChanged(handle);
			}
		};
		if (!emitNotifications) notifications = {};
		notifications.execute = [this](const std::function<void()>& operation) { ExecuteInstanceOperation(operation); };
		notifications.canExecute = [this] { return !IsClearPending(); };
		return notifications;
	}

	void AbilitySystemComponent::NotifyAbilityChanged(AbilityHandle handle)
	{
		RefreshCooldownTags();
		onAbilityChanged.Broadcast(handle);
	}

	void AbilitySystemComponent::RefreshCooldownTags()
	{
		bool abilityCooldownActive = false;
		bool primaryWeaponCooldownActive = false;
		if (mAbilityRuntime)
		{
			for (const AbilityRuntimeSnapshot& snapshot :
				mAbilityRuntime->BuildSnapshots())
			{
				if (snapshot.cooldownRemaining <= 0.f)
				{
					continue;
				}
				if (snapshot.slot == AbilitySlot::PrimaryFire)
				{
					primaryWeaponCooldownActive = true;
				}
				else
				{
					abilityCooldownActive = true;
				}
			}
		}

		if (abilityCooldownActive != mAbilityCooldownTagActive)
		{
			if (abilityCooldownActive)
			{
				mOwnedTags.AddTag(AbilityCooldownTags::Ability);
			}
			else
			{
				mOwnedTags.RemoveTag(AbilityCooldownTags::Ability);
			}
			mAbilityCooldownTagActive = abilityCooldownActive;
		}

		if (primaryWeaponCooldownActive != mPrimaryWeaponCooldownTagActive)
		{
			if (primaryWeaponCooldownActive)
			{
				mOwnedTags.AddTag(AbilityCooldownTags::PrimaryWeapon);
			}
			else
			{
				mOwnedTags.RemoveTag(AbilityCooldownTags::PrimaryWeapon);
			}
			mPrimaryWeaponCooldownTagActive = primaryWeaponCooldownActive;
		}
	}

	void AbilitySystemComponent::AddOwnedTag(const ly::GameplayTag& tag)
	{
		return RunOperation([&]()
		{
			if (IsClearPending()) return;
			mOwnedTags.AddTag(tag);
		});
	}

	void AbilitySystemComponent::RemoveOwnedTag(const ly::GameplayTag& tag)
	{
		return RunOperation([&]()
		{
			mOwnedTags.RemoveTag(tag);
		});
	}

	bool AbilitySystemComponent::HasOwnedTag(
		const ly::GameplayTag& tag,
		bool exactMatch
	) const
	{
		return mOwnedTags.HasTag(tag, exactMatch);
	}

	bool AbilitySystemComponent::HasAllOwnedTags(
		const ly::List<ly::GameplayTag>& tags
	) const
	{
		return mOwnedTags.HasAll(tags);
	}

	bool AbilitySystemComponent::HasAnyOwnedTags(
		const ly::List<ly::GameplayTag>& tags
	) const
	{
		return mOwnedTags.HasAny(tags);
	}

	void AbilitySystemComponent::SetEffectRuntimeCallbacks(
		EffectCallbacks callbacks
	)
	{
		mEffectBindings = std::move(callbacks);
		RebuildEffectRuntimeCallbacks();
	}

	void AbilitySystemComponent::RebuildEffectRuntimeCallbacks()
	{
		EffectCallbacks callbacks = mEffectBindings;

		auto changed = std::move(callbacks.changed);
		callbacks.changed =
			[this, callback = std::move(changed)](
				ActiveGameplayEffect& effect
			)
			{
				if (callback)
				{
					callback(effect);
				}
				if (!IsClearPending()) onGameplayEffectChanged.Broadcast(effect.handle);
			};

		auto applied = std::move(callbacks.applied);
		callbacks.applied =
			[this, callback = std::move(applied)](
				GameplayEffectHandle handle
			)
			{
				if (callback)
				{
					callback(handle);
				}
				if (!IsClearPending()) onGameplayEffectApplied.Broadcast(handle);
			};

		auto removed = std::move(callbacks.removed);
		callbacks.removed =
			[this, callback = std::move(removed)](
				GameplayEffectHandle handle
			)
			{
				if (callback)
				{
					callback(handle);
				}
				if (!IsClearPending()) onGameplayEffectRemoved.Broadcast(handle);
			};

		auto collectionChanged = std::move(callbacks.collectionChanged);
		callbacks.collectionChanged =
			[this, callback = std::move(collectionChanged)]()
			{
				if (callback)
				{
					callback();
				}
				if (!IsClearPending()) onGameplayEffectsChanged.Broadcast();
			};

		callbacks.shouldContinue = [this] { return !IsClearPending(); };
		mEffects.SetCallbacks(std::move(callbacks));
	}

	GameplayEffectHandle AbilitySystemComponent::ApplyGameplayEffect(
		const GameplayEffectDefinition& definition,
		const GameplayEffectSourceContext& context
	)
	{
		return RunOperation([&]() -> GameplayEffectHandle
		{
			if (IsClearPending()) return {};
			return mEffects.ApplyEffect(definition, context);
		});
	}

	GameplayEffectHandle AbilitySystemComponent::ApplyGameplayEffect(
		const GameplayEffectSpec& spec,
		const GameplayEffectSourceContext& context
	)
	{
		return RunOperation([&]() -> GameplayEffectHandle
		{
			if (IsClearPending()) return {};
			return mEffects.ApplyEffect(spec, context);
		});
	}

	bool AbilitySystemComponent::RefreshGameplayEffectDuration(
		GameplayEffectHandle handle
	)
	{
		return RunOperation([&]() -> bool
		{
			return mEffects.RefreshEffectDuration(handle);
		});
	}

	void AbilitySystemComponent::RemoveGameplayEffect(
		GameplayEffectHandle handle
	)
	{
		return RunOperation([&]()
		{
			mEffects.RemoveEffect(handle);
		});
	}

	std::size_t AbilitySystemComponent::RemoveGameplayEffectsIf(
		const std::function<bool(const ActiveGameplayEffect&)>& predicate
	)
	{
		return RunOperation([&]()
		{
			return mEffects.RemoveEffectsIf(predicate);
		});
	}

	ActiveGameplayEffect* AbilitySystemComponent::FindGameplayEffect(
		GameplayEffectHandle handle
	)
	{
		return mEffects.FindEffect(handle);
	}

	const ActiveGameplayEffect* AbilitySystemComponent::FindGameplayEffect(
		GameplayEffectHandle handle
	) const
	{
		return mEffects.FindEffect(handle);
	}

	ActiveGameplayEffect* AbilitySystemComponent::FindGameplayEffectById(
		const std::string& effectId
	)
	{
		return mEffects.FindEffectById(effectId);
	}

	const ActiveGameplayEffect* AbilitySystemComponent::FindGameplayEffectById(
		const std::string& effectId
	) const
	{
		return mEffects.FindEffectById(effectId);
	}

	ly::List<GameplayEffectRuntimeSnapshot>
	AbilitySystemComponent::BuildGameplayEffectSnapshots() const
	{
		return mEffects.BuildSnapshots();
	}

	bool AbilitySystemComponent::CanApplyGameplayEffect(
		const GameplayEffectDefinition& definition
	) const
	{
		return !IsClearPending() && mEffects.CanApplyEffect(definition);
	}

	void AbilitySystemComponent::Tick(float deltaTime)
	{
		return RunOperation([&]()
		{
			if (IsClearPending()) return;
			mEffects.Tick(deltaTime);
			if (!IsClearPending() && mAbilityRuntime)
			{
				mAbilityRuntime->Tick(deltaTime);
			}
		});
	}

	void AbilitySystemComponent::Clear()
	{
		if (mClearing) return;
		mClearRequested = true;
		if (mOperationDepth != 0) return;
		mClearing = true;
		std::exception_ptr error;
		const auto cleanup = [&error](auto&& operation)
		{
			try { operation(); }
			catch (...) { if (!error) error = std::current_exception(); }
		};
		cleanup([&] { if (mAbilityRuntime) mAbilityRuntime->Clear(); });
		const bool runtimeRetainedAbilities =
			mAbilityRuntime && mAbilityRuntime->HasAbilityInstances();
		if (runtimeRetainedAbilities)
		{
			if (!error)
			{
				error = std::make_exception_ptr(std::runtime_error(
					"Ability cleanup is incomplete; retry Clear after retained cleanup succeeds."
				));
			}
			cleanup([&] { ClearAdditionalState(true); });
			mClearing = false;
			// Keep clear requested and preserve attributes, tags, effects, and the
			// game-owned weapon override state until retained EndFire debt is drained.
			std::rethrow_exception(error);
		}
		try { ClearAdditionalState(false); }
		catch (...)
		{
			if (!error) error = std::current_exception();
			mClearing = false;
			std::rethrow_exception(error);
		}
		cleanup([&] { mEffects.Clear(); });
		cleanup([&] { mOwnedTags.Clear(); });
		cleanup([&] { mAttributes.Clear(); });
		if (!error) cleanup([&] { OnClearCompleted(); });
		mAbilityCooldownTagActive = false;
		mPrimaryWeaponCooldownTagActive = false;
		mClearRequested = false;
		mClearing = false;
		if (error) std::rethrow_exception(error);
	}

	void AbilitySystemComponent::ExecuteOperation(const std::function<void()>& operation)
	{
		++mOperationDepth;
		std::exception_ptr error;
		try { operation(); }
		catch (...) { error = std::current_exception(); }
		--mOperationDepth;
		if (mOperationDepth == 0 && mClearRequested && !mClearing)
		{
			try { Clear(); }
			catch (...) { if (!error) error = std::current_exception(); }
		}
		if (error) std::rethrow_exception(error);
	}

	void AbilitySystemComponent::ExecuteInstanceOperation(const std::function<void()>& operation)
	{
		ExecuteOperation([&]
		{
			++mInstanceExecutionDepth;
			std::exception_ptr error;
			try { operation(); }
			catch (...) { error = std::current_exception(); }
			--mInstanceExecutionDepth;
			if (mInstanceExecutionDepth == 0)
			{
				try { OnAbilityInstanceOperationsCompleted(); }
				catch (...) { if (!error) error = std::current_exception(); }
			}
			if (error) std::rethrow_exception(error);
		});
	}
}
