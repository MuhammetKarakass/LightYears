#include "AbilitySystemComponent.h"

#include <utility>

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
		return mAbilityRuntime &&
			mAbilityRuntime->RemoveAbility(handle, reason);
	}

	bool AbilitySystemComponent::RebindAbility(
		AbilityHandle handle,
		AbilitySlot targetSlot,
		std::string* failureReason
	)
	{
		return mAbilityRuntime &&
			mAbilityRuntime->RebindAbility(
				handle,
				AbilityRuntimeBinding{ targetSlot },
				failureReason
			);
	}

	void AbilitySystemComponent::ClearAbilitySlot(AbilitySlot slot)
	{
		if (mAbilityRuntime)
		{
			mAbilityRuntime->ClearSlot(slot);
		}
	}

	void AbilitySystemComponent::SetAbilitySlotInput(
		AbilitySlot slot,
		bool inputHeld
	)
	{
		if (mAbilityRuntime)
		{
			mAbilityRuntime->SetSlotInput(slot, inputHeld);
		}
	}

	bool AbilitySystemComponent::SetAbilityLevel(
		AbilityHandle handle,
		int level
	)
	{
		return mAbilityRuntime &&
			mAbilityRuntime->SetAbilityLevel(handle, level);
	}

	bool AbilitySystemComponent::SetAbilityLevel(
		AbilitySlot slot,
		int level
	)
	{
		return mAbilityRuntime &&
			mAbilityRuntime->SetAbilityLevel(slot, level);
	}

	bool AbilitySystemComponent::LevelUpAbility(AbilityHandle handle)
	{
		return mAbilityRuntime &&
			mAbilityRuntime->LevelUpAbility(handle);
	}

	bool AbilitySystemComponent::LevelUpAbility(AbilitySlot slot)
	{
		return mAbilityRuntime &&
			mAbilityRuntime->LevelUpAbility(slot);
	}

	void AbilitySystemComponent::ReduceAbilityCooldowns(
		float amount,
		bool includePrimaryFire
	)
	{
		if (mAbilityRuntime)
		{
			mAbilityRuntime->ReduceCooldowns(
				amount,
				includePrimaryFire
			);
		}
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
	AbilitySystemComponent::CreateAbilityInstanceNotifications()
	{
		return AbilityInstanceNotifications{
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
		mOwnedTags.AddTag(tag);
	}

	void AbilitySystemComponent::RemoveOwnedTag(const ly::GameplayTag& tag)
	{
		mOwnedTags.RemoveTag(tag);
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
				onGameplayEffectChanged.Broadcast(effect.handle);
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
				onGameplayEffectApplied.Broadcast(handle);
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
				onGameplayEffectRemoved.Broadcast(handle);
			};

		auto collectionChanged = std::move(callbacks.collectionChanged);
		callbacks.collectionChanged =
			[this, callback = std::move(collectionChanged)]()
			{
				if (callback)
				{
					callback();
				}
				onGameplayEffectsChanged.Broadcast();
			};

		mEffects.SetCallbacks(std::move(callbacks));
	}

	GameplayEffectHandle AbilitySystemComponent::ApplyGameplayEffect(
		const GameplayEffectDefinition& definition,
		const GameplayEffectSourceContext& context
	)
	{
		return mEffects.ApplyEffect(definition, context);
	}

	GameplayEffectHandle AbilitySystemComponent::ApplyGameplayEffect(
		const GameplayEffectSpec& spec,
		const GameplayEffectSourceContext& context
	)
	{
		return mEffects.ApplyEffect(spec, context);
	}

	bool AbilitySystemComponent::RefreshGameplayEffectDuration(
		GameplayEffectHandle handle
	)
	{
		return mEffects.RefreshEffectDuration(handle);
	}

	void AbilitySystemComponent::RemoveGameplayEffect(
		GameplayEffectHandle handle
	)
	{
		mEffects.RemoveEffect(handle);
	}

	std::size_t AbilitySystemComponent::RemoveGameplayEffectsIf(
		const std::function<bool(const ActiveGameplayEffect&)>& predicate
	)
	{
		return mEffects.RemoveEffectsIf(predicate);
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
		return mEffects.CanApplyEffect(definition);
	}

	void AbilitySystemComponent::Tick(float deltaTime)
	{
		mEffects.Tick(deltaTime);
		if (mAbilityRuntime)
		{
			mAbilityRuntime->Tick(deltaTime);
		}
	}

	void AbilitySystemComponent::Clear()
	{
		if (mAbilityRuntime)
		{
			mAbilityRuntime->Clear();
		}
		mEffects.Clear();
		mOwnedTags.Clear();
		mAttributes.Clear();
	}
}
