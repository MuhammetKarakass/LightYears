#include "gameplay/module/rules/legendary/LegendaryReactionCoreRuntime.h"

namespace vega
{
	LegendaryReactionCoreRuntime::LegendaryReactionCoreRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryReactionCoreRuntime::~LegendaryReactionCoreRuntime() { Clear(); }

	void LegendaryReactionCoreRuntime::Replace(const List<ResolvedModuleLegendaryReactionCoreRule>& rules)
	{
		Clear();
		mRules = rules;
	}

	void LegendaryReactionCoreRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
		// TODO(Legendary ReactionCore behavior task): implement subsystem hooks and owned effect cleanup here.
	}

	void LegendaryReactionCoreRuntime::Clear()
	{
		mRules.clear();
	}
}
