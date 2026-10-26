#include "gameplay/module/rules/legendary/LegendarySecondHardpointRuntime.h"

namespace vega
{
	LegendarySecondHardpointRuntime::LegendarySecondHardpointRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendarySecondHardpointRuntime::~LegendarySecondHardpointRuntime() { Clear(); }

	void LegendarySecondHardpointRuntime::Replace(const List<ResolvedModuleLegendarySecondHardpointRule>& rules)
	{
		Clear();
		mRules = rules;
	}

	void LegendarySecondHardpointRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
		// TODO(Legendary SecondHardpoint behavior task): implement subsystem hooks and owned effect cleanup here.
	}

	void LegendarySecondHardpointRuntime::Clear()
	{
		mRules.clear();
	}
}
