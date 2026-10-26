#pragma once

#include "framework/Actor.h"
#include "attributes/AttributeSystem.h"

#include <cstdint>
#include <optional>
#include <unordered_map>

namespace vega
{
	class EnemyActor;

	// Switches a hostile enemy to its taker's side for a limited time, then the enemy dies. While taken it is a friendly
	// combatant (FriendlySummon collision), so its own behavior targets enemies only, its shots hit enemies only, and the
	// taker's side never targets it. Its kill rewards are granted once when the takeover starts; its later death grants
	// nothing more. The takeover ends early when the taker is gone. Used by Neural Hijacker and Parasite Hull.
	struct CombatantTakeoverRequest
	{
		weak_ptr<Actor> taker;
		float seconds = 0.f;
		// When set, health becomes this fraction of MaxHealth when the takeover starts.
		std::optional<float> healthFractionOfMax;
		// MaxHealth and current health are scaled by this (host boosts).
		float healthMultiplier = 1.f;
		// Multiplier on all outgoing damage of the taken enemy.
		float damageMultiplier = 1.f;
	};

	class CombatantTakeoverActor;

	// True for a live, ready enemy of normal or elite class that is not already taken over. Bosses, minibosses and
	// non-enemy combatants (dummies, objects) are never taken.
	bool CanTakeOverCombatant(const Actor& target);
	bool IsCombatantTakenOver(const Actor& target);
	// Starts a takeover; returns the controller (empty when the target is not eligible).
	weak_ptr<CombatantTakeoverActor> BeginCombatantTakeover(EnemyActor& target, const CombatantTakeoverRequest& request);
	// Ends the target's takeover now; the taken enemy dies without rewards.
	void EndCombatantTakeover(Actor& target);

	// Controller actor of one takeover: counts its time down and ends it. Owns the stat changes it made on the target.
	class CombatantTakeoverActor final : public Actor
	{
	public:
		CombatantTakeoverActor(World* world, const weak_ptr<EnemyActor>& target, const CombatantTakeoverRequest& request);
		void Tick(float deltaTime) override;
		float GetRemainingSeconds() const { return mRemaining; }
		// Kills the taken enemy (no rewards) and destroys the controller.
		void Finish();

	private:
		friend weak_ptr<CombatantTakeoverActor> BeginCombatantTakeover(EnemyActor& target, const CombatantTakeoverRequest& request);
		void Start();

		weak_ptr<EnemyActor> mTarget;
		CombatantTakeoverRequest mRequest;
		float mRemaining = 0.f;
		bool mFinished = false;
	};

	// World-owned index of running takeovers, keyed by the taken enemy's unique id. Entries are weak and pruned.
	class CombatantTakeoverRegistry final
	{
	public:
		static CombatantTakeoverRegistry* Find(const World& world);
		static CombatantTakeoverRegistry& GetOrCreate(World& world);
		void Add(std::uint64_t targetId, const weak_ptr<CombatantTakeoverActor>& controller);
		shared_ptr<CombatantTakeoverActor> FindController(std::uint64_t targetId);
		void Remove(std::uint64_t targetId, const CombatantTakeoverActor* controller);

	private:
		std::unordered_map<std::uint64_t, weak_ptr<CombatantTakeoverActor>> mControllers;
	};
}
