#pragma once

#include "gameplay/module/rules/legendary/LegendaryMeltdownBarrelRules.h"

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendaryMeltdownBarrelRuntime final
	{
	public:
		LegendaryMeltdownBarrelRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryMeltdownBarrelRuntime();
		LegendaryMeltdownBarrelRuntime(const LegendaryMeltdownBarrelRuntime&) = delete;
		LegendaryMeltdownBarrelRuntime& operator=(const LegendaryMeltdownBarrelRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryMeltdownBarrelRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendaryMeltdownBarrelRule>& GetRules() const { return mRules; }

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendaryMeltdownBarrelRule> mRules;
	};
}
