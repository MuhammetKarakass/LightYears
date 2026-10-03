#pragma once
#include "framework/ActorComponent.h"
#include "engineConfigs/EngineStructs.h"
#include <box2d/box2d.h>
#include <cstdint>
#include <optional>

namespace ly
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
	public:
		explicit CollisionComponent(Actor& owner);

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

	private:
		void RefreshFilterAndSpatialQuery();

		bool mPhysicsEnabled = false;
		std::optional<b2BodyId> mPhysicsBodyId;
		PhysicsBodyType mPhysicsBodyType = PhysicsBodyType::Dynamic;
		CollisionLayer mCollisionLayer = CollisionLayer::None;
		CollisionLayer mCollisionMask = CollisionLayer::None;
	};
}
