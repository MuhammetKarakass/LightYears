#include "gameplay/module/rules/legendary/LegendaryBulwarkPulseRuntime.h"

namespace vega
{
	LegendaryBulwarkPulseRuntime::LegendaryBulwarkPulseRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryBulwarkPulseRuntime::~LegendaryBulwarkPulseRuntime() { Clear(); }

	void LegendaryBulwarkPulseRuntime::Replace(const List<ResolvedModuleLegendaryBulwarkPulseRule>& rules)
	{
		Clear();
		mRules = rules;
	}

	void LegendaryBulwarkPulseRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
		// TODO(Legendary BulwarkPulse behavior task): implement subsystem hooks and owned effect cleanup here.
	}

	void LegendaryBulwarkPulseRuntime::Clear()
	{
		mRules.clear();
	}
}
