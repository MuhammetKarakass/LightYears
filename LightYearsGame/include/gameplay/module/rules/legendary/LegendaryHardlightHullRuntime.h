#pragma once

#include "gameplay/module/rules/legendary/LegendaryHardlightHullRules.h"
#include "attributes/AttributeSystem.h"

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendaryHardlightHullRuntime final
	{
	public:
		LegendaryHardlightHullRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryHardlightHullRuntime();
		LegendaryHardlightHullRuntime(const LegendaryHardlightHullRuntime&) = delete;
		LegendaryHardlightHullRuntime& operator=(const LegendaryHardlightHullRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryHardlightHullRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		void Refresh();
		const List<ResolvedModuleLegendaryHardlightHullRule>& GetRules() const { return mRules; }

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendaryHardlightHullRule> mRules;
		sas::AttributeModifierHandle mHealthHandle;
		sas::AttributeModifierHandle mShieldHandle;
		float mConvertedHealth = 0.f;
		float mConvertedShield = 0.f;
		bool mRefreshing = false;
	};
}
