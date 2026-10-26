#pragma once

#include "gameplay/module/rules/legendary/LegendaryTwinRackRules.h"

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendaryTwinRackRuntime final
	{
	public:
		LegendaryTwinRackRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryTwinRackRuntime();
		LegendaryTwinRackRuntime(const LegendaryTwinRackRuntime&) = delete;
		LegendaryTwinRackRuntime& operator=(const LegendaryTwinRackRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryTwinRackRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendaryTwinRackRule>& GetRules() const { return mRules; }

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendaryTwinRackRule> mRules;
	};
}
