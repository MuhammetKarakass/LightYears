#pragma once

#include "gameplay/module/rules/legendary/LegendaryBulwarkPulseRules.h"

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;

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
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendaryBulwarkPulseRule> mRules;
	};
}
