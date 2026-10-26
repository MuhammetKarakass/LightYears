#include "gameplay/module/rules/legendary/LegendaryIonCometRuntime.h"

namespace vega
{
	LegendaryIonCometRuntime::LegendaryIonCometRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryIonCometRuntime::~LegendaryIonCometRuntime() { Clear(); }

	void LegendaryIonCometRuntime::Replace(const List<ResolvedModuleLegendaryIonCometRule>& rules)
	{
		Clear();
		mRules = rules;
	}

	void LegendaryIonCometRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
		// TODO(Legendary IonComet behavior task): implement subsystem hooks and owned effect cleanup here.
	}

	void LegendaryIonCometRuntime::Clear()
	{
		mRules.clear();
	}
}
