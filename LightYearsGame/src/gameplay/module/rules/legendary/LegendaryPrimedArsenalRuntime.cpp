#include "gameplay/module/rules/legendary/LegendaryPrimedArsenalRuntime.h"

namespace vega
{
	LegendaryPrimedArsenalRuntime::LegendaryPrimedArsenalRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryPrimedArsenalRuntime::~LegendaryPrimedArsenalRuntime() { Clear(); }

	void LegendaryPrimedArsenalRuntime::Replace(const List<ResolvedModuleLegendaryPrimedArsenalRule>& rules)
	{
		Clear();
		mRules = rules;
	}

	void LegendaryPrimedArsenalRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
		// TODO(Legendary PrimedArsenal behavior task): implement subsystem hooks and owned effect cleanup here.
	}

	void LegendaryPrimedArsenalRuntime::Clear()
	{
		mRules.clear();
	}
}
