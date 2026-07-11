#include "gameplay/effects/GameplayEffectSystem.h"
#include "gameplay/effects/GameplayEffectBehavior.h"
#include "gameplay/attributes/AttributeSystem.h"
#include "framework/Actor.h"
#include "presentation/effects/GameplayEffectVisual.h"
#include "presentation/effects/GameplayEffectVisualRegistry.h"
#include <algorithm>

namespace ly
{
	GameplayEffectSystem::GameplayEffectSystem(
		Actor& owner,
		AttributeSystem& attributes,
		GameplayTagContainer& ownedTags
	)
		: mOwner{ &owner },
		mAttributes{ &attributes },
		mOwnedTags{ &ownedTags }
	{
	}

	GameplayEffectHandle GameplayEffectSystem::ApplyEffect(const GameplayEffectDefinition& definition, Actor* source)
	{
		if (!CanApplyEffect(definition))
		{
			return {};
		}

		const GameplayEffectHandle newHandle{ mNextHandleId++ };
		if (definition.durationPolicy == GameplayEffectDurationPolicy::Instant)
		{
			if (mAttributes)
			{
				for (const AttributeModifier& modifier : definition.modifiers)
				{
					mAttributes->ApplyBaseModifier(modifier);
				}
			}
			onEffectApplied.Broadcast(newHandle);
			onEffectRemoved.Broadcast(newHandle);
			onEffectsChanged.Broadcast();
			return newHandle;
		}

		if (definition.stackingPolicy != GameplayEffectStackingPolicy::None)
		{
			for (ActiveGameplayEffect& activeEffect : mActiveEffects)
			{
				if (activeEffect.definition.effectId != definition.effectId)
				{
					continue;
				}

				if (definition.stackingPolicy == GameplayEffectStackingPolicy::RefreshDuration)
				{
					RemoveGrantedTags(activeEffect);
					RemoveModifiers(activeEffect);
					DestroyVisual(activeEffect);
					activeEffect.definition = definition;
					activeEffect.source = source;
					activeEffect.stackCount = 1;
					GameplayEffectBehavior::Refresh(activeEffect);
					ApplyModifiers(activeEffect);
					GrantTags(activeEffect);
					SpawnVisual(activeEffect);
				}
				else if (definition.stackingPolicy == GameplayEffectStackingPolicy::Stack)
				{
					activeEffect.definition = definition;
					activeEffect.source = source;
					if (activeEffect.stackCount < std::max(1, definition.maxStacks))
					{
						++activeEffect.stackCount;
						ApplyModifiers(activeEffect);
						GameplayEffectBehavior::AddStack(activeEffect);
					}
				}

				activeEffect.remainingDuration = definition.duration;
				activeEffect.totalDuration = definition.duration;
				SynchronizeVisual(activeEffect);
				onEffectChanged.Broadcast(activeEffect.handle);
				onEffectsChanged.Broadcast();
				return activeEffect.handle;
			}
		}

		ActiveGameplayEffect effect;
		effect.handle = newHandle;
		effect.definition = definition;
		effect.remainingDuration = definition.duration;
		effect.totalDuration = definition.duration;
		effect.source = source;
		GameplayEffectBehavior::Initialize(effect);
		ApplyModifiers(effect);
		GrantTags(effect);
		SpawnVisual(effect);
		mActiveEffects.push_back(effect);
		onEffectApplied.Broadcast(newHandle);
		onEffectsChanged.Broadcast();
		return newHandle;
	}

	void GameplayEffectSystem::RemoveEffect(GameplayEffectHandle handle)
	{
		for (size_t i = 0; i < mActiveEffects.size(); ++i)
		{
			if (mActiveEffects[i].handle == handle)
			{
				RemoveEffectAt(i);
				return;
			}
		}
	}

	void GameplayEffectSystem::Tick(float deltaTime)
	{
		for (size_t i = 0; i < mActiveEffects.size();)
		{
			ActiveGameplayEffect& effect = mActiveEffects[i];
			if (effect.definition.durationPolicy == GameplayEffectDurationPolicy::Duration)
			{
				effect.remainingDuration -= deltaTime;
				if (effect.remainingDuration <= 0.f)
				{
					RemoveEffectAt(i);
					continue;
				}
				onEffectChanged.Broadcast(effect.handle);
				onEffectsChanged.Broadcast();
			}
			SynchronizeVisual(effect);
			++i;
		}
	}

	void GameplayEffectSystem::Clear()
	{
		while (!mActiveEffects.empty())
		{
			RemoveEffectAt(mActiveEffects.size() - 1);
		}
	}

	void GameplayEffectSystem::ProcessIncomingDamage(DamageContext& context)
	{
		for (size_t i = 0; i < mActiveEffects.size() && context.remainingDamage > 0.f;)
		{
			ActiveGameplayEffect& effect = mActiveEffects[i];
			const GameplayEffectBehaviorResult result = GameplayEffectBehavior::ProcessIncomingDamage(effect, context);
			if (result.changed)
			{
				SynchronizeVisual(effect);
				onEffectChanged.Broadcast(effect.handle);
				onEffectsChanged.Broadcast();
			}
			for (const GameplayEffectBehaviorEvent& event : result.events)
			{
				QueueOwnerEvent(event.eventTag, &context, event.magnitude);
			}
			if (result.removeEffect)
			{
				RemoveEffectAt(i);
				continue;
			}
			++i;
		}
	}

	List<AbilityEvent> GameplayEffectSystem::DrainPendingEvents()
	{
		List<AbilityEvent> events = std::move(mPendingEvents);
		mPendingEvents.clear();
		return events;
	}

	const ActiveGameplayEffect* GameplayEffectSystem::FindEffect(GameplayEffectHandle handle) const
	{
		for (const ActiveGameplayEffect& effect : mActiveEffects)
		{
			if (effect.handle == handle)
			{
				return &effect;
			}
		}
		return nullptr;
	}

	List<GameplayEffectSnapshot> GameplayEffectSystem::BuildSnapshots() const
	{
		List<GameplayEffectSnapshot> snapshots;
		for (const ActiveGameplayEffect& effect : mActiveEffects)
		{
			GameplayEffectSnapshot snapshot;
			snapshot.effectId = effect.definition.effectId;
			snapshot.remainingDuration = effect.remainingDuration;
			snapshot.totalDuration = effect.totalDuration;
			snapshot.stackCount = effect.stackCount;
			snapshot.runtimeAttributes = effect.runtimeAttributes;
			snapshots.push_back(std::move(snapshot));
		}
		return snapshots;
	}

	bool GameplayEffectSystem::CanApplyEffect(const GameplayEffectDefinition& definition) const
	{
		return mOwnedTags &&
			mOwnedTags->HasAll(definition.applicationRequiredTags) &&
			!mOwnedTags->HasAny(definition.applicationBlockedTags);
	}

	void GameplayEffectSystem::QueueOwnerEvent(const GameplayTag& eventTag, const DamageContext* context, float magnitude)
	{
		if (!mOwner)
		{
			return;
		}
		AbilityEvent event;
		event.eventTag = eventTag;
		event.source = context ? context->source : mOwner;
		event.target = mOwner;
		event.magnitude = magnitude;
		event.damageContext = context;
		mPendingEvents.push_back(event);
	}

	void GameplayEffectSystem::ApplyModifiers(ActiveGameplayEffect& effect)
	{
		if (!mAttributes)
		{
			return;
		}
		for (const AttributeModifier& modifier : effect.definition.modifiers)
		{
			const AttributeModifierHandle handle = mAttributes->AddModifier(modifier);
			if (handle.IsValid())
			{
				effect.appliedModifierHandles.push_back(handle);
			}
		}
	}

	void GameplayEffectSystem::RemoveModifiers(ActiveGameplayEffect& effect)
	{
		if (mAttributes)
		{
			for (AttributeModifierHandle handle : effect.appliedModifierHandles)
			{
				mAttributes->RemoveModifier(handle);
			}
		}
		effect.appliedModifierHandles.clear();
	}

	void GameplayEffectSystem::GrantTags(const ActiveGameplayEffect& effect)
	{
		if (!mOwnedTags)
		{
			return;
		}
		for (const GameplayTag& tag : effect.definition.grantedTags)
		{
			mOwnedTags->AddTag(tag);
		}
	}

	void GameplayEffectSystem::RemoveGrantedTags(const ActiveGameplayEffect& effect)
	{
		if (!mOwnedTags)
		{
			return;
		}
		for (const GameplayTag& tag : effect.definition.grantedTags)
		{
			mOwnedTags->RemoveTag(tag);
		}
	}

	void GameplayEffectSystem::RemoveEffectAt(size_t index)
	{
		if (index >= mActiveEffects.size())
		{
			return;
		}
		ActiveGameplayEffect& effect = mActiveEffects[index];
		const GameplayEffectHandle removedHandle = effect.handle;
		RemoveGrantedTags(effect);
		RemoveModifiers(effect);
		DestroyVisual(effect);
		mActiveEffects.erase(mActiveEffects.begin() + static_cast<std::ptrdiff_t>(index));
		onEffectRemoved.Broadcast(removedHandle);
		onEffectsChanged.Broadcast();
	}

	void GameplayEffectSystem::SpawnVisual(ActiveGameplayEffect& effect)
	{
		if (!mOwner || effect.definition.activeVisualId.empty())
		{
			return;
		}
		effect.visual = GameplayEffectVisualRegistry::Spawn(
			effect.definition.activeVisualId,
			*mOwner
		);
		SynchronizeVisual(effect);
	}

	void GameplayEffectSystem::SynchronizeVisual(ActiveGameplayEffect& effect)
	{
		if (auto visual = effect.visual.lock())
		{
			visual->SynchronizeState(GameplayEffectVisualStateView{
				effect.remainingDuration,
				effect.totalDuration,
				effect.stackCount,
				effect.runtimeAttributes
			});
		}
	}

	void GameplayEffectSystem::DestroyVisual(ActiveGameplayEffect& effect)
	{
		if (auto visual = effect.visual.lock())
		{
			visual->Destroy();
		}
		effect.visual.reset();
	}
}
