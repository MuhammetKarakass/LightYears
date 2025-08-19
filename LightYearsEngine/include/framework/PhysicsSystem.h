#pragma once

#include <box2d/box2d.h>
#include "framework/Core.h"

namespace ly
{
	class Actor;

	class PhysicsSystem
	{
	public:
		static PhysicsSystem& Get();
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
		
		~PhysicsSystem();  // Public destructor for unique_ptr

	protected:
		PhysicsSystem();

	private:

		void ProcessPendingRemoveListeners();

		static unique_ptr<PhysicsSystem> physicsSystem;
		b2WorldId mPhysicsWorld;
		List<b2BodyId> mPendingRemoveListeners;
		float mPhysicsRate;
	};
}

