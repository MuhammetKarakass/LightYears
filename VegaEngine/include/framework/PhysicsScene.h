#pragma once

#include <box2d/box2d.h>
#include "framework/Core.h"
#include <SFML/Graphics/Rect.hpp>

namespace vega
{
	class Actor;

	// Owns one Box2D world. Each vega::World owns exactly one PhysicsScene (mirrors
	// Unreal's UWorld/FPhysScene), so a scene's lifetime is the world's lifetime
	// and bodies from different worlds can never interact.
	class PhysicsScene
	{
	public:
		using ActorBoundsVisitor = bool(*)(void* context, Actor* actor);

		explicit PhysicsScene(b2Vec2 gravity = { 0.0f, 0.0f });
		~PhysicsScene();
		PhysicsScene(const PhysicsScene&) = delete;
		PhysicsScene& operator=(const PhysicsScene&) = delete;

		void Step(float deltaTime);

		// Contact event handling (Box2D v3.x event-based system)
		void ProcessContactEvents();

		b2WorldId GetWorldId() const { return mPhysicsWorld; }
		float GetPhysicsRate() const { return mPhysicsRate; }
		b2BodyId AddListener(Actor* listener);
		void RemoveListener(b2BodyId bodyId);

		bool IsInitialized() const;

		void SetCollisionRadius(b2BodyId bodyId, float radius);
		// Mirrors an Actor's gameplay collision layer/mask to every Box2D shape
		// on an existing body. Actors call this when their runtime collision
		// policy changes (for example during portal transit or reflection).
		void RefreshCollisionFilter(b2BodyId bodyId);
		// Visits the single physics shape owned by each matching actor without
		// constructing a temporary result container. The visitor returns false to
		// stop the Box2D query early.
		void VisitActorsInBounds(
			const sf::FloatRect& bounds,
			void* context,
			ActorBoundsVisitor visitor
		) const;
		List<Actor*> QueryActorsInBounds(const sf::FloatRect& bounds) const;

	private:
		void ProcessPendingRemoveListeners();

		b2WorldId mPhysicsWorld;
		List<b2BodyId> mPendingRemoveListeners;
		List<b2ShapeId> mShapeScratchBuffer;
		float mPhysicsRate;
	};
}
