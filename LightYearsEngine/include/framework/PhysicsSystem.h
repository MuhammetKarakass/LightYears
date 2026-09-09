#pragma once

#include <box2d/box2d.h>
#include "framework/Core.h"
#include <SFML/Graphics/Rect.hpp>

namespace ly
{
	class Actor;

	class PhysicsSystem
	{
	public:
		using ActorBoundsVisitor = bool(*)(void* context, Actor* actor);

		static PhysicsSystem& Get();
		static void ShutdownPhysicsSystem();
		void Step(float deltaTime);
		void InitializeWorld(b2Vec2 gravity = {0.0f, 0.0f});
		void Cleanup();
		
		// Contact event handling (Box2D v3.x event-based system)
		void ProcessContactEvents();
		
		b2WorldId GetWorld() const { return mPhysicsWorld; }
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
		
		~PhysicsSystem();  // Public destructor for unique_ptr

	protected:
		PhysicsSystem();

	private:

		void ProcessPendingRemoveListeners();

		static unique_ptr<PhysicsSystem> physicsSystem;
		b2WorldId mPhysicsWorld;
		List<b2BodyId> mPendingRemoveListeners;
		List<b2ShapeId> mShapeScratchBuffer;
		float mPhysicsRate;
	};
}

