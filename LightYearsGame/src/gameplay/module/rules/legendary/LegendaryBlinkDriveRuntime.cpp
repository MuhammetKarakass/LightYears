#include "gameplay/module/rules/legendary/LegendaryBlinkDriveRuntime.h"

namespace vega
{
	LegendaryBlinkDriveRuntime::LegendaryBlinkDriveRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryBlinkDriveRuntime::~LegendaryBlinkDriveRuntime() { Clear(); }

	void LegendaryBlinkDriveRuntime::Replace(const List<ResolvedModuleLegendaryBlinkDriveRule>& rules)
	{
		Clear();
		mRules = rules;
	}

	void LegendaryBlinkDriveRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
		// TODO(Legendary BlinkDrive behavior task): implement subsystem hooks and owned effect cleanup here.
	}

	void LegendaryBlinkDriveRuntime::Clear()
	{
		mRules.clear();
	}
}
