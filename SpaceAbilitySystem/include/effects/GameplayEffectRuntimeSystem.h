#pragma once

#include "effects/GameplayEffectBehaviorResult.h"
#include "effects/GameplayEffectBindings.h"
#include "effects/GameplayEffectCollection.h"
#include "effects/GameplayEffectLifecycleOrchestrator.h"
#include "effects/GameplayEffectRuntimeEntry.h"
#include "effects/GameplayEffectSpec.h"

#include <cstddef>
#include <cmath>
#include <functional>
#include <exception>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace sas
{
	template <typename ActiveEffect, typename IncomingSpec = GameplayEffectSpec>
	struct GameplayEffectRuntimeCallbacks
	{
		std::function<void(ActiveEffect&, const GameplayEffectSourceContext&)> bindSource;
		std::function<void(ActiveEffect&)> initialize;
		std::function<void(ActiveEffect&)> refresh;
		std::function<bool(ActiveEffect&, const IncomingSpec&)> cappedReapply;
		std::function<bool(ActiveEffect&)> addStack;
		std::function<GameplayEffectBehaviorResult(ActiveEffect&, float)> tick;
		std::function<void(const GameplayEffectBehaviorEvent&)> behaviorEvent;
		std::function<void(ActiveEffect&)> activated;
		std::function<void(ActiveEffect&)> changed;
		std::function<void(ActiveEffect&)> stackChanged;
		std::function<void(ActiveEffect&)> removing;
		std::function<void(GameplayEffectHandle)> applied;
		std::function<void(GameplayEffectHandle)> removed;
		std::function<void()> collectionChanged;
		std::function<bool()> shouldContinue;
	};

	template <typename Spec, typename ActiveEffect = GameplayEffectRuntimeEntry<Spec>>
	class GameplayEffectRuntimeSystem
	{
	public:
		using Callbacks = GameplayEffectRuntimeCallbacks<ActiveEffect, Spec>;
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
			if (mIsClearing || mClearRequested || mCleanupIncomplete)
			{
				return {};
			}

			const GameplayEffectDefinition& definition = spec.definition;
			if (!CanApplyEffect(definition) || spec.maxStacks < 1 ||
				(definition.durationPolicy == GameplayEffectDurationPolicy::Duration &&
					(!std::isfinite(spec.duration) || spec.duration <= 0.f)) ||
				(definition.stackLifetimePolicy ==
						GameplayEffectStackLifetimePolicy::DecayAfterDuration &&
					(!std::isfinite(definition.stackDecayInterval) ||
						definition.stackDecayInterval <= 0.f)))
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
				ApplyInstantGameplayEffect(spec, mAttributes, [this] { return ShouldContinueGlobal(); });
				if (!ShouldContinueGlobal()) return {};
				NotifyApplied(newHandle);
				if (!ShouldContinueGlobal()) return {};
				NotifyRemoved(newHandle);
				if (!ShouldContinueGlobal()) return {};
				NotifyCollectionChanged();
				return ShouldContinueGlobal() ? newHandle : GameplayEffectHandle{};
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
				if (stackingTarget->operationDepth != 0 ||
					stackingTarget->removeRequested ||
					stackingTarget->removalInProgress)
				{
					return {};
				}
				return RunEffectOperation(stackingHandle, [&]() -> GameplayEffectHandle
				{
					if (applicationKind == GameplayEffectApplicationKind::RefreshActive)
					{
						RemoveGameplayEffectTags(*stackingTarget, mOwnedTags);
						if (!ShouldContinueOperation(stackingHandle)) return {};
						RemoveGameplayEffectModifiers(*stackingTarget, mAttributes);
						if (!ShouldContinueOperation(stackingHandle)) return {};
						NotifyRemoving(*stackingTarget);
						if (!ShouldContinueOperation(stackingHandle)) return {};
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
						if (!ShouldContinueOperation(stackingHandle)) return {};
						if (mCallbacks.cappedReapply)
						{
						mCallbacks.cappedReapply(*stackingTarget, spec);
						if (!ShouldContinueOperation(stackingHandle)) return {};
					}
					GameplayEffectLifecycleOrchestrator::ApplyStackingState(
						applicationKind, *stackingTarget, spec.duration,
						spec.definition.stackDecayInterval, spec.maxStacks
					);
				}
				else
				{
					stackingTarget->spec = spec;
					BindSource(*stackingTarget, context);
					if (!ShouldContinueOperation(stackingHandle)) return {};
					const bool stackAdded =
						GameplayEffectLifecycleOrchestrator::ApplyStackingState(
							applicationKind, *stackingTarget, spec.duration,
							spec.definition.stackDecayInterval, spec.maxStacks
						);
					if (applicationKind == GameplayEffectApplicationKind::RefreshActive)
					{
						Refresh(*stackingTarget);
						if (!ShouldContinueOperation(stackingHandle)) return {};
						ApplyGameplayEffectModifiers(
							stackingTarget->spec, *stackingTarget, mAttributes,
							[&] { return ShouldContinueOperation(stackingHandle); }
						);
						if (!ShouldContinueOperation(stackingHandle)) return {};
						GrantGameplayEffectTags(
							stackingTarget->spec.definition, *stackingTarget, mOwnedTags
						);
						NotifyActivated(*stackingTarget);
						if (!ShouldContinueOperation(stackingHandle)) return {};
					}
					else if (stackAdded)
					{
						const bool customStackHandled = AddStack(*stackingTarget);
						if (!ShouldContinueOperation(stackingHandle)) return {};
						if (!customStackHandled)
						{
							stackingTarget->RefreshRuntimeAttributesFromSpec();
						}
						ApplyGameplayEffectModifiers(
							stackingTarget->spec, *stackingTarget, mAttributes,
							[&] { return ShouldContinueOperation(stackingHandle); }
						);
						if (!ShouldContinueOperation(stackingHandle)) return {};
						NotifyStackChanged(*stackingTarget);
						if (!ShouldContinueOperation(stackingHandle)) return {};
					}
				}
				NotifyChanged(*stackingTarget);
				if (!ShouldContinueOperation(stackingHandle)) return {};
				NotifyCollectionChanged();
				return ShouldContinueOperation(stackingHandle)
					? stackingHandle
					: GameplayEffectHandle{};
				});
			}

			mActiveEffects.Emplace(newHandle);
			return RunEffectOperation(newHandle, [&]() -> GameplayEffectHandle
			{
				ActiveEffect* current = FindEffect(newHandle);
				if (!current) return {};
				current->spec = spec;
				current->Initialize(
					newHandle, spec.duration,
					spec.definition.stackDecayInterval, spec.attributes
				);
				BindSource(*current, context);
				if (!ShouldContinueOperation(newHandle)) return {};
				Initialize(*current);
				if (!ShouldContinueOperation(newHandle)) return {};
				ApplyGameplayEffectModifiers(
					current->spec, *current, mAttributes,
					[&] { return ShouldContinueOperation(newHandle); }
				);
				if (!ShouldContinueOperation(newHandle)) return {};
				GrantGameplayEffectTags(current->spec.definition, *current, mOwnedTags);
				NotifyActivated(*current);
				if (!ShouldContinueOperation(newHandle)) return {};
				NotifyApplied(newHandle);
				if (!ShouldContinueOperation(newHandle)) return {};
				NotifyCollectionChanged();
				return ShouldContinueOperation(newHandle)
					? newHandle
					: GameplayEffectHandle{};
			});
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
			if (effect->operationDepth != 0 || effect->removeRequested ||
				effect->removalInProgress)
			{
				return false;
			}
			return RunEffectOperation(handle, [&]() -> bool
			{
				ActiveEffect* current = FindEffect(handle);
				if (!current) return false;
				current->RefreshDuration(
					current->spec.duration,
					current->spec.definition.stackDecayInterval
				);
				NotifyChanged(*current);
				if (!ShouldContinueOperation(handle)) return false;
				NotifyCollectionChanged();
				return ShouldContinueOperation(handle);
			});
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
				bool matched = false;
				RunEffectOperation(handle, [&]
				{
					const ActiveEffect* current = FindEffect(handle);
					matched = current && predicate(*current);
				});
				if (matched)
				{
					if (FindEffect(handle)) RemoveEffect(handle);
					if (!FindEffect(handle)) ++removedCount;
				}
			}
			return removedCount;
		}

		void Tick(float deltaTime)
		{
			// Effects may add or remove effects from callbacks, so ticking must use a
			// stable handle snapshot. Reuse the normal-frame snapshot capacity instead
			// of allocating a new vector every frame. A nested Tick can occur through a
			// callback; it receives its own snapshot so it cannot overwrite the outer
			// iteration buffer.
			if (mTickInProgress)
			{
				const std::vector<GameplayEffectHandle> nestedHandles = GetHandles();
				TickHandles(nestedHandles, deltaTime);
				return;
			}

			mTickHandleScratch.clear();
			mTickHandleScratch.reserve(mActiveEffects.GetAll().size());
			for (const ActiveEffect& effect : mActiveEffects.GetAll())
			{
				mTickHandleScratch.push_back(effect.handle);
			}

			mTickInProgress = true;
			try
			{
				TickHandles(mTickHandleScratch, deltaTime);
			}
			catch (...)
			{
				mTickInProgress = false;
				throw;
			}
			mTickInProgress = false;
		}

	private:
		bool ShouldContinueGlobal() const
		{
			return !mIsClearing && !mClearRequested && !mCleanupIncomplete &&
				(!mCallbacks.shouldContinue || mCallbacks.shouldContinue());
		}

		bool ShouldContinueOperation(GameplayEffectHandle handle)
		{
			ActiveEffect* effect = FindEffect(handle);
			if (!effect || effect->removeRequested || effect->removalInProgress)
			{
				return false;
			}
			if (!ShouldContinueGlobal())
			{
				effect->removeRequested = true;
				return false;
			}
			return true;
		}

		template <typename Operation>
		auto RunEffectOperation(
			GameplayEffectHandle handle,
			Operation&& operation
		) -> decltype(operation())
		{
			using Result = decltype(operation());
			ActiveEffect* effect = FindEffect(handle);
			if (!effect || effect->operationDepth != 0 || effect->removeRequested ||
				effect->removalInProgress || !ShouldContinueGlobal())
			{
				if constexpr (!std::is_void_v<Result>) return Result{};
				else return;
			}

			++effect->operationDepth;
			std::exception_ptr error;
			if constexpr (std::is_void_v<Result>)
			{
				try
				{
					std::invoke(std::forward<Operation>(operation));
					ShouldContinueOperation(handle);
				}
				catch (...) { error = std::current_exception(); }
			}
			else
			{
				std::optional<Result> result;
				try
				{
					result.emplace(std::invoke(std::forward<Operation>(operation)));
					ShouldContinueOperation(handle);
				}
				catch (...) { error = std::current_exception(); }

				effect = FindEffect(handle);
				if (effect)
				{
					if (error) effect->removeRequested = true;
					if (effect->operationDepth != 0) --effect->operationDepth;
				}
				if (effect && effect->operationDepth == 0 && effect->removeRequested)
				{
					try { RemoveEffectInternal(handle); }
					catch (...) { if (!error) error = std::current_exception(); }
				}
				if (mClearRequested && !mIsClearing)
				{
					DrainRequestedClear(error);
				}
				if (error) std::rethrow_exception(error);
				effect = FindEffect(handle);
				return effect && !effect->removeRequested
					? std::move(*result)
					: Result{};
			}

			if (!error) ShouldContinueOperation(handle);
			effect = FindEffect(handle);
			if (effect)
			{
				if (error) effect->removeRequested = true;
				if (effect->operationDepth != 0) --effect->operationDepth;
			}
			if (effect && effect->operationDepth == 0 && effect->removeRequested)
			{
				try { RemoveEffectInternal(handle); }
				catch (...) { if (!error) error = std::current_exception(); }
			}
			if (mClearRequested && !mIsClearing)
			{
				DrainRequestedClear(error);
			}
			if (error) std::rethrow_exception(error);
		}

		void DrainRequestedClear(std::exception_ptr& error)
		{
			if (!mClearRequested || mIsClearing) return;
			mIsClearing = true;
			for (ActiveEffect& effect : mActiveEffects.GetAll())
			{
				effect.removeRequested = true;
			}

			std::vector<GameplayEffectHandle> handles;
			try { handles = GetHandles(); }
			catch (...)
			{
				if (!error) error = std::current_exception();
				mIsClearing = false;
				mCleanupIncomplete = !mActiveEffects.GetAll().empty();
				return;
			}
			for (const GameplayEffectHandle handle : handles)
			{
				try { RemoveEffectInternal(handle); }
				catch (...) { if (!error) error = std::current_exception(); }
			}
			mIsClearing = false;

			if (mActiveEffects.GetAll().empty())
			{
				mClearRequested = false;
				mCleanupIncomplete = false;
				return;
			}

			mCleanupIncomplete = false;
			for (const ActiveEffect& effect : mActiveEffects.GetAll())
			{
				if (effect.operationDepth == 0 && !effect.removalInProgress)
				{
					mCleanupIncomplete = true;
					break;
				}
			}
		}

		bool IsDecayAfterDuration(const ActiveEffect& effect) const
		{
			return effect.spec.definition.durationPolicy == GameplayEffectDurationPolicy::Duration &&
				effect.spec.definition.stackLifetimePolicy == GameplayEffectStackLifetimePolicy::DecayAfterDuration;
		}

		void TickDecayEffect(GameplayEffectHandle effectHandle, float deltaTime)
		{
			float remainingDeltaTime = deltaTime;
			auto notifyDurationChange = [&](bool expired, bool stackChanged) -> bool
			{
				if (!stackChanged)
				{
					if (expired)
					{
						RemoveEffect(effectHandle);
						return false;
					}
					return true;
				}

				ActiveEffect* current = FindEffect(effectHandle);
				if (!current || !ShouldContinueOperation(effectHandle))
				{
					return false;
				}
				NotifyStackChanged(*current);
				if (!ShouldContinueOperation(effectHandle))
				{
					return false;
				}
				current = FindEffect(effectHandle);
				if (!current) return false;
				NotifyChanged(*current);
				if (!ShouldContinueOperation(effectHandle))
				{
					return false;
				}
				if (expired)
				{
					RemoveEffect(effectHandle);
					return false;
				}
				NotifyCollectionChanged();
				return true;
			};

			while (remainingDeltaTime > 0.f)
			{
				ActiveEffect* effect = FindEffect(effectHandle);
				if (!effect || !IsDecayAfterDuration(*effect))
				{
					return;
				}

				const float slice = GameplayEffectLifecycleOrchestrator::GetTickSliceDuration(
					effect->spec.definition.durationPolicy,
					effect->spec.definition.stackLifetimePolicy,
					*effect,
					remainingDeltaTime);
				if (slice <= 0.f)
				{
					const int stackCountBeforeDurationTick = effect->stackCount;
					const GameplayEffectDurationTickResult durationTick =
						GameplayEffectLifecycleOrchestrator::TickDuration(
							effect->spec.definition.durationPolicy,
							effect->spec.definition.stackLifetimePolicy,
							*effect,
							0.f);
					const bool stackChanged = effect->stackCount != stackCountBeforeDurationTick;
					if (!stackChanged && !durationTick.expired)
					{
						return;
					}
					if (!notifyDurationChange(durationTick.expired, stackChanged))
					{
						return;
					}
					continue;
				}

				const GameplayEffectBehaviorResult behaviorTick =
					mCallbacks.tick
						? mCallbacks.tick(*effect, slice)
						: GameplayEffectBehaviorResult{};
				if (!ShouldContinueOperation(effectHandle)) return;
				for (const GameplayEffectBehaviorEvent& event : behaviorTick.events)
				{
					if (mCallbacks.behaviorEvent)
					{
						mCallbacks.behaviorEvent(event);
					}
					if (!ShouldContinueOperation(effectHandle))
					{
						return;
					}
				}
				if (behaviorTick.changed)
				{
					if (ActiveEffect* current = FindEffect(effectHandle))
					{
						NotifyChanged(*current);
					}
					if (!ShouldContinueOperation(effectHandle))
					{
						return;
					}
					NotifyCollectionChanged();
					if (!ShouldContinueOperation(effectHandle)) return;
				}
				if (behaviorTick.removeEffect)
				{
					RemoveEffect(effectHandle);
					return;
				}

				effect = FindEffect(effectHandle);
				if (!effect || !IsDecayAfterDuration(*effect))
				{
					return;
				}
				const int stackCountBeforeDurationTick = effect->stackCount;
				const GameplayEffectDurationTickResult durationTick =
					GameplayEffectLifecycleOrchestrator::TickDuration(
						effect->spec.definition.durationPolicy,
						effect->spec.definition.stackLifetimePolicy,
						*effect,
						slice);
				const bool stackChanged = effect->stackCount != stackCountBeforeDurationTick;
				if (!notifyDurationChange(durationTick.expired, stackChanged))
				{
					return;
				}
				remainingDeltaTime -= slice;
			}
		}

		void TickHandles(
			const std::vector<GameplayEffectHandle>& handles,
			float deltaTime
		)
		{
			for (const GameplayEffectHandle effectHandle : handles)
			{
				if (!FindEffect(effectHandle)) continue;
				RunEffectOperation(effectHandle, [&] { TickHandle(effectHandle, deltaTime); });
			}
		}

		void TickHandle(GameplayEffectHandle effectHandle, float deltaTime)
		{
			ActiveEffect* effect = FindEffect(effectHandle);
			if (!effect) return;
			if (IsDecayAfterDuration(*effect))
			{
				TickDecayEffect(effectHandle, deltaTime);
				return;
			}

			const GameplayEffectBehaviorResult behaviorTick = mCallbacks.tick
				? mCallbacks.tick(*effect, deltaTime)
				: GameplayEffectBehaviorResult{};
			if (!ShouldContinueOperation(effectHandle)) return;
			for (const GameplayEffectBehaviorEvent& event : behaviorTick.events)
			{
				if (mCallbacks.behaviorEvent) mCallbacks.behaviorEvent(event);
				if (!ShouldContinueOperation(effectHandle)) return;
			}
			if (behaviorTick.changed)
			{
				if (ActiveEffect* current = FindEffect(effectHandle)) NotifyChanged(*current);
				if (!ShouldContinueOperation(effectHandle)) return;
				NotifyCollectionChanged();
				if (!ShouldContinueOperation(effectHandle)) return;
			}
			if (behaviorTick.removeEffect)
			{
				RemoveEffect(effectHandle);
				return;
			}

			effect = FindEffect(effectHandle);
			if (!effect) return;
			const int stackCountBeforeDurationTick = effect->stackCount;
			const GameplayEffectDurationTickResult durationTick =
				GameplayEffectLifecycleOrchestrator::TickDuration(
					effect->spec.definition.durationPolicy,
					effect->spec.definition.stackLifetimePolicy,
					*effect,
					deltaTime
				);
			const bool stackChanged = effect->stackCount != stackCountBeforeDurationTick;
			if (durationTick.expired)
			{
				if (stackChanged)
				{
					if (ActiveEffect* current = FindEffect(effectHandle))
					{
						NotifyStackChanged(*current);
						if (!ShouldContinueOperation(effectHandle)) return;
						current = FindEffect(effectHandle);
						if (!current) return;
						NotifyChanged(*current);
						if (!ShouldContinueOperation(effectHandle)) return;
					}
				}
				RemoveEffect(effectHandle);
				return;
			}
			if (durationTick.changed)
			{
				if (ActiveEffect* current = FindEffect(effectHandle))
				{
					if (stackChanged)
					{
						NotifyStackChanged(*current);
						if (!ShouldContinueOperation(effectHandle)) return;
						current = FindEffect(effectHandle);
						if (!current) return;
					}
					NotifyChanged(*current);
					if (!ShouldContinueOperation(effectHandle)) return;
				}
				NotifyCollectionChanged();
				ShouldContinueOperation(effectHandle);
			}
		}

	public:
		void Clear()
		{
			if (mIsClearing)
			{
				return;
			}
			mClearRequested = true;
			std::exception_ptr error;
			DrainRequestedClear(error);
			if (!error && mCleanupIncomplete)
			{
				error = std::make_exception_ptr(
					std::runtime_error("Gameplay effect cleanup remains incomplete.")
				);
			}
			if (error) std::rethrow_exception(error);
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
			ActiveEffect* effect = mActiveEffects.Find(handle);
			if (!effect)
			{
				return false;
			}
			if (effect->operationDepth != 0 || effect->removalInProgress)
			{
				effect->removeRequested = true;
				return true;
			}

			effect->removeRequested = true;
			effect->removalInProgress = true;
			std::exception_ptr error;
			const auto cleanup = [&error](auto&& operation)
			{
				try { operation(); }
				catch (...) { if (!error) error = std::current_exception(); }
			};
			cleanup([&] { RemoveGameplayEffectTags(*effect, mOwnedTags); });
			cleanup([&] { RemoveGameplayEffectModifiers(*effect, mAttributes); });
			bool removingCallbackCompleted = true;
			try { NotifyRemoving(*effect); }
			catch (...)
			{
				removingCallbackCompleted = false;
				if (!error) error = std::current_exception();
			}

			effect = mActiveEffects.Find(handle);
			if (!effect)
			{
				if (error) std::rethrow_exception(error);
				return false;
			}
			if (!removingCallbackCompleted ||
				!effect->appliedModifierHandles.empty() ||
				!effect->appliedGrantedTags.empty())
			{
				effect->removalInProgress = false;
				if (error) std::rethrow_exception(error);
				return false;
			}

			effect->removalInProgress = false;
			const bool erased = mActiveEffects.Erase(handle);
			if (!erased)
			{
				if (error) std::rethrow_exception(error);
				return false;
			}
			cleanup([&] { NotifyRemoved(handle); });
			cleanup([&] { NotifyCollectionChanged(); });
			if (mClearRequested && !mIsClearing)
			{
				DrainRequestedClear(error);
			}
			if (error) std::rethrow_exception(error);
			return erased;
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
			if (!ShouldContinueGlobal() ||
				!CanApplyGameplayEffect(definition, mOwnedTags))
			{
				return false;
			}

			if (!definition.immunityCategory.empty())
			{
				for (const ActiveEffect& activeEffect : mActiveEffects.GetAll())
				{
					const std::string& grantedCategory =
						activeEffect.spec.definition.grantedImmunityCategory;
					// Immunity categories are a dot-separated hierarchy. A provider of
					// "Control" therefore blocks every concrete control subtype (for
					// example "Control.Stun"), while a specific provider such as
					// "Movement.Slow" remains narrow.
					if (grantedCategory == definition.immunityCategory ||
						(!grantedCategory.empty() &&
							definition.immunityCategory.rfind(grantedCategory + ".", 0) == 0))
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
					if (!FindEffect(effectHandle)) continue;
					RunEffectOperation(effectHandle, [&]
					{
						ActiveEffect* effect = FindEffect(effectHandle);
						if (!effect || !ShouldContinueOperation(effectHandle) ||
							std::invoke(resolvePhase, *effect) != phase)
						{
							return;
						}
						const GameplayEffectBehaviorResult result =
							std::invoke(process, *effect, context);
						if (!ShouldContinueOperation(effectHandle)) return;
						for (const GameplayEffectBehaviorEvent& event : result.events)
						{
							if (mCallbacks.behaviorEvent) mCallbacks.behaviorEvent(event);
							if (!ShouldContinueOperation(effectHandle)) return;
						}
						if (result.changed)
						{
							if (ActiveEffect* current = FindEffect(effectHandle))
							{
								NotifyChanged(*current);
							}
							if (!ShouldContinueOperation(effectHandle)) return;
							NotifyCollectionChanged();
							if (!ShouldContinueOperation(effectHandle)) return;
						}
						if (result.removeEffect) RemoveEffect(effectHandle);
					});
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

		void NotifyStackChanged(ActiveEffect& effect)
		{
			if (mCallbacks.stackChanged) mCallbacks.stackChanged(effect);
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
		bool mIsClearing = false;
		bool mClearRequested = false;
		bool mCleanupIncomplete = false;
		std::vector<GameplayEffectHandle> mTickHandleScratch;
		bool mTickInProgress = false;
	};
}
