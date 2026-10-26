#pragma once

#include "gameplay/module/rules/legendary/LegendarySecondHardpointRules.h"

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendarySecondHardpointRuntime final
	{
	public:
		LegendarySecondHardpointRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendarySecondHardpointRuntime();
		LegendarySecondHardpointRuntime(const LegendarySecondHardpointRuntime&) = delete;
		LegendarySecondHardpointRuntime& operator=(const LegendarySecondHardpointRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendarySecondHardpointRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendarySecondHardpointRule>& GetRules() const { return mRules; }

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendarySecondHardpointRule> mRules;
	};
}
