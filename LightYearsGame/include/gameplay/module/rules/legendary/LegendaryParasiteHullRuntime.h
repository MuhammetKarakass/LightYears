#pragma once

#include "gameplay/module/rules/legendary/LegendaryParasiteHullRules.h"

#include <SFML/System/Vector2.hpp>

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;
	class PlayerSpaceShip;
	class EnemyActor;
	struct DamageContext;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	// Parasite Hull: on a lethal hit the owner escapes into the nearest eligible enemy (shared CombatantTakeover + player
	// possession). The own ship repairs while hidden and comes back at the host's position when the host dies or the
	// time runs out. Runs before Last Stand; on cooldown it does nothing.
	class LegendaryParasiteHullRuntime final
	{
	public:
		LegendaryParasiteHullRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryParasiteHullRuntime();
		LegendaryParasiteHullRuntime(const LegendaryParasiteHullRuntime&) = delete;
		LegendaryParasiteHullRuntime& operator=(const LegendaryParasiteHullRuntime&) = delete;
		// An empty rule list (uninstall) ends a running possession and returns the owner first.
		void Replace(const List<ResolvedModuleLegendaryParasiteHullRule>& rules);
		void Tick(float deltaSeconds);
		// Forgets all state without calling back into the owner (the owner may be shutting down).
		void Clear();
		const List<ResolvedModuleLegendaryParasiteHullRule>& GetRules() const { return mRules; }

		// The owner would die: take over a host. False when not ready or no host is in range.
		bool TryPossess(const DamageContext& context);
		bool IsPossessing() const { return mPossessing; }
		float GetCooldownRemaining() const { return mCooldownRemaining; }
		float GetPossessionSeconds() const { return mElapsed; }
		shared_ptr<EnemyActor> GetHost() const { return mHost.lock(); }

	private:
		void Return();

		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		PlayerSpaceShip* mOwner = nullptr;
		List<ResolvedModuleLegendaryParasiteHullRule> mRules;
		ResolvedModuleLegendaryParasiteHullRule mActiveRule;
		weak_ptr<EnemyActor> mHost;
		sf::Vector2f mLastHostLocation{};
		float mCooldownRemaining = 0.f;
		float mElapsed = 0.f;
		bool mPossessing = false;
	};
}
