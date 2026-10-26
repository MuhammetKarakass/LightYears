#pragma once

#include "gameplay/module/rules/legendary/LegendaryParasiteHullRules.h"

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendaryParasiteHullRuntime final
	{
	public:
		LegendaryParasiteHullRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryParasiteHullRuntime();
		LegendaryParasiteHullRuntime(const LegendaryParasiteHullRuntime&) = delete;
		LegendaryParasiteHullRuntime& operator=(const LegendaryParasiteHullRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryParasiteHullRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendaryParasiteHullRule>& GetRules() const { return mRules; }

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendaryParasiteHullRule> mRules;
	};
}
