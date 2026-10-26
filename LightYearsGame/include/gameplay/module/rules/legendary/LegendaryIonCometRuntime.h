#pragma once

#include "gameplay/module/rules/legendary/LegendaryIonCometRules.h"

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendaryIonCometRuntime final
	{
	public:
		LegendaryIonCometRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryIonCometRuntime();
		LegendaryIonCometRuntime(const LegendaryIonCometRuntime&) = delete;
		LegendaryIonCometRuntime& operator=(const LegendaryIonCometRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryIonCometRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendaryIonCometRule>& GetRules() const { return mRules; }

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendaryIonCometRule> mRules;
	};
}
