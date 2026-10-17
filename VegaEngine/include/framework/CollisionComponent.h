#pragma once
#include "framework/ActorComponent.h"
#include "engineConfigs/EngineStructs.h"
#include <box2d/box2d.h>
#include <cstdint>
#include <optional>
#include "framework/Delegate.h"
#include <unordered_map>

namespace vega
{
	// Physics bodies are opt-in by shape and mobility. The default preserves the
	// existing moving-actor behavior; static boxes are for reusable world
	// geometry such as barriers, doors, and future map obstacles.
	enum class PhysicsBodyType : uint8_t
	{
		Dynamic,
		Static
	};

	// Collision state of an actor (UE UPrimitiveComponent-lite): layer/mask, the
	// physics enable flag, body type and the Box2D body id. The shape providers
	// (GetPhysicsCollisionRadius/Box*) deliberately stay virtual on Actor because
	// they describe the actor's geometry and are overridden by many actor
	// subclasses; PhysicsScene::AddListener reads them through the owner. The
	// component is created lazily by the first Actor::Set* collision call, so
	// actors that never touch collision allocate nothing and report the defaults
	// (layer/mask None, physics disabled, dynamic body type). Layer and mask
	// changes refresh the Box2D filter first and the World spatial index second.
	class CollisionComponent : public ActorComponent
	{
		VEGA_CLASS(CollisionComponent)
	public:
		explicit CollisionComponent(Actor& owner);
		~CollisionComponent() override;
		void EndPlay() override;

		CollisionLayer GetCollisionLayer() const { return mCollisionLayer; }
		CollisionLayer GetCollisionMask() const { return mCollisionMask; }
		void SetCollisionLayer(CollisionLayer layer);
		void SetCollisionMask(CollisionLayer mask);
		bool CanCollideWith(const Actor* other) const;

		void SetEnablePhysics(bool enable);
		bool IsPhysicsEnabled() const { return mPhysicsEnabled; }
		bool HasPhysicsBody() const { return mPhysicsBodyId.has_value(); }
		void InitializePhysics();
		void UnInitializePhysics();
		void UpdatePhysicsTransform();
		void SetPhysicsBodyType(PhysicsBodyType bodyType);
		PhysicsBodyType GetPhysicsBodyType() const { return mPhysicsBodyType; }
		void SetCollisionRadius(float radius);
		// Contact dispatch is independent of Actor overrides, so components also
		// receive events when a gameplay override does not call its base method.
		void NotifyContact(Actor* other, bool begin);

	private:
		void RefreshFilterAndSpatialQuery();
		void OtherDestroyed(Actor* other);
		// Teardown unbinds silently; filter/body changes end both sides explicitly.
		void ClearContacts(bool notify = false);
		void ClearInvalidContacts();

		bool mPhysicsEnabled = false;
		std::optional<b2BodyId> mPhysicsBodyId;
		PhysicsBodyType mPhysicsBodyType = PhysicsBodyType::Dynamic;
		CollisionLayer mCollisionLayer = CollisionLayer::None;
		CollisionLayer mCollisionMask = CollisionLayer::None;
		struct Contact
		{
			unsigned int count = 0;
			weak_ptr<Actor> actor;
			DelegateHandle destroyed;
		};
		std::unordered_map<unsigned int, Contact> mContacts;
	};
}
