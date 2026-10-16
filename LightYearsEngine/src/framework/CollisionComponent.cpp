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
	CollisionComponent::~CollisionComponent() { ClearContacts(); }
	void CollisionComponent::EndPlay() { ClearContacts(true); }
	void CollisionComponent::ClearContacts(bool notify)
	{
		// Move first: overlap callbacks can change filters, disable bodies or destroy actors.
		auto contacts = std::move(mContacts);
		mContacts.clear();
		for (const auto& entry : contacts)
		{
			if (auto actor = entry.second.actor.lock())
			{
				actor->onActorDestroyed.UnbindAction(entry.second.destroyed);
				if (!notify) continue;
				if (auto* collision = actor->FindComponent<CollisionComponent>()) collision->OtherDestroyed(&GetOwner());
				GetOwner().ForEachComponent([&](ActorComponent& component)
				{
					if (component.HasBegunPlay() && component.IsActive() && !GetOwner().GetIsPendingDestroy()) component.OnOverlapEnd(actor.get());
				});
			}
		}
	}
	void CollisionComponent::ClearInvalidContacts()
	{
		std::vector<weak_ptr<Actor>> invalid;
		for (const auto& entry : mContacts)
			if (auto actor = entry.second.actor.lock(); actor &&
				(!GetOwner().CanCollideWith(actor.get()) || !actor->CanCollideWith(&GetOwner()))) invalid.push_back(actor);
		for (const auto& weak : invalid)
		{
			if (auto actor = weak.lock())
			{
				OtherDestroyed(actor.get());
				if (auto* collision = actor->FindComponent<CollisionComponent>()) collision->OtherDestroyed(&GetOwner());
			}
		}
	}
	void CollisionComponent::OtherDestroyed(Actor* other)
	{
		const auto found = mContacts.find(other->GetUniqueID());
		if (found == mContacts.end()) return;
		found->second.count = 1;
		NotifyContact(other, false);
	}

	void CollisionComponent::NotifyContact(Actor* other, bool begin)
	{
		if (!other || GetOwner().GetIsPendingDestroy()) return;
		const auto id = other->GetUniqueID();
		if (begin)
		{
			// A compound collider can produce several shape contacts for one actor.
			auto& contact = mContacts[id];
			if (++contact.count != 1) return;
			contact.actor = other->GetWeakActor();
			contact.destroyed = other->onActorDestroyed.BindAction(this, &CollisionComponent::OtherDestroyed);
			GetOwner().ForEachComponent([&](ActorComponent& component)
			{
				if (component.HasBegunPlay() && component.IsActive() && !GetOwner().GetIsPendingDestroy()) component.OnOverlapBegin(other);
			});
			// This engine uses contacts without solver impulses. A hit means the
			// first physical touch, rather than an impulse-threshold notification.
			GetOwner().ForEachComponent([&](ActorComponent& component)
			{
				if (component.HasBegunPlay() && component.IsActive() && !GetOwner().GetIsPendingDestroy()) component.OnHit(other);
			});
		}
		else
		{
			const auto found = mContacts.find(id);
			if (found == mContacts.end() || --found->second.count != 0) return;
			other->onActorDestroyed.UnbindAction(found->second.destroyed);
			mContacts.erase(found);
			GetOwner().ForEachComponent([&](ActorComponent& component)
			{
				if (component.HasBegunPlay() && component.IsActive() && !GetOwner().GetIsPendingDestroy()) component.OnOverlapEnd(other);
			});
		}
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
		if (mCollisionLayer == layer) return;
		mCollisionLayer = layer;
		ClearInvalidContacts();
		RefreshFilterAndSpatialQuery();
	}

	void CollisionComponent::SetCollisionMask(CollisionLayer mask)
	{
		if (mCollisionMask == mask) return;
		mCollisionMask = mask;
		ClearInvalidContacts();
		RefreshFilterAndSpatialQuery();
	}

	bool CollisionComponent::CanCollideWith(const Actor* other) const
	{
		if (other == nullptr) return false;
		return HasCollisionLayer(mCollisionMask, other->GetCollisionLayer());
	}

	void CollisionComponent::SetEnablePhysics(bool enable)
	{
		if (enable && GetOwner().GetParent() && mPhysicsBodyType == PhysicsBodyType::Dynamic) GetOwner().Detach();
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
		ClearContacts(true);
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
		if (bodyType == PhysicsBodyType::Dynamic && mPhysicsEnabled && GetOwner().GetParent()) GetOwner().Detach();
		mPhysicsBodyType = bodyType;
		if (mPhysicsBodyId)
		{
			b2Body_SetType(
				*mPhysicsBodyId,
				GetOwner().GetParent() ? b2_kinematicBody : bodyType == PhysicsBodyType::Static ? b2_staticBody : b2_dynamicBody
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
