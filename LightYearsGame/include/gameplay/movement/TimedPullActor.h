#pragma once

#include "framework/Actor.h"
#include "gameplay/movement/MovementInfluenceTypes.h"

#include <SFML/System/Vector2.hpp>

namespace vega
{
	// Drags the source's opposing combatants found within radius of a center toward it at a fixed speed for a fixed time.
	// The center follows a live anchor actor (it stays at the last anchor position once the anchor is gone). Each target's
	// pull time goes through its control response (Displaced), so immune/interrupt-only targets are skipped and reduced ones
	// are released early. Movement uses forced movement keyed by this actor's unique id and is always removed on end.
	class TimedPullActor final : public Actor
	{
	public:
		TimedPullActor(World* world, const weak_ptr<Actor>& source, const weak_ptr<Actor>& anchor, float radius, float speed, float seconds);
		~TimedPullActor() override;
		void Tick(float deltaTime) override;
		void EndPlay() override;

		// Spawns the pull at the anchor's location; the anchor itself is never pulled. Returns the actor (empty when nothing spawned).
		static weak_ptr<TimedPullActor> Spawn(Actor& source, Actor& anchor, float radius, float speed, float seconds);

	private:
		struct Target
		{
			weak_ptr<Actor> actor;
			float remaining = 0.f;
		};
		void CollectTargets();
		// Points every active target at the center; false when none is left.
		bool Steer(float deltaTime);
		void Release(Actor& target);
		void ReleaseAll();

		weak_ptr<Actor> mSource;
		weak_ptr<Actor> mAnchor;
		sf::Vector2f mCenter{};
		float mRadius = 0.f;
		float mSpeed = 0.f;
		float mSeconds = 0.f;
		bool mCollected = false;
		List<Target> mTargets;
	};
}
