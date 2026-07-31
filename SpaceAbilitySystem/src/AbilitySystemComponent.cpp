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
		onAbilityChanged.Broadcast(handle);
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
