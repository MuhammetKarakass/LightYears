#include "framework/PhysicsSystem.h"
#include "framework/Actor.h"	
#include "framework/MathUtility.h"

namespace ly
{
	namespace
	{
		b2Filter MakeCollisionFilter(const Actor& actor)
		{
			b2Filter filter = b2DefaultFilter();
			filter.categoryBits = static_cast<uint8_t>(actor.GetCollisionLayer());
			filter.maskBits = static_cast<uint8_t>(actor.GetCollisionMask());
			return filter;
		}

		Actor* ResolveActor(b2ShapeId shapeId)
		{
			if (!b2Shape_IsValid(shapeId))
			{
				return nullptr;
			}
			const b2BodyId bodyId = b2Shape_GetBody(shapeId);
			return b2Body_IsValid(bodyId)
				? static_cast<Actor*>(b2Body_GetUserData(bodyId))
				: nullptr;
		}

		void DispatchContactEvent(
			b2ShapeId shapeIdA,
			b2ShapeId shapeIdB,
			bool isBeginEvent
		)
		{
			Actor* actorA = ResolveActor(shapeIdA);
			Actor* actorB = ResolveActor(shapeIdB);
			if (!actorA || !actorB || actorA->GetIsPendingDestroy() ||
				actorB->GetIsPendingDestroy() || !actorA->CanCollideWith(actorB) ||
				!actorB->CanCollideWith(actorA))
			{
				return;
			}

			if (isBeginEvent)
			{
				actorA->OnActorBeginOverlap(actorB);
				if (!actorB->GetIsPendingDestroy())
				{
					actorB->OnActorBeginOverlap(actorA);
				}
			}
			else
			{
				actorA->OnActorEndOverlap(actorB);
				if (!actorB->GetIsPendingDestroy())
				{
					actorB->OnActorEndOverlap(actorA);
				}
			}
		}
	}

	unique_ptr<PhysicsSystem> PhysicsSystem::physicsSystem{ nullptr };

	PhysicsSystem& PhysicsSystem::Get()
	{
		if (!physicsSystem)
		{
			physicsSystem = std::move(unique_ptr<PhysicsSystem>{new PhysicsSystem});
		}
		return *physicsSystem;
	}

	void PhysicsSystem::ShutdownPhysicsSystem()
	{
		if (physicsSystem)
		{
			physicsSystem->Cleanup();
			physicsSystem.reset();
		}
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
		mPendingRemoveListeners.clear();

		if (mPhysicsWorld.index1 != 0)
		{
			b2DestroyWorld(mPhysicsWorld);
			mPhysicsWorld = b2WorldId{ 0, 0 };
		}
	}

	b2BodyId PhysicsSystem::AddListener(Actor* listener)
	{
		if (listener->GetIsPendingDestroy()) return b2BodyId{ 0,0,0 };

		// Most actors use their visual bounds as a polygon. Gameplay projectiles
		// are often rendered procedurally and have no sprite, so allow them to
		// provide an explicit collision radius instead of silently losing physics.
		sf::FloatRect bounds = listener->GetActorGlobalBounds();
		const float collisionRadius = std::max(0.f, listener->GetPhysicsCollisionRadius());
		const std::size_t explicitBoxCount = listener->GetPhysicsCollisionBoxCount();
		bool hasExplicitBox = false;
		for (std::size_t index = 0; index < explicitBoxCount; ++index)
		{
			const PhysicsCollisionBox box = listener->GetPhysicsCollisionBox(index);
			if (box.halfExtents.x > 0.f && box.halfExtents.y > 0.f)
			{
				hasExplicitBox = true;
				break;
			}
		}
		const bool hasValidBounds = bounds.size.x > 0.0f && bounds.size.y > 0.0f;
		if (!hasValidBounds && collisionRadius <= 0.f && !hasExplicitBox)
		{
			return b2BodyId{ 0,0,0 };
		}

		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = listener->GetPhysicsBodyType() == PhysicsBodyType::Static
			? b2_staticBody
			: b2_dynamicBody;

		bodyDef.userData = listener;

		sf::Vector2f actorLocation = listener->GetActorLocation();
		bodyDef.position = { actorLocation.x * GetPhysicsRate(), actorLocation.y * GetPhysicsRate() };
		bodyDef.rotation = b2MakeRot(DegreesToRadians(listener->GetActorRotation()));

		b2BodyId bodyId = b2CreateBody(mPhysicsWorld, &bodyDef);

		b2ShapeDef shapeDef = b2DefaultShapeDef();

		shapeDef.density = 1.0f;
		shapeDef.material.friction = 0.3f;
		shapeDef.material.restitution = 0.6f;

		shapeDef.isSensor = false;

		shapeDef.enableContactEvents = true;
		shapeDef.enableSensorEvents = false;

		shapeDef.invokeContactCreation = true;
		// Keep Box2D's broadphase aligned with the gameplay collision contract.
		// Without this, every physics shape can form contacts with every other
		// shape and gets rejected only after the expensive contact callback path.
		shapeDef.filter = MakeCollisionFilter(*listener);

		if (hasExplicitBox)
		{
			for (std::size_t index = 0; index < explicitBoxCount; ++index)
			{
				const PhysicsCollisionBox box = listener->GetPhysicsCollisionBox(index);
				if (box.halfExtents.x <= 0.f || box.halfExtents.y <= 0.f)
				{
					continue;
				}
				const b2Polygon shape = b2MakeOffsetBox(
					box.halfExtents.x * GetPhysicsRate(),
					box.halfExtents.y * GetPhysicsRate(),
					{
						box.localCenter.x * GetPhysicsRate(),
						box.localCenter.y * GetPhysicsRate()
					},
					b2MakeRot(DegreesToRadians(box.localRotationDegrees))
				);
				b2CreatePolygonShape(bodyId, &shapeDef, &shape);
			}
		}
		else if (collisionRadius > 0.f)
		{
			b2Circle circle;
			circle.center = { 0.f, 0.f };
			circle.radius = collisionRadius * GetPhysicsRate();
			b2CreateCircleShape(bodyId, &shapeDef, &circle);
		}
		else
		{
			const float halfWidth = bounds.size.x / 2.0f * GetPhysicsRate();
			const float halfHeight = bounds.size.y / 2.0f * GetPhysicsRate();
			b2Polygon box = b2MakeBox(halfWidth, halfHeight);
			b2CreatePolygonShape(bodyId, &shapeDef, &box);
		}

		return bodyId;
	}

	void PhysicsSystem::ProcessContactEvents()
	{
		if (mPhysicsWorld.index1 == 0) return;

		b2ContactEvents contactEvents = b2World_GetContactEvents(mPhysicsWorld);

		for (int i = 0; i < contactEvents.beginCount; ++i)
		{
			const b2ContactBeginTouchEvent& beginEvent = contactEvents.beginEvents[i];
			DispatchContactEvent(beginEvent.shapeIdA, beginEvent.shapeIdB, true);
		}

		for (int i = 0; i < contactEvents.endCount; ++i)
		{
			const b2ContactEndTouchEvent& endEvent = contactEvents.endEvents[i];
			DispatchContactEvent(endEvent.shapeIdA, endEvent.shapeIdB, false);
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
			mShapeScratchBuffer.resize(static_cast<std::size_t>(shapeCount));

			b2Body_GetShapes(bodyId, mShapeScratchBuffer.data(), shapeCount);

			for (int i = 0; i < shapeCount; ++i)
			{
				if (b2Shape_IsValid(mShapeScratchBuffer[static_cast<std::size_t>(i)]))
				{
					b2DestroyShape(mShapeScratchBuffer[static_cast<std::size_t>(i)], true);
				}
			}
		}

		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.density = 1.0f;
		shapeDef.material.friction = 0.3f;
		shapeDef.material.restitution = 0.1f;

		shapeDef.enableContactEvents = true;
		shapeDef.invokeContactCreation = true;
		shapeDef.isSensor = false;
		if (Actor* actor = static_cast<Actor*>(b2Body_GetUserData(bodyId)))
		{
			shapeDef.filter = MakeCollisionFilter(*actor);
		}

		b2Circle circle;
		circle.center = { 0.0f, 0.0f };
		circle.radius = radius * mPhysicsRate;

		b2CreateCircleShape(bodyId, &shapeDef, &circle);
	}

	void PhysicsSystem::RefreshCollisionFilter(b2BodyId bodyId)
	{
		if (!b2Body_IsValid(bodyId))
		{
			return;
		}

		Actor* actor = static_cast<Actor*>(b2Body_GetUserData(bodyId));
		if (!actor)
		{
			return;
		}

		const int shapeCount = b2Body_GetShapeCount(bodyId);
		if (shapeCount <= 0)
		{
			return;
		}

		mShapeScratchBuffer.resize(static_cast<std::size_t>(shapeCount));
		b2Body_GetShapes(bodyId, mShapeScratchBuffer.data(), shapeCount);
		const b2Filter filter = MakeCollisionFilter(*actor);
		for (int i = 0; i < shapeCount; ++i)
		{
			const b2ShapeId shapeId = mShapeScratchBuffer[static_cast<std::size_t>(i)];
			if (b2Shape_IsValid(shapeId))
			{
				b2Shape_SetFilter(shapeId, filter);
			}
		}
	}


	void PhysicsSystem::VisitActorsInBounds(
		const sf::FloatRect& bounds,
		void* context,
		ActorBoundsVisitor visitor
	) const
	{
		if (mPhysicsWorld.index1 == 0 ||
			bounds.size.x < 0.f || bounds.size.y < 0.f || !visitor)
		{
			return;
		}

		struct QueryContext
		{
			void* visitorContext = nullptr;
			ActorBoundsVisitor visitor = nullptr;
		};
		QueryContext queryContext{ context, visitor };
		const auto callback = [](b2ShapeId shapeId, void* rawContext)
		{
			auto* query = static_cast<QueryContext*>(rawContext);
			if (!query || !b2Shape_IsValid(shapeId))
			{
				return true;
			}
			Actor* actor = ResolveActor(shapeId);
			if (actor && !actor->GetIsPendingDestroy())
			{
				return query->visitor(query->visitorContext, actor);
			}
			return true;
		};

		const float physicsRate = GetPhysicsRate();
		const b2AABB queryBounds{
			{ bounds.position.x * physicsRate, bounds.position.y * physicsRate },
			{
				(bounds.position.x + bounds.size.x) * physicsRate,
				(bounds.position.y + bounds.size.y) * physicsRate
			}
		};
		// Gameplay spatial queries are discovery operations, not collision tests.
		// Use all bits in both directions so an actor whose collision mask only
		// accepts enemies is still discoverable by area effects such as Gravity
		// Anomaly. Collision filtering remains active for actual Box2D contacts.
		b2QueryFilter queryFilter = b2DefaultQueryFilter();
		queryFilter.categoryBits = ~uint64_t{ 0 };
		queryFilter.maskBits = ~uint64_t{ 0 };
		b2World_OverlapAABB(
			mPhysicsWorld,
			queryBounds,
			queryFilter,
			callback,
			&queryContext
		);
	}

	List<Actor*> PhysicsSystem::QueryActorsInBounds(
		const sf::FloatRect& bounds
	) const
	{
		List<Actor*> actors;
		VisitActorsInBounds(
			bounds,
			&actors,
			[](void* context, Actor* actor)
			{
				static_cast<List<Actor*>*>(context)->push_back(actor);
				return true;
			}
		);
		return actors;
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
