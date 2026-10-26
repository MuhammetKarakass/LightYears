#include "gameplay/module/rules/legendary/LegendaryTwinRackRuntime.h"

namespace vega
{
	LegendaryTwinRackRuntime::LegendaryTwinRackRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryTwinRackRuntime::~LegendaryTwinRackRuntime() { Clear(); }

	void LegendaryTwinRackRuntime::Replace(const List<ResolvedModuleLegendaryTwinRackRule>& rules)
	{
		Clear();
		mRules = rules;
	}

	void LegendaryTwinRackRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
		// TODO(Legendary TwinRack behavior task): implement subsystem hooks and owned effect cleanup here.
	}

	void LegendaryTwinRackRuntime::Clear()
	{
		mRules.clear();
	}
}
