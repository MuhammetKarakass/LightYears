#pragma once

#include "effects/GameplayEffectBehaviorResult.h"
#include "effects/GameplayEffectBindings.h"
#include "effects/GameplayEffectCollection.h"
#include "effects/GameplayEffectLifecycleOrchestrator.h"
#include "effects/GameplayEffectRuntimeEntry.h"

#include <cstddef>
#include <cmath>
#include <functional>
#include <utility>
#include <vector>

namespace sas
{
	template <typename ActiveEffect>
	struct GameplayEffectRuntimeCallbacks
	{
		std::function<void(ActiveEffect&, const GameplayEffectSourceContext&)> bindSource;
		std::function<void(ActiveEffect&)> initialize;
		std::function<void(ActiveEffect&)> refresh;
		std::function<bool(ActiveEffect&)> addStack;
		std::function<GameplayEffectBehaviorResult(ActiveEffect&, float)> tick;
		std::function<void(const GameplayEffectBehaviorEvent&)> behaviorEvent;
		std::function<void(ActiveEffect&)> activated;
		std::function<void(ActiveEffect&)> changed;
		std::function<void(ActiveEffect&)> removing;
		std::function<void(GameplayEffectHandle)> applied;
		std::function<void(GameplayEffectHandle)> removed;
		std::function<void()> collectionChanged;
	};

	template <typename Spec, typename ActiveEffect = GameplayEffectRuntimeEntry<Spec>>
	class GameplayEffectRuntimeSystem
	{
	public:
		using Callbacks = GameplayEffectRuntimeCallbacks<ActiveEffect>;
		using Collection = GameplayEffectCollection<ActiveEffect>;

		GameplayEffectRuntimeSystem(
			AttributeSystem& attributes,
			ly::GameplayTagContainer& ownedTags,
			Callbacks callbacks = {}
		)
			: mAttributes{ attributes },
			mOwnedTags{ ownedTags },
			mCallbacks{ std::move(callbacks) }
		{
		}

		void SetCallbacks(Callbacks callbacks)
		{
			mCallbacks = std::move(callbacks);
		}

		GameplayEffectHandle ApplyEffect(
			const GameplayEffectDefinition& definition,
			const GameplayEffectSourceContext& context = {}
		)
		{
			Spec spec;
			static_cast<GameplayEffectSpec&>(spec) = MakeGameplayEffectSpec(definition);
			return ApplyEffect(spec, context);
		}

		GameplayEffectHandle ApplyEffect(
			const Spec& spec,
			const GameplayEffectSourceContext& context = {}
		)
		{
			const GameplayEffectDefinition& definition = spec.definition;
			if (!CanApplyEffect(definition) || spec.maxStacks < 1 ||
				(definition.durationPolicy == GameplayEffectDurationPolicy::Duration &&
					(!std::isfinite(spec.duration) || spec.duration <= 0.f)))
			{
				return {};
			}

			const GameplayEffectHandle newHandle = mActiveEffects.AllocateHandle();
			GameplayEffectApplicationKind applicationKind =
				GameplayEffectLifecycleOrchestrator::ResolveApplication(
					definition, false, 0, spec.maxStacks
				);
			if (applicationKind == GameplayEffectApplicationKind::Instant)
			{
				ApplyInstantGameplayEffect(spec, mAttributes);
				NotifyApplied(newHandle);
				NotifyRemoved(newHandle);
				NotifyCollectionChanged();
				return newHandle;
			}

			ActiveEffect* stackingTarget = mActiveEffects.FindFirst(
				[&](const ActiveEffect& activeEffect)
				{
					return GameplayEffectLifecycleOrchestrator::MatchesStackingTarget(
						definition,
						activeEffect.spec.definition.effectId,
						activeEffect.sourceScope,
						context.sourceScope
					);
				}
			);
			applicationKind = GameplayEffectLifecycleOrchestrator::ResolveApplication(
				definition,
				stackingTarget != nullptr,
				stackingTarget ? stackingTarget->stackCount : 0,
				spec.maxStacks
			);
			if (stackingTarget)
			{
				const GameplayEffectHandle stackingHandle = stackingTarget->handle;
				if (applicationKind == GameplayEffectApplicationKind::RefreshActive)
				{
					RemoveGameplayEffectTags(stackingTarget->spec.definition, mOwnedTags);
					RemoveGameplayEffectModifiers(*stackingTarget, mAttributes);
					NotifyRemoving(*stackingTarget);
					if (!FindEffect(stackingHandle))
					{
						return {};
					}
				}

				if (applicationKind == GameplayEffectApplicationKind::RefreshStackDuration)
				{
					// This policy is duration-only. Keep the active runtime
					// attributes and modifiers intact; only the duration-facing
					// spec fields and source context are refreshed.
					stackingTarget->spec.duration = spec.duration;
					stackingTarget->spec.maxStacks = spec.maxStacks;
					stackingTarget->spec.sourceAbilityUpgradeIds =
						spec.sourceAbilityUpgradeIds;
					BindSource(*stackingTarget, context);
					if (!FindEffect(stackingHandle))
					{
						return {};
					}
					GameplayEffectLifecycleOrchestrator::ApplyStackingState(
						applicationKind,
						*stackingTarget,
						spec.duration,
						spec.maxStacks
					);
				}
				else
				{
					stackingTarget->spec = spec;
					BindSource(*stackingTarget, context);
					if (!FindEffect(stackingHandle))
					{
						return {};
					}
					const bool stackAdded =
						GameplayEffectLifecycleOrchestrator::ApplyStackingState(
							applicationKind,
							*stackingTarget,
							spec.duration,
							spec.maxStacks
						);
					if (applicationKind == GameplayEffectApplicationKind::RefreshActive)
					{
						Refresh(*stackingTarget);
						if (!FindEffect(stackingHandle))
						{
							return {};
						}
						ApplyGameplayEffectModifiers(
							stackingTarget->spec, *stackingTarget, mAttributes
						);
						GrantGameplayEffectTags(stackingTarget->spec.definition, mOwnedTags);
						NotifyActivated(*stackingTarget);
					}
					else if (stackAdded)
					{
						const bool customStackHandled = AddStack(*stackingTarget);
						if (!customStackHandled)
						{
							stackingTarget->RefreshRuntimeAttributesFromSpec();
						}
						ApplyGameplayEffectModifiers(
							stackingTarget->spec, *stackingTarget, mAttributes
						);
					}
				}
				if (!FindEffect(stackingHandle))
				{
					return {};
				}
				NotifyChanged(*stackingTarget);
				if (!FindEffect(stackingHandle))
				{
					return {};
				}
				NotifyCollectionChanged();
				return stackingHandle;
			}

			ActiveEffect& effect = mActiveEffects.Emplace();
			effect.spec = spec;
			effect.Initialize(newHandle, spec.duration, spec.attributes);
			BindSource(effect, context);
			if (!FindEffect(newHandle))
			{
				return {};
			}
			Initialize(effect);
			if (!FindEffect(newHandle))
			{
				return {};
			}
			ApplyGameplayEffectModifiers(effect.spec, effect, mAttributes);
			GrantGameplayEffectTags(effect.spec.definition, mOwnedTags);
			NotifyActivated(effect);
			NotifyApplied(newHandle);
			NotifyCollectionChanged();
			return newHandle;
		}

		bool RefreshEffectDuration(GameplayEffectHandle handle)
		{
			ActiveEffect* effect = mActiveEffects.Find(handle);
			if (!effect ||
				!GameplayEffectLifecycleOrchestrator::CanRefreshDuration(
					effect->spec.definition.durationPolicy
				))
			{
				return false;
			}
			effect->RefreshDuration(effect->spec.duration);
			NotifyChanged(*effect);
			NotifyCollectionChanged();
			return true;
		}

		bool RemoveEffect(GameplayEffectHandle handle)
		{
			return RemoveEffectInternal(handle);
		}

		std::size_t RemoveEffectsIf(
			const std::function<bool(const ActiveEffect&)>& predicate
		)
		{
			if (!predicate)
			{
				return 0;
			}

			std::size_t removedCount = 0;
			for (const GameplayEffectHandle handle : GetHandles())
			{
				const ActiveEffect* effect = FindEffect(handle);
				if (!effect || !predicate(*effect))
				{
					continue;
				}
				removedCount += RemoveEffect(handle) ? 1u : 0u;
			}
			return removedCount;
		}

		void Tick(float deltaTime)
		{
			for (const GameplayEffectHandle effectHandle : GetHandles())
			{
				ActiveEffect* effect = FindEffect(effectHandle);
				if (!effect)
				{
					continue;
				}
				const GameplayEffectBehaviorResult behaviorTick =
					mCallbacks.tick
						? mCallbacks.tick(*effect, deltaTime)
						: GameplayEffectBehaviorResult{};
				for (const GameplayEffectBehaviorEvent& event : behaviorTick.events)
				{
					if (mCallbacks.behaviorEvent)
					{
						mCallbacks.behaviorEvent(event);
					}
				}
				if (behaviorTick.changed)
				{
					if (ActiveEffect* current = FindEffect(effectHandle))
					{
						NotifyChanged(*current);
					}
					if (!FindEffect(effectHandle))
					{
						continue;
					}
					NotifyCollectionChanged();
				}
				if (behaviorTick.removeEffect)
				{
					RemoveEffect(effectHandle);
					continue;
				}
				effect = FindEffect(effectHandle);
				if (!effect)
				{
					continue;
				}
				const GameplayEffectDurationTickResult durationTick =
					GameplayEffectLifecycleOrchestrator::TickDuration(
						effect->spec.definition.durationPolicy, *effect, deltaTime
					);
				if (durationTick.expired)
				{
					RemoveEffect(effectHandle);
					continue;
				}
				if (durationTick.changed)
				{
					if (ActiveEffect* current = FindEffect(effectHandle))
					{
						NotifyChanged(*current);
					}
					NotifyCollectionChanged();
				}
			}
		}

		void Clear()
		{
			for (const GameplayEffectHandle handle : GetHandles())
			{
				RemoveEffect(handle);
			}
			while (!mActiveEffects.GetAll().empty())
			{
				const std::vector<GameplayEffectHandle> handles = GetHandles();
				bool removedAny = false;
				for (const GameplayEffectHandle handle : handles)
				{
					removedAny = RemoveEffect(handle) || removedAny;
				}
				if (!removedAny)
				{
					break;
				}
			}
			mActiveEffects.Reset();
		}

		bool RemoveEffectAt(std::size_t index)
		{
			if (index >= mActiveEffects.GetAll().size())
			{
				return false;
			}
			ActiveEffect* effect = mActiveEffects.At(index);
			if (!effect)
			{
				return false;
			}
			return RemoveEffectInternal(effect->handle);
		}

		bool RemoveEffectInternal(GameplayEffectHandle handle)
		{
			if (std::find(
					mRemovalInProgress.begin(),
					mRemovalInProgress.end(),
					handle
				) != mRemovalInProgress.end())
			{
				return false;
			}
			ActiveEffect* effect = mActiveEffects.Find(handle);
			if (!effect)
			{
				return false;
			}
			mRemovalInProgress.push_back(handle);
			RemoveGameplayEffectTags(effect->spec.definition, mOwnedTags);
			RemoveGameplayEffectModifiers(*effect, mAttributes);
			NotifyRemoving(*effect);
			const auto currentIndex = mActiveEffects.FindIndex(handle);
			if (currentIndex)
			{
				mActiveEffects.EraseAt(*currentIndex);
			}
			mRemovalInProgress.pop_back();
			NotifyRemoved(handle);
			NotifyCollectionChanged();
			return true;
		}

		ActiveEffect* FindEffect(GameplayEffectHandle handle)
		{
			return mActiveEffects.Find(handle);
		}

		const ActiveEffect* FindEffect(GameplayEffectHandle handle) const
		{
			return mActiveEffects.Find(handle);
		}

		std::vector<GameplayEffectHandle> GetHandles() const
		{
			std::vector<GameplayEffectHandle> handles;
			handles.reserve(mActiveEffects.GetAll().size());
			for (const ActiveEffect& effect : mActiveEffects.GetAll())
			{
				handles.push_back(effect.handle);
			}
			return handles;
		}

		ActiveEffect* FindEffectById(const std::string& effectId)
		{
			return mActiveEffects.FindFirst(
				[&](const ActiveEffect& effect)
				{
					return effect.spec.definition.effectId == effectId;
				}
			);
		}

		const ActiveEffect* FindEffectById(const std::string& effectId) const
		{
			return mActiveEffects.FindFirst(
				[&](const ActiveEffect& effect)
				{
					return effect.spec.definition.effectId == effectId;
				}
			);
		}

		std::vector<GameplayEffectRuntimeSnapshot> BuildSnapshots() const
		{
			std::vector<GameplayEffectRuntimeSnapshot> snapshots;
			snapshots.reserve(mActiveEffects.GetAll().size());
			for (const ActiveEffect& effect : mActiveEffects.GetAll())
			{
				snapshots.push_back(effect.BuildRuntimeSnapshot());
			}
			return snapshots;
		}

		bool CanApplyEffect(const GameplayEffectDefinition& definition) const
		{
			if (!CanApplyGameplayEffect(definition, mOwnedTags))
			{
				return false;
			}

			if (!definition.immunityCategory.empty())
			{
				for (const ActiveEffect& activeEffect : mActiveEffects.GetAll())
				{
					if (activeEffect.spec.definition.grantedImmunityCategory ==
						definition.immunityCategory)
					{
						return false;
					}
				}
			}

			return true;
		}

		template <
			typename EventContext,
			typename EventPhase,
			typename PhaseResolver,
			typename EventProcessor,
			typename ContinuePredicate
		>
		void ProcessEvent(
			EventContext& context,
			const std::vector<EventPhase>& phases,
			PhaseResolver&& resolvePhase,
			EventProcessor&& process,
			ContinuePredicate&& shouldContinue
		)
		{
			const std::vector<GameplayEffectHandle> handles = GetHandles();
			for (const EventPhase& phase : phases)
			{
				for (const GameplayEffectHandle effectHandle : handles)
				{
					if (!std::invoke(shouldContinue, context))
					{
						break;
					}
					ActiveEffect* effect = FindEffect(effectHandle);
					if (!effect)
					{
						continue;
					}
					if (std::invoke(resolvePhase, *effect) != phase)
					{
						continue;
					}

					const GameplayEffectBehaviorResult result =
						std::invoke(process, *effect, context);
					for (const GameplayEffectBehaviorEvent& event : result.events)
					{
						if (mCallbacks.behaviorEvent)
						{
							mCallbacks.behaviorEvent(event);
						}
					}
					if (!FindEffect(effectHandle))
					{
						continue;
					}
					if (result.changed)
					{
						if (ActiveEffect* current = FindEffect(effectHandle))
						{
							NotifyChanged(*current);
						}
						if (!FindEffect(effectHandle))
						{
							continue;
						}
						NotifyCollectionChanged();
					}
					if (result.removeEffect)
					{
						RemoveEffect(effectHandle);
					}
				}
			}
		}

	private:
		void BindSource(
			ActiveEffect& effect,
			const GameplayEffectSourceContext& context
		)
		{
			effect.sourceObject = context.sourceObject;
			effect.sourceScope = context.sourceScope;
			effect.runtimeContext = context.runtimeContext;
			if (mCallbacks.bindSource)
			{
				mCallbacks.bindSource(effect, context);
			}
		}

		void Initialize(ActiveEffect& effect)
		{
			if (mCallbacks.initialize)
			{
				mCallbacks.initialize(effect);
			}
			else
			{
				effect.ResetRuntimeAttributesFromSpec();
			}
		}

		void Refresh(ActiveEffect& effect)
		{
			if (mCallbacks.refresh)
			{
				mCallbacks.refresh(effect);
			}
			else
			{
				effect.ResetRuntimeAttributesFromSpec();
			}
		}

		bool AddStack(ActiveEffect& effect)
		{
			if (mCallbacks.addStack)
			{
				return mCallbacks.addStack(effect);
			}
			return false;
		}

		void NotifyActivated(ActiveEffect& effect)
		{
			if (mCallbacks.activated) mCallbacks.activated(effect);
		}

		void NotifyChanged(ActiveEffect& effect)
		{
			if (mCallbacks.changed) mCallbacks.changed(effect);
		}

		void NotifyRemoving(ActiveEffect& effect)
		{
			if (mCallbacks.removing) mCallbacks.removing(effect);
		}

		void NotifyApplied(GameplayEffectHandle handle)
		{
			if (mCallbacks.applied) mCallbacks.applied(handle);
		}

		void NotifyRemoved(GameplayEffectHandle handle)
		{
			if (mCallbacks.removed) mCallbacks.removed(handle);
		}

		void NotifyCollectionChanged()
		{
			if (mCallbacks.collectionChanged) mCallbacks.collectionChanged();
		}

		AttributeSystem& mAttributes;
		ly::GameplayTagContainer& mOwnedTags;
		Callbacks mCallbacks;
		Collection mActiveEffects;
		std::vector<GameplayEffectHandle> mRemovalInProgress;
	};
}
