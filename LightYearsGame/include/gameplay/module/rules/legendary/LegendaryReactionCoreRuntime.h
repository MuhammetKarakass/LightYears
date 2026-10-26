#pragma once

#include "gameplay/module/rules/legendary/LegendaryReactionCoreRules.h"

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
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

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendaryReactionCoreRule> mRules;
	};
}
