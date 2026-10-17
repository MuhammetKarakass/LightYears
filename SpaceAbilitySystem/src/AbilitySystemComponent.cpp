#include "AbilitySystemComponent.h"

#include <utility>
#include <exception>
#include <stdexcept>

namespace sas
{
	namespace
	{
		// Instant effects have no active entry. Keep nested application IDs available
		// to the existing lifecycle callbacks without retaining definition pointers.
		struct DebugApplication
		{
			const AbilitySystemComponent* owner;
			std::string id;
			unsigned int instantHandle = 0;
		};
		thread_local std::vector<DebugApplication> DebugApplications;
		struct DebugApplicationScope
		{
			bool active = AbilitySystemDebugView::recording;
			DebugApplicationScope(const AbilitySystemComponent* owner, const std::string& id)
			{
				if (active) DebugApplications.push_back({ owner, id });
			}
			~DebugApplicationScope() { if (active) DebugApplications.pop_back(); }
		};
		std::string DebugApplicationId(const AbilitySystemComponent* owner)
		{
			for (auto entry = DebugApplications.rbegin(); entry != DebugApplications.rend(); ++entry)
				if (entry->owner == owner) return entry->id;
			return "unknown";
		}
		void MarkDebugInstantApplication(const AbilitySystemComponent* owner, GameplayEffectHandle handle)
		{
			for (auto entry = DebugApplications.rbegin(); entry != DebugApplications.rend(); ++entry)
			{
				if (entry->owner != owner) continue;
				entry->instantHandle = handle.id;
				return;
			}
		}
		std::string DebugRemovedEffectId(const AbilitySystemComponent* owner, GameplayEffectHandle handle)
		{
			for (auto entry = DebugApplications.rbegin(); entry != DebugApplications.rend(); ++entry)
				if (entry->owner == owner && entry->instantHandle == handle.id) return entry->id;
			return std::to_string(handle.id);
		}
	}
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
		return SetAbilityLevel(handle, level, nullptr, nullptr);
	}

	bool AbilitySystemComponent::SetAbilityLevel(
		AbilityHandle handle,
		int level,
		AbilityLevelCommitSink commitSink,
		void* commitContext
	)
	{
		return RunOperation([&]() -> bool
		{
			return mAbilityRuntime &&
				mAbilityRuntime->SetAbilityLevel(handle, level, commitSink, commitContext);
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

	bool AbilitySystemComponent::SetAbilityChargeBonus(AbilitySlot slot, int bonus)
	{
		return RunOperation([&]() -> bool
		{
			return mAbilityRuntime &&
				mAbilityRuntime->SetAbilityChargeBonus(slot, bonus);
		});
	}

	bool AbilitySystemComponent::SetAbilityChargeBonus(const std::string& abilityId, int bonus)
	{
		return RunOperation([&]() -> bool
		{
			return mAbilityRuntime &&
				mAbilityRuntime->SetAbilityChargeBonus(abilityId, bonus);
		});
	}

	int AbilitySystemComponent::GrantAbilityCharge(AbilitySlot slot, int amount)
	{
		return RunOperation([&]() -> int
		{
			return mAbilityRuntime ? mAbilityRuntime->GrantAbilityCharge(slot, amount) : 0;
		});
	}

	int AbilitySystemComponent::GrantAbilityCharge(const std::string& abilityId, int amount)
	{
		return RunOperation([&]() -> int
		{
			return mAbilityRuntime ? mAbilityRuntime->GrantAbilityCharge(abilityId, amount) : 0;
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

	vega::List<AbilityRuntimeSnapshot>
	AbilitySystemComponent::BuildAbilitySnapshots() const
	{
		return mAbilityRuntime
			? mAbilityRuntime->BuildSnapshots()
			: vega::List<AbilityRuntimeSnapshot>{};
	}

	const vega::List<AbilityHandle>&
	AbilitySystemComponent::GetPassiveAbilities() const
	{
		static const vega::List<AbilityHandle> Empty;
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
				if (AbilitySystemDebugView::recording)
				{
					for (const auto& ability : BuildAbilitySnapshots())
						if (ability.handle == handle) RecordDebugEvent("Activated " + ability.abilityId);
				}
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
			mAbilityRuntime->GetCooldownActivity(abilityCooldownActive, primaryWeaponCooldownActive);
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

	void AbilitySystemComponent::AddOwnedTag(const vega::GameplayTag& tag)
	{
		return RunOperation([&]()
		{
			if (IsClearPending()) return;
			mOwnedTags.AddTag(tag);
		});
	}

	void AbilitySystemComponent::RemoveOwnedTag(const vega::GameplayTag& tag)
	{
		return RunOperation([&]()
		{
			mOwnedTags.RemoveTag(tag);
		});
	}

	bool AbilitySystemComponent::HasOwnedTag(
		const vega::GameplayTag& tag,
		bool exactMatch
	) const
	{
		return mOwnedTags.HasTag(tag, exactMatch);
	}

	bool AbilitySystemComponent::HasAllOwnedTags(
		const vega::List<vega::GameplayTag>& tags
	) const
	{
		return mOwnedTags.HasAll(tags);
	}

	bool AbilitySystemComponent::HasAnyOwnedTags(
		const vega::List<vega::GameplayTag>& tags
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
				if (AbilitySystemDebugView::recording)
				{
					const auto* effect = FindGameplayEffect(handle);
					const auto id = effect ? effect->spec.definition.effectId : DebugApplicationId(this);
					RecordDebugEvent("Effect applied " + id);
					// Instant removal runs in this application scope, even though no active
					// entry exists. Its identity must not survive an interrupted callback.
					if (effect) mDebugEffectIds[handle.id] = id;
					else MarkDebugInstantApplication(this, handle);
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
				if (AbilitySystemDebugView::recording)
				{
					const auto found = mDebugEffectIds.find(handle.id);
					const auto id = found == mDebugEffectIds.end() ? DebugRemovedEffectId(this, handle) : found->second;
					RecordDebugEvent("Effect removed " + id);
				}
				mDebugEffectIds.erase(handle.id);
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
			DebugApplicationScope debugScope{ this, definition.effectId };
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
			DebugApplicationScope debugScope{ this, spec.definition.effectId };
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

	vega::List<GameplayEffectRuntimeSnapshot>
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

	ClearResult AbilitySystemComponent::Clear()
	{
		if (mClearing) return ClearResult::Pending;
		mClearRequested = true;
		if (mOperationDepth != 0) return ClearResult::Pending;
		mClearing = true;
		std::exception_ptr error;
		const auto cleanup = [&error](auto&& operation)
		{
			try { operation(); }
			catch (...) { if (!error) error = std::current_exception(); }
		};
		// Keeps the clear requested so the next outermost operation retries it. A callback exception
		// captured on the way still propagates instead of being reported as plain Pending.
		const auto retainPending = [&]
		{
			mClearing = false;
			if (error) std::rethrow_exception(error);
			return ClearResult::Pending;
		};
		ClearResult abilityResult = ClearResult::Completed;
		cleanup([&] { if (mAbilityRuntime) abilityResult = mAbilityRuntime->Clear(); });
		const bool runtimeRetainedAbilities = abilityResult == ClearResult::Pending ||
			(mAbilityRuntime && mAbilityRuntime->HasAbilityInstances());
		if (runtimeRetainedAbilities)
		{
			cleanup([&] { ClearAdditionalState(true); });
			// Keep clear requested and preserve attributes, tags, effects, and the
			// game-owned weapon override state until retained EndFire debt is drained.
			return retainPending();
		}
		bool additionalStateError = false;
		try { ClearAdditionalState(true); }
		catch (...) { additionalStateError = true; if (!error) error = std::current_exception(); }
		std::optional<bool> pendingAdditionalCleanup;
		try { pendingAdditionalCleanup = GetPendingAdditionalCleanup(); }
		catch (...) { additionalStateError = true; if (!error) error = std::current_exception(); }
		if (pendingAdditionalCleanup.value_or(additionalStateError)) return retainPending();
		ClearResult effectsResult = ClearResult::Completed;
		cleanup([&] { effectsResult = mEffects.Clear(); });
		bool retainedEffects = true;
		try { retainedEffects = effectsResult == ClearResult::Pending || !mEffects.GetHandles().empty(); }
		catch (...) { if (!error) error = std::current_exception(); }
		if (retainedEffects) return retainPending();
		cleanup([&] { ReleaseAdditionalRuntimeDependencies(); });
		cleanup([&] { mOwnedTags.Clear(); });
		cleanup([&] { mAttributes.Clear(); });
		cleanup([&] { OnClearCompleted(); });
		mAbilityCooldownTagActive = false;
		mPrimaryWeaponCooldownTagActive = false;
		mClearRequested = false;
		mClearing = false;
		if (error) std::rethrow_exception(error);
		return ClearResult::Completed;
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
			try { static_cast<void>(Clear()); }
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

namespace sas
{
	void AbilitySystemComponent::RecordDebugEvent(const std::string& message)
	{
		if (!AbilitySystemDebugView::recording) return;
		if (mDebugSession != AbilitySystemDebugView::session)
		{
			mDebugSession = AbilitySystemDebugView::session;
			mDebugEvents.clear();
			// Active effect identities outlive a recording session. Removal retires them,
			// including when recording is disabled, so they never retain instant effects.
		}
		if (mDebugEvents.size() == 100) mDebugEvents.pop_front();
		mDebugEvents.push_back({ std::chrono::duration<double>(std::chrono::steady_clock::now() - AbilitySystemDebugView::epoch).count(), message });
	}

	AbilitySystemDebugSnapshot AbilitySystemComponent::BuildDebugSnapshot() const
	{
		AbilitySystemDebugSnapshot result;
		for (const auto& ability : BuildAbilitySnapshots())
		{
			AbilityDebugEntry entry;
			entry.runtime = ability;
			entry.displayName = ability.abilityId;
			const auto& passives = GetPassiveAbilities();
			entry.passive = std::find(passives.begin(), passives.end(), ability.handle) != passives.end();
			DescribeDebugAbility(entry);
			result.abilities.push_back(std::move(entry));
		}
		result.effects = mEffects.BuildDebugSnapshots();
		result.attributes = mAttributes.BuildSnapshot();
		result.tags = mOwnedTags.BuildCountSnapshot();
		std::sort(result.tags.begin(), result.tags.end(), [](const auto& a, const auto& b) { return a.first.name < b.first.name; });
		if (mDebugSession == AbilitySystemDebugView::session) result.events.assign(mDebugEvents.begin(), mDebugEvents.end());
		return result;
	}
}
