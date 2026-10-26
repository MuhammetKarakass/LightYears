#pragma once

#include "gameplay/module/rules/legendary/LegendaryUnderdogCoreRules.h"

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendaryUnderdogCoreRuntime final
	{
	public:
		LegendaryUnderdogCoreRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryUnderdogCoreRuntime();
		LegendaryUnderdogCoreRuntime(const LegendaryUnderdogCoreRuntime&) = delete;
		LegendaryUnderdogCoreRuntime& operator=(const LegendaryUnderdogCoreRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryUnderdogCoreRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendaryUnderdogCoreRule>& GetRules() const { return mRules; }

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendaryUnderdogCoreRule> mRules;
	};
}
