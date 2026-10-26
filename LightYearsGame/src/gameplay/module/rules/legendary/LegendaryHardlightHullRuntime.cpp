#include "gameplay/module/rules/legendary/LegendaryHardlightHullRuntime.h"

namespace vega
{
	LegendaryHardlightHullRuntime::LegendaryHardlightHullRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryHardlightHullRuntime::~LegendaryHardlightHullRuntime() { Clear(); }

	void LegendaryHardlightHullRuntime::Replace(const List<ResolvedModuleLegendaryHardlightHullRule>& rules)
	{
		Clear();
		mRules = rules;
	}

	void LegendaryHardlightHullRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
		// TODO(Legendary HardlightHull behavior task): implement subsystem hooks and owned effect cleanup here.
	}

	void LegendaryHardlightHullRuntime::Clear()
	{
		mRules.clear();
	}
}
