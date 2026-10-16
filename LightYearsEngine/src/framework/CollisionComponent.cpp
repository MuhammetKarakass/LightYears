#include "framework/CollisionComponent.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "framework/MathUtility.h"
#include "framework/reflection/Reflection.h"
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

	VEGA_REFLECT(CollisionComponent, ActorComponent)
	{
		VEGA_ADDABLE();
		VEGA_CATEGORY("Physics");
		VEGA_PROPERTY("Physics Enabled", IsPhysicsEnabled, SetEnablePhysics);
		VEGA_READONLY("Has Physics Body", HasPhysicsBody);
		VEGA_PROPERTY("Body Type", GetPhysicsBodyType, SetPhysicsBodyType)
			.Labels({ { 0, "Dynamic" }, { 1, "Static" } });

		reflect::EnumLabels collisionLabels;
		const char* bits[]{ "Player", "PlayerBullet", "Enemy", "EnemyBullet", "Powerup", "Environment", "RelayProjectile", "FriendlySummon" };
		for (int bit = 0; bit < 8; ++bit) collisionLabels.emplace_back(1 << bit, bits[bit]);

		VEGA_CATEGORY("Collision");
		VEGA_PROPERTY("Collision Layer", GetCollisionLayer, SetCollisionLayer).Flags(collisionLabels);
		VEGA_PROPERTY("Collision Mask", GetCollisionMask, SetCollisionMask).Flags(collisionLabels);
	}
}
