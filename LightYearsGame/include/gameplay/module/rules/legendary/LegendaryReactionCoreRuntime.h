#pragma once

#include "gameplay/module/rules/legendary/LegendaryReactionCoreRules.h"
#include "gameplay/damage/StatusReactions.h"
#include "gameplay/module/generated/ModuleGeneratedLease.h"
#include "framework/Delegate.h"

#include <cstdint>
#include <memory>

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;
	class SpaceShip;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	// Reaction Core: when an owner hit adds a new stack of one damage status to a target that already carried a different
	// one, both statuses are consumed and their reaction fires. Pairing runs on the next runtime tick, so the triggering
	// attack finishes its own damage and statuses first. Effects use the shared mechanics (shot deviation, timed pull, area
	// damage, armor break, control stun, fragment burst); this runtime only decides when and how strong.
	class LegendaryReactionCoreRuntime final
	{
	public:
		LegendaryReactionCoreRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryReactionCoreRuntime();
		LegendaryReactionCoreRuntime(const LegendaryReactionCoreRuntime&) = delete;
		LegendaryReactionCoreRuntime& operator=(const LegendaryReactionCoreRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryReactionCoreRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendaryReactionCoreRule>& GetRules() const { return mRules; }

		// Test/debug view: reactions fired while installed, and the remaining re-trigger lock of a target.
		int GetReactionCount(StatusReactionKind kind) const;
		float GetTargetLockSeconds(std::uint64_t targetId) const;

	private:
		struct PendingStack
		{
			weak_ptr<Actor> target;
			std::uint64_t targetId = 0;
			List<GameplayTag> added;
			List<GameplayTag> carriedBefore;
		};
		struct TargetLock
		{
			std::uint64_t targetId = 0;
			float remaining = 0.f;
		};

		void OnStatusStacksAdded(const StatusStacksAddedEvent& event);
		void ProcessPending();
		void Fire(const ResolvedModuleLegendaryReactionCoreRule& rule, StatusReactionKind kind, Actor& target);

		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		SpaceShip* mShip = nullptr;
		List<ResolvedModuleLegendaryReactionCoreRule> mRules;
		DelegateHandle mStatusHandle;
		std::shared_ptr<ModuleGeneratedLease> mLease = std::make_shared<ModuleGeneratedLease>();
		List<PendingStack> mPending;
		List<TargetLock> mLocks;
		List<std::pair<StatusReactionKind, int>> mReactionCounts;
	};
}
