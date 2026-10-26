#include "gameplay/module/rules/legendary/LegendaryNeuralHijackerRuntime.h"

namespace vega
{
	LegendaryNeuralHijackerRuntime::LegendaryNeuralHijackerRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryNeuralHijackerRuntime::~LegendaryNeuralHijackerRuntime() { Clear(); }

	void LegendaryNeuralHijackerRuntime::Replace(const List<ResolvedModuleLegendaryNeuralHijackerRule>& rules)
	{
		Clear();
		mRules = rules;
	}

	void LegendaryNeuralHijackerRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
		// TODO(Legendary NeuralHijacker behavior task): implement subsystem hooks and owned effect cleanup here.
	}

	void LegendaryNeuralHijackerRuntime::Clear()
	{
		mRules.clear();
	}
}
