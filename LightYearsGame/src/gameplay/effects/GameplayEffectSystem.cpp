#include "gameplay/effects/GameplayEffectSystem.h"
#include "gameplay/effects/GameplayEffectBehavior.h"
#include "gameplay/attributes/AttributeSystem.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/combat/Combatant.h"
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
		return ApplyEffect(MakeGameplayEffectSpec(definition), GameplayEffectApplicationContext{ source });
	}

	GameplayEffectHandle GameplayEffectSystem::ApplyEffect(
		const GameplayEffectDefinition& definition,
		const GameplayEffectApplicationContext& context
	)
	{
		return ApplyEffect(MakeGameplayEffectSpec(definition), context);
	}

	GameplayEffectHandle GameplayEffectSystem::ApplyEffect(
		const GameplayEffectSpec& spec,
		Actor* source
	)
	{
		return ApplyEffect(spec, GameplayEffectApplicationContext{ source });
	}

	GameplayEffectHandle GameplayEffectSystem::ApplyEffect(
		const GameplayEffectSpec& spec,
		const GameplayEffectApplicationContext& context
	)
	{
		LY_PROFILE_FUNCTION();
		const GameplayEffectDefinition& definition = spec.definition;
		if (!CanApplyEffect(definition))
		{
			return {};
		}

		const GameplayEffectHandle newHandle{ mNextHandleId++ };
		if (definition.durationPolicy == GameplayEffectDurationPolicy::Instant)
		{
			if (mAttributes)
			{
				for (const AttributeModifier& modifier : spec.modifiers)
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
				if (activeEffect.spec.definition.effectId != definition.effectId ||
					(definition.sourceScopedApplication &&
						activeEffect.sourceScope != context.sourceScope))
				{
					continue;
				}

				if (definition.stackingPolicy == GameplayEffectStackingPolicy::RefreshDuration)
				{
					RemoveGrantedTags(activeEffect);
					RemoveModifiers(activeEffect);
					DestroyVisual(activeEffect);
					activeEffect.spec = spec;
					activeEffect.source = context.source;
					activeEffect.sourceScope = context.sourceScope;
					activeEffect.runtimeContext = context.runtimeContext;
					activeEffect.stackCount = 1;
					GameplayEffectBehavior::Refresh(activeEffect);
					ApplyModifiers(activeEffect);
					GrantTags(activeEffect);
					SpawnVisual(activeEffect);
				}
				else if (definition.stackingPolicy == GameplayEffectStackingPolicy::Stack)
				{
					activeEffect.spec = spec;
					activeEffect.source = context.source;
					activeEffect.sourceScope = context.sourceScope;
					activeEffect.runtimeContext = context.runtimeContext;
					if (activeEffect.stackCount < std::max(1, spec.maxStacks))
					{
						++activeEffect.stackCount;
						ApplyModifiers(activeEffect);
						GameplayEffectBehavior::AddStack(activeEffect);
					}
				}

				activeEffect.remainingDuration = spec.duration;
				activeEffect.totalDuration = spec.duration;
				SynchronizeVisual(activeEffect);
				onEffectChanged.Broadcast(activeEffect.handle);
				onEffectsChanged.Broadcast();
				return activeEffect.handle;
			}
		}

		mActiveEffects.emplace_back();
		ActiveGameplayEffect& effect = mActiveEffects.back();
		effect.handle = newHandle;
		effect.spec = spec;
		effect.remainingDuration = spec.duration;
		effect.totalDuration = spec.duration;
		effect.source = context.source;
		effect.sourceScope = context.sourceScope;
		effect.runtimeContext = context.runtimeContext;
		GameplayEffectBehavior::Initialize(effect);
		ApplyModifiers(effect);
		GrantTags(effect);
		SpawnVisual(effect);
		onEffectApplied.Broadcast(newHandle);
		onEffectsChanged.Broadcast();
		return newHandle;
	}

	bool GameplayEffectSystem::RefreshEffectDuration(GameplayEffectHandle handle)
	{
		for (ActiveGameplayEffect& effect : mActiveEffects)
		{
			if (!(effect.handle == handle) ||
				effect.spec.definition.durationPolicy != GameplayEffectDurationPolicy::Duration)
			{
				continue;
			}
			effect.remainingDuration = effect.spec.duration;
			effect.totalDuration = effect.spec.duration;
			SynchronizeVisual(effect);
			onEffectChanged.Broadcast(effect.handle);
			onEffectsChanged.Broadcast();
			return true;
		}
		return false;
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
		LY_PROFILE_FUNCTION();
		for (size_t i = 0; i < mActiveEffects.size();)
		{
			ActiveGameplayEffect& effect = mActiveEffects[i];
			const GameplayEffectBehaviorResult behaviorTick = mOwner
				? GameplayEffectBehavior::Tick(effect, *mOwner, deltaTime)
				: GameplayEffectBehaviorResult{};
			if (behaviorTick.changed)
			{
				SynchronizeVisual(effect);
				onEffectChanged.Broadcast(effect.handle);
				onEffectsChanged.Broadcast();
			}
			if (behaviorTick.removeEffect)
			{
				RemoveEffectAt(i);
				continue;
			}
			if (effect.spec.definition.durationPolicy == GameplayEffectDurationPolicy::Duration)
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
		LY_PROFILE_FUNCTION();
		const auto processPhase = [&](IncomingDamagePhase phase)
		{
			for (size_t i = 0; i < mActiveEffects.size() && context.remainingDamage > 0.f;)
			{
				ActiveGameplayEffect& effect = mActiveEffects[i];
				if (GameplayEffectBehavior::GetIncomingDamagePhase(effect) != phase)
				{
					++i;
					continue;
				}

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
		};

		processPhase(IncomingDamagePhase::PreMitigation);
		processPhase(IncomingDamagePhase::Standard);
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

	const ActiveGameplayEffect* GameplayEffectSystem::FindEffectById(
		const std::string& effectId
	) const
	{
		for (const ActiveGameplayEffect& effect : mActiveEffects)
		{
			if (effect.spec.definition.effectId == effectId)
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
			snapshot.effectId = effect.spec.definition.effectId;
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
		for (const AttributeModifier& modifier : effect.spec.modifiers)
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
		for (const GameplayTag& tag : effect.spec.definition.grantedTags)
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
		for (const GameplayTag& tag : effect.spec.definition.grantedTags)
		{
			mOwnedTags->RemoveTag(tag);
		}
	}

	void GameplayEffectSystem::RemoveEffectAt(size_t index)
	{
		LY_ASSERT(
			index < mActiveEffects.size(),
			"GameplayEffectSystem removal index out of range: %zu/%zu",
			index,
			mActiveEffects.size()
		);
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
		if (!mOwner || effect.spec.definition.activeVisualId.empty())
		{
			return;
		}
		effect.visual = GameplayEffectVisualRegistry::Spawn(
			effect.spec.definition.activeVisualId,
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
