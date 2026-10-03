#include "framework/CollisionComponent.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "framework/MathUtility.h"
#include <framework/PhysicsScene.h>

namespace ly
{
	CollisionComponent::CollisionComponent(Actor& owner) : ActorComponent{ owner }
	{
	}

	void CollisionComponent::RefreshFilterAndSpatialQuery()
	{
		World* world = GetWorld();
		if (mPhysicsBodyId && world)
		{
			world->GetPhysicsScene().RefreshCollisionFilter(*mPhysicsBodyId);
		}
		if (world)
		{
			world->RefreshActorSpatialQuery(GetOwner());
		}
	}

	void CollisionComponent::SetCollisionLayer(CollisionLayer layer)
	{
		mCollisionLayer = layer;
		RefreshFilterAndSpatialQuery();
	}

	void CollisionComponent::SetCollisionMask(CollisionLayer mask)
	{
		mCollisionMask = mask;
		RefreshFilterAndSpatialQuery();
	}

	bool CollisionComponent::CanCollideWith(const Actor* other) const
	{
		if (other == nullptr) return false;
		return HasCollisionLayer(mCollisionMask, other->GetCollisionLayer());
	}

	void CollisionComponent::SetEnablePhysics(bool enable)
	{
		mPhysicsEnabled = enable;

		if (mPhysicsEnabled)
		{
			InitializePhysics();
		}
		else
		{
			UnInitializePhysics();
		}

		if (World* world = GetWorld())
		{
			world->RefreshActorSpatialQuery(GetOwner());
		}
	}

	void CollisionComponent::InitializePhysics()
	{
		World* world = GetWorld();
		if (!mPhysicsBodyId && world)
		{
			const b2BodyId bodyId = world->GetPhysicsScene().AddListener(&GetOwner());
			// AddListener returns the zero-initialized id when an actor has no
			// physical shape. Do not pass that sentinel back into Box2D: its
			// debug validation intentionally asserts on an invalid world id.
			if (bodyId.index1 != 0)
			{
				mPhysicsBodyId = bodyId;
			}
		}
	}

	void CollisionComponent::UnInitializePhysics()
	{
		if (mPhysicsBodyId)
		{
			if (World* world = GetWorld())
			{
				world->GetPhysicsScene().RemoveListener(*mPhysicsBodyId);
			}
			mPhysicsBodyId.reset();
		}
	}

	void CollisionComponent::UpdatePhysicsTransform()
	{
		World* world = GetWorld();
		if (mPhysicsBodyId && world)
		{
			float physicsRate = world->GetPhysicsScene().GetPhysicsRate();
			sf::Vector2f actorLocation = GetOwner().GetActorLocation();
			float actorRotation = GetOwner().GetActorRotation();

			b2Vec2 position{ actorLocation.x * physicsRate, actorLocation.y * physicsRate };
			b2Rot rotation = b2MakeRot(DegreesToRadians(actorRotation));

			b2Body_SetTransform(*mPhysicsBodyId, position, rotation);
		}
	}

	void CollisionComponent::SetPhysicsBodyType(PhysicsBodyType bodyType)
	{
		mPhysicsBodyType = bodyType;
		if (mPhysicsBodyId)
		{
			b2Body_SetType(
				*mPhysicsBodyId,
				bodyType == PhysicsBodyType::Static ? b2_staticBody : b2_dynamicBody
			);
		}
	}

	void CollisionComponent::SetCollisionRadius(float radius)
	{
		World* world = GetWorld();
		if (!mPhysicsBodyId || !world) return;
		world->GetPhysicsScene().SetCollisionRadius(*mPhysicsBodyId, radius);
	}
}
