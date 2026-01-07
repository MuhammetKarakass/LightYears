#include "framework/PhysicsSystem.h"
#include "framework/Actor.h"	
#include "framework/MathUtility.h"

namespace ly
{
	unique_ptr<PhysicsSystem> PhysicsSystem::physicsSystem{ nullptr };

	PhysicsSystem& PhysicsSystem::Get()
	{
		if (!physicsSystem)
		{
			physicsSystem = std::move(unique_ptr<PhysicsSystem>{new PhysicsSystem});
		}
		return *physicsSystem;
	}

	void PhysicsSystem::Step(float deltaTime)
	{
		if (mPhysicsWorld.index1 != 0)
		{
			ProcessPendingRemoveListeners();
			b2World_Step(mPhysicsWorld, deltaTime, 4);

			ProcessContactEvents();
		}
	}

	void PhysicsSystem::InitializeWorld(b2Vec2 gravity)
	{
		Cleanup();

		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = gravity;

		mPhysicsWorld = b2CreateWorld(&worldDef);

		b2World_EnableSleeping(mPhysicsWorld, false);
	}

	void PhysicsSystem::Cleanup()
	{
		if (mPhysicsWorld.index1 != 0)
		{
			b2DestroyWorld(mPhysicsWorld);
			mPhysicsWorld = b2WorldId{ 0, 0 };
		}
	}

	b2BodyId PhysicsSystem::AddListener(Actor* listener)
	{
		if (listener->GetIsPendingDestroy()) return b2BodyId{ 0,0,0 };

		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = b2_dynamicBody;

		bodyDef.userData = listener;

		sf::Vector2f actorLocation = listener->GetActorLocation();
		bodyDef.position = { actorLocation.x * GetPhysicsRate(), actorLocation.y * GetPhysicsRate() };
		bodyDef.rotation = b2MakeRot(DegreesToRadians(listener->GetActorRotation()));

		b2BodyId bodyId = b2CreateBody(mPhysicsWorld, &bodyDef);

		sf::FloatRect bounds = listener->GetActorGlobalBounds();
		float halfWidth = bounds.size.x / 2.0f * GetPhysicsRate();
		float halfHeight = bounds.size.y / 2.0f * GetPhysicsRate();

		b2Polygon box = b2MakeBox(halfWidth, halfHeight);

		b2ShapeDef shapeDef = b2DefaultShapeDef();

		shapeDef.density = 1.0f;
		shapeDef.material.friction = 0.3f;
		shapeDef.material.restitution = 0.6f;

		shapeDef.isSensor = false;

		shapeDef.enableContactEvents = true;
		shapeDef.enableSensorEvents = false;

		shapeDef.invokeContactCreation = true;

		b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &box);

		return bodyId;
	}

	void PhysicsSystem::ProcessContactEvents()
	{
		if (mPhysicsWorld.index1 == 0) return;

		b2ContactEvents contactEvents = b2World_GetContactEvents(mPhysicsWorld);

		for (int i = 0; i < contactEvents.beginCount; ++i)
		{
			b2ContactBeginTouchEvent beginEvent = contactEvents.beginEvents[i];

			Actor* actorA = nullptr;
			Actor* actorB = nullptr;

			if (b2Shape_IsValid(beginEvent.shapeIdA))
			{
				b2BodyId bodyIdA = b2Shape_GetBody(beginEvent.shapeIdA);
				if (b2Body_IsValid(bodyIdA))
				{
					void* userDataA = b2Body_GetUserData(bodyIdA);
					if (userDataA)
					{
						actorA = static_cast<Actor*>(userDataA);
					}
				}
			}

			if (b2Shape_IsValid(beginEvent.shapeIdB))
			{
				b2BodyId bodyIdB = b2Shape_GetBody(beginEvent.shapeIdB);
				if (b2Body_IsValid(bodyIdB))
				{
					void* userDataB = b2Body_GetUserData(bodyIdB);
					if (userDataB)
					{
						actorB = static_cast<Actor*>(userDataB);
					}
				}
			}

			bool aValidAtStart = actorA && !actorA->GetIsPendingDestroy();
			bool bValidAtStart = actorB && !actorB->GetIsPendingDestroy();

			if (aValidAtStart)
			{
				actorA->OnActorBeginOverlap(actorB);
			}

			if (bValidAtStart)
			{
				actorB->OnActorBeginOverlap(actorA);
			}
		}

		for (int i = 0; i < contactEvents.endCount; ++i)
		{
			b2ContactEndTouchEvent endEvent = contactEvents.endEvents[i];

			Actor* actorA = nullptr;
			Actor* actorB = nullptr;

			if (b2Shape_IsValid(endEvent.shapeIdA))
			{
				b2BodyId bodyIdA = b2Shape_GetBody(endEvent.shapeIdA);
				if (b2Body_IsValid(bodyIdA))
				{
					void* userDataA = b2Body_GetUserData(bodyIdA);
					if (userDataA)
					{
						actorA = static_cast<Actor*>(userDataA);
					}
				}
			}

			if (b2Shape_IsValid(endEvent.shapeIdB))
			{
				b2BodyId bodyIdB = b2Shape_GetBody(endEvent.shapeIdB);
				if (b2Body_IsValid(bodyIdB))
				{
					void* userDataB = b2Body_GetUserData(bodyIdB);
					if (userDataB)
					{
						actorB = static_cast<Actor*>(userDataB);
					}
				}
			}

			bool aValidAtStart = actorA && !actorA->GetIsPendingDestroy();
			bool bValidAtStart = actorB && !actorB->GetIsPendingDestroy();

			if (aValidAtStart)
			{
				actorA->OnActorEndOverlap(actorB);
			}

			if (bValidAtStart)
			{
				actorB->OnActorEndOverlap(actorA);
			}
		}
	}

	void PhysicsSystem::RemoveListener(b2BodyId bodyId)
	{
		mPendingRemoveListeners.push_back(bodyId);
	}

	bool PhysicsSystem::IsInitialized() const
	{
		return mPhysicsWorld.index1 != 0;
	}

	void PhysicsSystem::SetCollisionRadius(b2BodyId bodyId, float radius)
	{
		if (!b2Body_IsValid(bodyId))
		{
			return;
		}

		int shapeCount = b2Body_GetShapeCount(bodyId);

		if (shapeCount > 0)
		{
			b2ShapeId* shapes = new b2ShapeId[shapeCount];

			b2Body_GetShapes(bodyId, shapes, shapeCount);

			for (int i = 0; i < shapeCount; ++i)
			{
				if (b2Shape_IsValid(shapes[i]))
				{
					b2DestroyShape(shapes[i], true);
				}
			}

			delete[] shapes;
		}

		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.density = 1.0f;
		shapeDef.material.friction = 0.3f;
		shapeDef.material.restitution = 0.1f;

		shapeDef.enableContactEvents = true;
		shapeDef.invokeContactCreation = true;
		shapeDef.isSensor = false;

		b2Circle circle;
		circle.center = { 0.0f, 0.0f };
		circle.radius = radius * mPhysicsRate;

		b2CreateCircleShape(bodyId, &shapeDef, &circle);
	}

	PhysicsSystem::PhysicsSystem() :
		mPhysicsWorld{ 0, 0 },
		mPhysicsRate{ 0.01f },
		mPendingRemoveListeners{}
	{
		InitializeWorld({ 0.0f, 0.0f });
	}

	void PhysicsSystem::ProcessPendingRemoveListeners()
	{
		for (auto bodyId : mPendingRemoveListeners)
		{
			if (b2Body_IsValid(bodyId))
			{
				b2DestroyBody(bodyId);
			}
		}
		mPendingRemoveListeners.clear();
	}

	PhysicsSystem::~PhysicsSystem()
	{
		Cleanup();
	}
}