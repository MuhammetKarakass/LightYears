#include "gameplay/module/rules/legendary/LegendaryMeltdownBarrelRuntime.h"

namespace vega
{
	LegendaryMeltdownBarrelRuntime::LegendaryMeltdownBarrelRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryMeltdownBarrelRuntime::~LegendaryMeltdownBarrelRuntime() { Clear(); }

	void LegendaryMeltdownBarrelRuntime::Replace(const List<ResolvedModuleLegendaryMeltdownBarrelRule>& rules)
	{
		Clear();
		mRules = rules;
	}

	void LegendaryMeltdownBarrelRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
		// TODO(Legendary MeltdownBarrel behavior task): implement subsystem hooks and owned effect cleanup here.
	}

	void LegendaryMeltdownBarrelRuntime::Clear()
	{
		mRules.clear();
	}
}
