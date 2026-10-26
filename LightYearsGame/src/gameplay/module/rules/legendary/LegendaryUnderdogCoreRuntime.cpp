#include "gameplay/module/rules/legendary/LegendaryUnderdogCoreRuntime.h"

namespace vega
{
	LegendaryUnderdogCoreRuntime::LegendaryUnderdogCoreRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryUnderdogCoreRuntime::~LegendaryUnderdogCoreRuntime() { Clear(); }

	void LegendaryUnderdogCoreRuntime::Replace(const List<ResolvedModuleLegendaryUnderdogCoreRule>& rules)
	{
		Clear();
		mRules = rules;
	}

	void LegendaryUnderdogCoreRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
		// TODO(Legendary UnderdogCore behavior task): implement subsystem hooks and owned effect cleanup here.
	}

	void LegendaryUnderdogCoreRuntime::Clear()
	{
		mRules.clear();
	}
}
