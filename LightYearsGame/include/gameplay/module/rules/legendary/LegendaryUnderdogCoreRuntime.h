#pragma once

#include "gameplay/module/rules/legendary/LegendaryUnderdogCoreRules.h"
#include "attributes/AttributeSystem.h"

#include <cstddef>

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendaryUnderdogCoreRuntime final
	{
	public:
		LegendaryUnderdogCoreRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryUnderdogCoreRuntime();
		LegendaryUnderdogCoreRuntime(const LegendaryUnderdogCoreRuntime&) = delete;
		LegendaryUnderdogCoreRuntime& operator=(const LegendaryUnderdogCoreRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryUnderdogCoreRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendaryUnderdogCoreRule>& GetRules() const { return mRules; }

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendaryUnderdogCoreRule> mRules;
		List<sas::AttributeModifierHandle> mOwnerHandles;
		List<std::size_t> mPrimaryHandles;
		List<std::size_t> mCooldownHandles;
	};
}
