#pragma once

#include "gameplay/module/rules/legendary/LegendaryNeuralHijackerRules.h"

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendaryNeuralHijackerRuntime final
	{
	public:
		LegendaryNeuralHijackerRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryNeuralHijackerRuntime();
		LegendaryNeuralHijackerRuntime(const LegendaryNeuralHijackerRuntime&) = delete;
		LegendaryNeuralHijackerRuntime& operator=(const LegendaryNeuralHijackerRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryNeuralHijackerRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendaryNeuralHijackerRule>& GetRules() const { return mRules; }

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendaryNeuralHijackerRule> mRules;
	};
}
