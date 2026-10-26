#include "gameplay/module/rules/legendary/LegendaryParasiteHullRuntime.h"

namespace vega
{
	LegendaryParasiteHullRuntime::LegendaryParasiteHullRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryParasiteHullRuntime::~LegendaryParasiteHullRuntime() { Clear(); }

	void LegendaryParasiteHullRuntime::Replace(const List<ResolvedModuleLegendaryParasiteHullRule>& rules)
	{
		Clear();
		mRules = rules;
	}

	void LegendaryParasiteHullRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
		// TODO(Legendary ParasiteHull behavior task): implement subsystem hooks and owned effect cleanup here.
	}

	void LegendaryParasiteHullRuntime::Clear()
	{
		mRules.clear();
	}
}
