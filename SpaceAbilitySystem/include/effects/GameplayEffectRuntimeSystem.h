#pragma once

#include "ClearResult.h"
#include "effects/GameplayEffectBehaviorResult.h"
#include "effects/GameplayEffectBindings.h"
#include "effects/GameplayEffectCollection.h"
#include "effects/GameplayEffectLifecycleOrchestrator.h"
#include "effects/GameplayEffectRuntimeEntry.h"
#include "effects/GameplayEffectSpec.h"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <functional>
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

	// Every mutation of an active effect runs inside an OperationScope. While any scope is
	// open the collection is structurally frozen: RemoveEffect and Clear only queue their
	// work, and the queue is drained when the outermost scope closes. Callbacks therefore
	// never see an effect disappear mid-operation. A callback that requests removal (or
	// clears the system) marks the effect cancelled, and OperationScope::Step/Run stop the
	// remaining steps at the next callback boundary.
	template <typename Spec, typename ActiveEffect = GameplayEffectRuntimeEntry<Spec>>
	class GameplayEffectRuntimeSystem
	{
		class OperationScope;

	public:
		using Callbacks = GameplayEffectRuntimeCallbacks<ActiveEffect, Spec>;
		using Collection = GameplayEffectCollection<ActiveEffect>;

		GameplayEffectRuntimeSystem(AttributeSystem& attributes, vega::GameplayTagContainer& ownedTags, Callbacks callbacks = {})
			: mAttributes{ attributes },
			mOwnedTags{ ownedTags },
			mCallbacks{ std::move(callbacks) }
		{
		}

		void SetCallbacks(Callbacks callbacks) { mCallbacks = std::move(callbacks); }

		GameplayEffectHandle ApplyEffect(const GameplayEffectDefinition& definition, const GameplayEffectSourceContext& context = {})
		{
			Spec spec;
			static_cast<GameplayEffectSpec&>(spec) = MakeGameplayEffectSpec(definition);
			return ApplyEffect(spec, context);
		}

		GameplayEffectHandle ApplyEffect(const Spec& spec, const GameplayEffectSourceContext& context = {})
		{
			const GameplayEffectDefinition& definition = spec.definition;
			if (!CanApplyEffect(definition) || !Orchestrator::IsApplicableSpec(spec)) return {};

			const GameplayEffectHandle newHandle = mActiveEffects.AllocateHandle();
			if (definition.durationPolicy == GameplayEffectDurationPolicy::Instant) return ApplyInstantEffect(spec, newHandle);

			ActiveEffect* stackingTarget = FindStackingTarget(definition, context);
			if (!stackingTarget) return CreateActiveEffect(spec, context, newHandle);
			if (!stackingTarget->CanBeginOperation()) return {};
			const GameplayEffectApplicationKind kind = Orchestrator::ResolveApplication(definition, true, stackingTarget->stackCount, spec.maxStacks);
			return ReapplyActiveEffect(*stackingTarget, kind, spec, context);
		}

		bool RefreshEffectDuration(GameplayEffectHandle handle)
		{
			const ActiveEffect* effect = FindEffect(handle);
			if (!effect || !Orchestrator::CanRefreshDuration(effect->spec.definition.durationPolicy)) return false;
			return RunEffectOperation(handle, [&](OperationScope& scope) -> bool
			{
				ActiveEffect& current = scope.Effect();
				current.RefreshDuration(current.spec.duration, current.spec.definition.stackDecayInterval);
				return scope.Run(
					[&] { Notify(mCallbacks.changed, current); },
					[&] { Notify(mCallbacks.collectionChanged); }
				);
			});
		}

		// Returns true when the effect was removed or its removal was queued behind an open scope.
		bool RemoveEffect(GameplayEffectHandle handle)
		{
			ActiveEffect* effect = FindEffect(handle);
			if (!effect) return false;
			RequestRemoval(*effect);
			if (mScopeDepth > 0) return true;
			FlushDeferred();
			return !FindEffect(handle);
		}

		bool RemoveEffectAt(std::size_t index)
		{
			const ActiveEffect* effect = mActiveEffects.At(index);
			return effect && RemoveEffect(effect->handle);
		}

		std::size_t RemoveEffectsIf(const std::function<bool(const ActiveEffect&)>& predicate)
		{
			if (!predicate) return 0;
			std::size_t removedCount = 0;
			for (const GameplayEffectHandle handle : GetHandles())
			{
				bool matched = false;
				RunEffectOperation(handle, [&](OperationScope& scope) { matched = predicate(scope.Effect()); });
				if (matched && RemoveEffect(handle)) ++removedCount;
			}
			return removedCount;
		}

		void Tick(float deltaTime)
		{
			// Callbacks may add or remove effects, so ticking uses a stable handle snapshot. The
			// outer tick reuses one scratch buffer; a nested Tick gets its own snapshot.
			if (mTickInProgress)
			{
				TickHandles(GetHandles(), deltaTime);
				return;
			}
			mTickHandleScratch.clear();
			mTickHandleScratch.reserve(mActiveEffects.GetAll().size());
			for (const ActiveEffect& effect : mActiveEffects.GetAll()) mTickHandleScratch.push_back(effect.handle);

			FlagGuard tickGuard{ mTickInProgress };
			TickHandles(mTickHandleScratch, deltaTime);
		}

		// Inside an open scope the clear is only requested and runs when the scope closes. That case, a
		// clear already in progress, and a failed removal that kept an effect all report Pending; the clear
		// stays requested and the next flush retries it. Exceptions from callbacks still propagate.
		ClearResult Clear()
		{
			if (mIsClearing) return ClearResult::Pending;
			mClearRequested = true;
			if (mScopeDepth > 0) return ClearResult::Pending;
			FlushDeferred();
			return mCleanupIncomplete ? ClearResult::Pending : ClearResult::Completed;
		}

		ActiveEffect* FindEffect(GameplayEffectHandle handle) { return mActiveEffects.Find(handle); }
		const ActiveEffect* FindEffect(GameplayEffectHandle handle) const { return mActiveEffects.Find(handle); }

		ActiveEffect* FindEffectById(const std::string& effectId)
		{
			return mActiveEffects.FindFirst([&](const ActiveEffect& effect) { return effect.spec.definition.effectId == effectId; });
		}

		const ActiveEffect* FindEffectById(const std::string& effectId) const
		{
			return mActiveEffects.FindFirst([&](const ActiveEffect& effect) { return effect.spec.definition.effectId == effectId; });
		}

		std::vector<GameplayEffectHandle> GetHandles() const
		{
			std::vector<GameplayEffectHandle> handles;
			handles.reserve(mActiveEffects.GetAll().size());
			for (const ActiveEffect& effect : mActiveEffects.GetAll()) handles.push_back(effect.handle);
			return handles;
		}

		std::vector<GameplayEffectRuntimeSnapshot> BuildSnapshots() const
		{
			std::vector<GameplayEffectRuntimeSnapshot> snapshots;
			snapshots.reserve(mActiveEffects.GetAll().size());
			for (const ActiveEffect& effect : mActiveEffects.GetAll()) snapshots.push_back(effect.BuildRuntimeSnapshot());
			return snapshots;
		}

		std::vector<GameplayEffectRuntimeSnapshot> BuildDebugSnapshots() const
		{
			std::vector<GameplayEffectRuntimeSnapshot> snapshots;
			snapshots.reserve(mActiveEffects.GetAll().size());
			for (const ActiveEffect& effect : mActiveEffects.GetAll()) snapshots.push_back(effect.BuildDebugSnapshot());
			return snapshots;
		}

		bool CanApplyEffect(const GameplayEffectDefinition& definition) const
		{
			if (!ShouldContinueGlobal() || !CanApplyGameplayEffect(definition, mOwnedTags)) return false;
			if (definition.immunityCategory.empty()) return true;
			for (const ActiveEffect& activeEffect : mActiveEffects.GetAll())
			{
				if (Orchestrator::GrantsImmunity(activeEffect.spec.definition.grantedImmunityCategory, definition.immunityCategory)) return false;
			}
			return true;
		}

		template <typename EventContext, typename EventPhase, typename PhaseResolver, typename EventProcessor, typename ContinuePredicate>
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
				for (const GameplayEffectHandle handle : handles)
				{
					if (!std::invoke(shouldContinue, context)) break;
					RunEffectOperation(handle, [&](OperationScope& scope)
					{
						ActiveEffect& effect = scope.Effect();
						if (!scope.Alive() || std::invoke(resolvePhase, effect) != phase) return;
						GameplayEffectBehaviorResult result;
						if (!scope.Step([&] { result = std::invoke(process, effect, context); })) return;
						PublishBehaviorResult(scope, effect, result);
					});
				}
			}
		}

	private:
		using Orchestrator = GameplayEffectLifecycleOrchestrator;

		struct FlagGuard
		{
			explicit FlagGuard(bool& flag) : mFlag{ flag } { mFlag = true; }
			~FlagGuard() { mFlag = false; }
			FlagGuard(const FlagGuard&) = delete;
			FlagGuard& operator=(const FlagGuard&) = delete;
			bool& mFlag;
		};

		class OperationScope
		{
		public:
			OperationScope(GameplayEffectRuntimeSystem& runtime, ActiveEffect& effect) : mRuntime{ runtime }, mEffect{ effect }
			{
				mEffect.operationActive = true;
				++mRuntime.mScopeDepth;
			}

			OperationScope(const OperationScope&) = delete;
			OperationScope& operator=(const OperationScope&) = delete;

			// Reached only when the operation threw: cancel the effect, let the exception propagate.
			~OperationScope()
			{
				if (mClosed) return;
				mEffect.operationActive = false;
				mRuntime.RequestRemoval(mEffect);
				try { mRuntime.LeaveScope(); }
				catch (...) {}
			}

			ActiveEffect& Effect() const { return mEffect; }

			// The one cancellation check: false once this effect was removed or the system is stopping.
			bool Alive()
			{
				if (mEffect.IsRemovalPending()) return false;
				if (mRuntime.ShouldContinueGlobal()) return true;
				mRuntime.RequestRemoval(mEffect);
				return false;
			}

			// Runs one step (typically a callback), then reports whether the effect is still alive.
			template <typename Callable>
			bool Step(Callable&& step)
			{
				std::invoke(step);
				return Alive();
			}

			// Runs steps in order and stops at the first one after which the effect is cancelled.
			template <typename... Steps>
			bool Run(Steps&&... steps)
			{
				return (... && Step(steps));
			}

			// Closes the scope; returns whether the effect survived (it may be erased by the queue drain).
			bool Close()
			{
				Alive();
				const GameplayEffectHandle handle = mEffect.handle;
				mClosed = true;
				mEffect.operationActive = false;
				mRuntime.LeaveScope();
				const ActiveEffect* effect = mRuntime.FindEffect(handle);
				return effect && !effect->removeRequested;
			}

		private:
			GameplayEffectRuntimeSystem& mRuntime;
			ActiveEffect& mEffect;
			bool mClosed = false;
		};

		template <typename Callback, typename... Args>
		static void Notify(const Callback& callback, Args&&... args)
		{
			if (callback) callback(std::forward<Args>(args)...);
		}

		bool ShouldContinueGlobal() const
		{
			return !mIsClearing && !mClearRequested && !mCleanupIncomplete && (!mCallbacks.shouldContinue || mCallbacks.shouldContinue());
		}

		template <typename Operation>
		auto RunEffectOperation(GameplayEffectHandle handle, Operation&& operation) -> decltype(operation(std::declval<OperationScope&>()))
		{
			using Result = decltype(operation(std::declval<OperationScope&>()));
			ActiveEffect* effect = FindEffect(handle);
			if (!effect || !effect->CanBeginOperation() || !ShouldContinueGlobal()) return Result();

			OperationScope scope{ *this, *effect };
			if constexpr (std::is_void_v<Result>)
			{
				operation(scope);
				scope.Close();
			}
			else
			{
				Result result = operation(scope);
				return scope.Close() ? std::move(result) : Result{};
			}
		}

		void LeaveScope()
		{
			if (--mScopeDepth == 0) FlushDeferred();
		}

		void RequestRemoval(ActiveEffect& effect)
		{
			effect.removeRequested = true;
			if (effect.removalInProgress) return;
			if (std::find(mPendingRemovals.begin(), mPendingRemovals.end(), effect.handle) == mPendingRemovals.end())
			{
				mPendingRemovals.push_back(effect.handle);
			}
		}

		// Runs the queued removals and any requested clear. Called only with no scope open; the
		// depth bump makes requests raised by removal callbacks queue up instead of re-entering.
		void FlushDeferred()
		{
			std::exception_ptr error;
			++mScopeDepth;
			for (std::size_t index = 0; index < mPendingRemovals.size(); ++index) ExecuteRemoval(mPendingRemovals[index], error);
			if (mClearRequested && !mIsClearing) DrainRequestedClear(error);
			mPendingRemovals.clear();
			--mScopeDepth;
			if (error) std::rethrow_exception(error);
		}

		void DrainRequestedClear(std::exception_ptr& error)
		{
			mIsClearing = true;
			for (ActiveEffect& effect : mActiveEffects.GetAll()) effect.removeRequested = true;

			std::vector<GameplayEffectHandle> handles;
			try { handles = GetHandles(); }
			catch (...)
			{
				if (!error) error = std::current_exception();
				mIsClearing = false;
				mCleanupIncomplete = !mActiveEffects.GetAll().empty();
				return;
			}
			for (const GameplayEffectHandle handle : handles) ExecuteRemoval(handle, error);
			mIsClearing = false;
			// No scope is open here, so anything left behind is a failed removal that needs a retry.
			mClearRequested = mCleanupIncomplete = !mActiveEffects.GetAll().empty();
		}

		// Tears one effect down. Errors never abort the sequence: the first one is kept for the caller.
		void ExecuteRemoval(GameplayEffectHandle handle, std::exception_ptr& error)
		{
			ActiveEffect* effect = FindEffect(handle);
			if (!effect) return;
			effect->removeRequested = true;
			effect->removalInProgress = true;
			const auto capture = [&error](auto&& operation) -> bool
			{
				try { operation(); return true; }
				catch (...) { if (!error) error = std::current_exception(); return false; }
			};
			capture([&] { RemoveGameplayEffectTags(*effect, mOwnedTags); });
			capture([&] { RemoveGameplayEffectModifiers(*effect, mAttributes); });
			const bool removingCompleted = capture([&] { Notify(mCallbacks.removing, *effect); });
			effect->removalInProgress = false;
			// An incomplete teardown keeps the effect (still flagged) so a later removal or Clear retries it.
			if (!removingCompleted || !effect->appliedModifierHandles.empty() || !effect->appliedGrantedTags.empty()) return;

			mActiveEffects.Erase(handle);
			capture([&] { Notify(mCallbacks.removed, handle); });
			capture([&] { Notify(mCallbacks.collectionChanged); });
		}

		ActiveEffect* FindStackingTarget(const GameplayEffectDefinition& definition, const GameplayEffectSourceContext& context)
		{
			return mActiveEffects.FindFirst([&](const ActiveEffect& activeEffect)
			{
				return !activeEffect.IsAwaitingDeferredRemoval() &&
					Orchestrator::MatchesStackingTarget(definition, activeEffect.spec.definition.effectId, activeEffect.sourceScope, context.sourceScope);
			});
		}

		GameplayEffectHandle ApplyInstantEffect(const Spec& spec, GameplayEffectHandle handle)
		{
			// Instant effects have no entry to cancel, so only the system-wide stop is checked.
			const auto step = [this](auto&& notify) { notify(); return ShouldContinueGlobal(); };
			ApplyInstantGameplayEffect(spec, mAttributes, [this] { return ShouldContinueGlobal(); });
			const bool completed = ShouldContinueGlobal() &&
				step([&] { Notify(mCallbacks.applied, handle); }) &&
				step([&] { Notify(mCallbacks.removed, handle); }) &&
				step([&] { Notify(mCallbacks.collectionChanged); });
			return completed ? handle : GameplayEffectHandle{};
		}

		GameplayEffectHandle CreateActiveEffect(const Spec& spec, const GameplayEffectSourceContext& context, GameplayEffectHandle handle)
		{
			ActiveEffect& newEffect = mActiveEffects.Emplace(handle);
			return RunEffectOperation(handle, [&](OperationScope& scope) -> GameplayEffectHandle
			{
				newEffect.spec = spec;
				newEffect.Initialize(handle, spec.duration, spec.definition.stackDecayInterval, spec.attributes);
				const bool applied = scope.Run(
					[&] { BindSource(newEffect, context); },
					[&] { Initialize(newEffect); },
					[&] { ApplyModifiers(scope, newEffect); },
					[&] { GrantGameplayEffectTags(newEffect.spec.definition, newEffect, mOwnedTags); Notify(mCallbacks.activated, newEffect); },
					[&] { Notify(mCallbacks.applied, handle); },
					[&] { Notify(mCallbacks.collectionChanged); }
				);
				return applied ? handle : GameplayEffectHandle{};
			});
		}

		GameplayEffectHandle ReapplyActiveEffect(ActiveEffect& target, GameplayEffectApplicationKind kind, const Spec& spec, const GameplayEffectSourceContext& context)
		{
			return RunEffectOperation(target.handle, [&](OperationScope& scope) -> GameplayEffectHandle
			{
				const bool reapplied = kind == GameplayEffectApplicationKind::RefreshStackDuration
					? ReapplyCappedStack(scope, target, spec, context)
					: ReapplyStackOrRefresh(scope, target, kind, spec, context);
				const bool published = reapplied && scope.Run(
					[&] { Notify(mCallbacks.changed, target); },
					[&] { Notify(mCallbacks.collectionChanged); }
				);
				return published ? target.handle : GameplayEffectHandle{};
			});
		}

		// Duration-only policy: keep the active attributes and modifiers, refresh duration and source.
		bool ReapplyCappedStack(OperationScope& scope, ActiveEffect& target, const Spec& spec, const GameplayEffectSourceContext& context)
		{
			Orchestrator::RefreshCappedSpec(target.spec, spec);
			const bool alive = scope.Run(
				[&] { BindSource(target, context); },
				[&] { if (mCallbacks.cappedReapply) mCallbacks.cappedReapply(target, spec); }
			);
			if (alive) Orchestrator::ApplyStackingState(GameplayEffectApplicationKind::RefreshStackDuration, target, spec.duration, spec.definition.stackDecayInterval, spec.maxStacks);
			return alive;
		}

		bool ReapplyStackOrRefresh(OperationScope& scope, ActiveEffect& target, GameplayEffectApplicationKind kind, const Spec& spec, const GameplayEffectSourceContext& context)
		{
			const bool refresh = kind == GameplayEffectApplicationKind::RefreshActive;
			// A full refresh first tears down the previous activation (tags, modifiers, presentation).
			if (refresh && !scope.Run(
				[&] { RemoveGameplayEffectTags(target, mOwnedTags); },
				[&] { RemoveGameplayEffectModifiers(target, mAttributes); },
				[&] { Notify(mCallbacks.removing, target); }))
			{
				return false;
			}

			target.spec = spec;
			if (!scope.Step([&] { BindSource(target, context); })) return false;
			const bool stackAdded = Orchestrator::ApplyStackingState(kind, target, spec.duration, spec.definition.stackDecayInterval, spec.maxStacks);
			if (refresh)
			{
				return scope.Run(
					[&] { Refresh(target); },
					[&] { ApplyModifiers(scope, target); },
					[&] { GrantGameplayEffectTags(target.spec.definition, target, mOwnedTags); Notify(mCallbacks.activated, target); }
				);
			}
			if (!stackAdded) return true;

			bool customStackHandled = false;
			return scope.Run(
				[&] { customStackHandled = AddStack(target); },
				[&] { if (!customStackHandled) target.RefreshRuntimeAttributesFromSpec(); ApplyModifiers(scope, target); },
				[&] { Notify(mCallbacks.stackChanged, target); }
			);
		}

		void ApplyModifiers(OperationScope& scope, ActiveEffect& effect)
		{
			ApplyGameplayEffectModifiers(effect.spec, effect, mAttributes, [&scope] { return scope.Alive(); });
		}

		void TickHandles(const std::vector<GameplayEffectHandle>& handles, float deltaTime)
		{
			for (const GameplayEffectHandle handle : handles)
			{
				RunEffectOperation(handle, [&](OperationScope& scope) { TickEffect(scope, scope.Effect(), deltaTime); });
			}
		}

		void TickEffect(OperationScope& scope, ActiveEffect& effect, float deltaTime)
		{
			if (Orchestrator::IsDecayAfterDuration(effect.spec.definition))
			{
				TickDecayEffect(scope, effect, deltaTime);
				return;
			}
			if (!TickBehavior(scope, effect, deltaTime)) return;
			PublishDurationTick(scope, effect, AdvanceDuration(effect, deltaTime), true);
		}

		// Decay effects tick in slices that end on each stack-decay boundary.
		void TickDecayEffect(OperationScope& scope, ActiveEffect& effect, float deltaTime)
		{
			for (float remaining = deltaTime; remaining > 0.f;)
			{
				const GameplayEffectDefinition& definition = effect.spec.definition;
				const float slice = Orchestrator::GetTickSliceDuration(definition.durationPolicy, definition.stackLifetimePolicy, effect, remaining);
				if (slice > 0.f && (!TickBehavior(scope, effect, slice) || !Orchestrator::IsDecayAfterDuration(effect.spec.definition))) return;

				const GameplayEffectDurationTickResult tick = AdvanceDuration(effect, slice);
				if (slice <= 0.f && !tick.stackChanged && !tick.expired) return;
				if (!PublishDurationTick(scope, effect, tick, false)) return;
				remaining -= slice;
			}
		}

		GameplayEffectDurationTickResult AdvanceDuration(ActiveEffect& effect, float deltaTime)
		{
			const GameplayEffectDefinition& definition = effect.spec.definition;
			return Orchestrator::TickDuration(definition.durationPolicy, definition.stackLifetimePolicy, effect, deltaTime);
		}

		bool TickBehavior(OperationScope& scope, ActiveEffect& effect, float deltaTime)
		{
			GameplayEffectBehaviorResult result;
			if (!scope.Step([&] { if (mCallbacks.tick) result = mCallbacks.tick(effect, deltaTime); })) return false;
			return PublishBehaviorResult(scope, effect, result);
		}

		// Returns false when the effect was cancelled or its behavior asked for removal.
		bool PublishBehaviorResult(OperationScope& scope, ActiveEffect& effect, const GameplayEffectBehaviorResult& result)
		{
			for (const GameplayEffectBehaviorEvent& event : result.events)
			{
				if (!scope.Step([&] { Notify(mCallbacks.behaviorEvent, event); })) return false;
			}
			if (result.changed && !scope.Run([&] { Notify(mCallbacks.changed, effect); }, [&] { Notify(mCallbacks.collectionChanged); })) return false;
			if (!result.removeEffect) return true;
			RemoveEffect(effect.handle);
			return false;
		}

		// Publishes a duration tick as stackChanged, changed, then collectionChanged; an expired
		// effect is removed after its stack notifications. Returns false once the effect is done.
		bool PublishDurationTick(OperationScope& scope, ActiveEffect& effect, const GameplayEffectDurationTickResult& tick, bool notifyPlainTicks)
		{
			const bool notify = tick.stackChanged || (notifyPlainTicks && tick.changed && !tick.expired);
			if (notify)
			{
				const bool published = tick.stackChanged
					? scope.Run([&] { Notify(mCallbacks.stackChanged, effect); }, [&] { Notify(mCallbacks.changed, effect); })
					: scope.Step([&] { Notify(mCallbacks.changed, effect); });
				if (!published) return false;
			}
			if (tick.expired)
			{
				RemoveEffect(effect.handle);
				return false;
			}
			return !notify || scope.Step([&] { Notify(mCallbacks.collectionChanged); });
		}

		void BindSource(ActiveEffect& effect, const GameplayEffectSourceContext& context)
		{
			effect.sourceObject = context.sourceObject;
			effect.sourceScope = context.sourceScope;
			effect.runtimeContext = context.runtimeContext;
			Notify(mCallbacks.bindSource, effect, context);
		}

		void Initialize(ActiveEffect& effect)
		{
			if (mCallbacks.initialize) mCallbacks.initialize(effect);
			else effect.ResetRuntimeAttributesFromSpec();
		}

		void Refresh(ActiveEffect& effect)
		{
			if (mCallbacks.refresh) mCallbacks.refresh(effect);
			else effect.ResetRuntimeAttributesFromSpec();
		}

		bool AddStack(ActiveEffect& effect) { return mCallbacks.addStack && mCallbacks.addStack(effect); }

		AttributeSystem& mAttributes;
		vega::GameplayTagContainer& mOwnedTags;
		Callbacks mCallbacks;
		Collection mActiveEffects;
		std::size_t mScopeDepth = 0;
		std::vector<GameplayEffectHandle> mPendingRemovals;
		bool mIsClearing = false;
		bool mClearRequested = false;
		bool mCleanupIncomplete = false;
		std::vector<GameplayEffectHandle> mTickHandleScratch;
		bool mTickInProgress = false;
	};
}
