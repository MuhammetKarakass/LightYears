#pragma once

#include "gameplay/module/rules/legendary/LegendaryBulwarkPulseRules.h"
#include "gameplay/ability/runtime/AbilityLifecycleDispatcher.h"
#include "gameplay/module/generated/ModuleGeneratedLease.h"

#include <memory>

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;
	class SpaceShip;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendaryBulwarkPulseRuntime final
	{
	public:
		LegendaryBulwarkPulseRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryBulwarkPulseRuntime();
		LegendaryBulwarkPulseRuntime(const LegendaryBulwarkPulseRuntime&) = delete;
		LegendaryBulwarkPulseRuntime& operator=(const LegendaryBulwarkPulseRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryBulwarkPulseRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendaryBulwarkPulseRule>& GetRules() const { return mRules; }

	private:
		struct State
		{
			ResolvedModuleLegendaryBulwarkPulseRule rule;
			float cooldownRemaining = 0.f;
			shared_ptr<ModuleGeneratedLease> lease;
		};

		void OnAbilityActivated(const sas::AbilityLifecycleEvent& event);
		void Detonate(const ResolvedModuleLegendaryBulwarkPulseRule& rule, const shared_ptr<ModuleGeneratedLease>& lease);

		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		SpaceShip* mShip = nullptr;
		List<ResolvedModuleLegendaryBulwarkPulseRule> mRules;
		List<State> mStates;
		AbilityLifecycleObserverHandle mObserverHandle;
		bool mBusy = false;
	};
}
