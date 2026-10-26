#pragma once

#include "gameplay/module/rules/legendary/LegendaryBlinkDriveRules.h"

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendaryBlinkDriveRuntime final
	{
	public:
		LegendaryBlinkDriveRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryBlinkDriveRuntime();
		LegendaryBlinkDriveRuntime(const LegendaryBlinkDriveRuntime&) = delete;
		LegendaryBlinkDriveRuntime& operator=(const LegendaryBlinkDriveRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryBlinkDriveRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendaryBlinkDriveRule>& GetRules() const { return mRules; }

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendaryBlinkDriveRule> mRules;
	};
}
