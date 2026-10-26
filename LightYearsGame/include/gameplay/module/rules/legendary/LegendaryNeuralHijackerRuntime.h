#pragma once

#include "gameplay/module/rules/legendary/LegendaryNeuralHijackerRules.h"
#include "gameplay/combat/LethalHitInterceptors.h"

#include <cstddef>

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;
	class SpaceShip;
	class EnemyActor;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	// Neural Hijacker: the owner's lethal hits on eligible enemies become takeovers (shared CombatantTakeover mechanic)
	// instead of kills. The hit still counts as the kill and grants its rewards once. Allies end with the module.
	class LegendaryNeuralHijackerRuntime final
	{
	public:
		LegendaryNeuralHijackerRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryNeuralHijackerRuntime();
		LegendaryNeuralHijackerRuntime(const LegendaryNeuralHijackerRuntime&) = delete;
		LegendaryNeuralHijackerRuntime& operator=(const LegendaryNeuralHijackerRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryNeuralHijackerRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendaryNeuralHijackerRule>& GetRules() const { return mRules; }
		// Live allies made by this runtime.
		std::size_t GetAllyCount() const;

	private:
		LethalDamageResolution Intercept(SpaceShip& target, const DamageContext& context);
		void EndAllies();

		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		SpaceShip* mShip = nullptr;
		List<ResolvedModuleLegendaryNeuralHijackerRule> mRules;
		LethalHitInterceptorList::Registration mInterceptor;
		List<weak_ptr<EnemyActor>> mAllies;
	};
}
