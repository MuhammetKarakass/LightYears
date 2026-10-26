#pragma once

#include "gameplay/module/rules/legendary/LegendaryPrimedArsenalRules.h"

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendaryPrimedArsenalRuntime final
	{
	public:
		LegendaryPrimedArsenalRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryPrimedArsenalRuntime();
		LegendaryPrimedArsenalRuntime(const LegendaryPrimedArsenalRuntime&) = delete;
		LegendaryPrimedArsenalRuntime& operator=(const LegendaryPrimedArsenalRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryPrimedArsenalRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendaryPrimedArsenalRule>& GetRules() const { return mRules; }

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendaryPrimedArsenalRule> mRules;
	};
}
