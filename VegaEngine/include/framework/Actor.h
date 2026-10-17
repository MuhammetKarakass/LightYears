#pragma once
#include "framework/Object.h"
#include "framework/ActorComponent.h"
#include "framework/CollisionComponent.h"
#include "framework/Core.h"
#include "framework/SimulationTime.h"
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h> 
#include <cstddef>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>
#include "engineConfigs/EngineStructs.h"

	namespace vega
{
	enum class RenderLayer : uint8_t
	{
		Background,
		GroundDecal,
		World,
		Projectile,
		WorldVfx,
		Foreground,
		Count
	};

	class EngineContext;
	class World;
	class LightComponent;
	class SpriteComponent;
	// A body-local physical box. Most actors still expose one centered box via
	// GetPhysicsCollisionBoxHalfExtents(); actors with non-rectangular outlines
	// can opt into several boxes without owning Box2D fixtures themselves.
	struct PhysicsCollisionBox
	{
		sf::Vector2f halfExtents{};
		sf::Vector2f localCenter{};
		float localRotationDegrees = 0.f;
	};

	class Actor : public Object
	{
		VEGA_CLASS(Actor)
	public:
        Actor(World* OwningWorld, const std::string& TexturePath = "");
		virtual ~Actor();

		void BeginPlayInternal();
		virtual void BeginPlay();
		void TickInternal(float deltaTime);
		virtual void Tick(float deltaTime);

		virtual void Render(sf::RenderTarget& window);

		World* GetWorld() const { return mOwningWorld; }
		EngineContext& GetEngineContext() const { return *mContext; }
		weak_ptr<Actor> GetWeakActor();
		weak_ptr<const Actor> GetWeakActor() const;
		void SetSimulationTimeDomain(SimulationTimeDomain domain)
		{
			mSimulationTimeDomain = domain;
		}
		SimulationTimeDomain GetSimulationTimeDomain() const
		{
			return mSimulationTimeDomain;
		}
		bool IsActorOutOfWindow(float allowance=10.f) const;

		//PHYSICS
		// Collision state lives in CollisionComponent (created on the first setter
		// call). The accessors below are thin forwarders kept because they are used
		// by hundreds of call sites; an actor without the component reports the
		// defaults (layer/mask None, physics disabled, dynamic body, no body).
		void SetEnablePhysics(bool enable);
		bool IsPhysicsEnabled() const { return mCollision && mCollision->IsPhysicsEnabled(); }
		bool HasPhysicsBody() const { return mCollision && mCollision->HasPhysicsBody(); }
		void UnInitializePhysics();
		// The shape providers below describe the actor's geometry and are overridden
		// by many actor subclasses, so they stay virtual on Actor; the physics scene
		// and the World spatial index query them through the actor.
		// Actors without a sprite can still participate in physics when they
		// provide an explicit gameplay collision radius. The default remains zero
		// so ordinary visual-less actors do not receive an accidental body.
		virtual float GetPhysicsCollisionRadius() const { return 0.f; }
		// A non-zero value selects a rotated box over sprite-derived bounds. The
		// actor transform supplies the box orientation, so no feature needs to
		// create its own physics fixture implementation.
		virtual sf::Vector2f GetPhysicsCollisionBoxHalfExtents() const { return {}; }
		// Backward-compatible multi-box extension. Existing actors that override
		// only GetPhysicsCollisionBoxHalfExtents() continue to create one centered
		// box. Complex physical outlines override these two methods instead.
		virtual std::size_t GetPhysicsCollisionBoxCount() const
		{
			const sf::Vector2f halfExtents = GetPhysicsCollisionBoxHalfExtents();
			return halfExtents.x > 0.f && halfExtents.y > 0.f ? 1u : 0u;
		}
		virtual PhysicsCollisionBox GetPhysicsCollisionBox(std::size_t index) const
		{
			return index == 0u
				? PhysicsCollisionBox{ GetPhysicsCollisionBoxHalfExtents(), {}, 0.f }
				: PhysicsCollisionBox{};
		}
		void SetPhysicsBodyType(PhysicsBodyType bodyType);
		PhysicsBodyType GetPhysicsBodyType() const { return mCollision ? mCollision->GetPhysicsBodyType() : PhysicsBodyType::Dynamic; }
		bool CanCollideWith(const Actor* other) const;
		virtual	void OnActorEndOverlap(Actor* otherActor);
		virtual void OnActorBeginOverlap(Actor* otherActor);
		virtual void Destroy() override;
		CollisionLayer GetCollisionLayer() const { return mCollision ? mCollision->GetCollisionLayer() : CollisionLayer::None; }
		CollisionLayer GetCollisionMask() const { return mCollision ? mCollision->GetCollisionMask() : CollisionLayer::None; }
		void SetCollisionLayer(CollisionLayer layer);
		void SetCollisionMask(CollisionLayer mask);


		// COMPONENTS (UE-style). Component BeginPlay runs in BeginPlayInternal before
		// Actor::BeginPlay; ticking components run from the base Actor::Tick (so derived
		// Tick overrides must keep calling Actor::Tick first, as they already do for
		// lights); EndPlay runs from Destroy. Adding a component to an actor that has
		// already begun play begins it immediately.
		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			static_assert(std::is_base_of<ActorComponent, T>::value, "AddComponent requires an ActorComponent type");
			std::unique_ptr<T> owned = std::make_unique<T>(*this, std::forward<Args>(args)...);
			T& component = *owned;
			mComponents.push_back(std::move(owned));
			CacheComponent(component);
			if (mBeganPlay) component.BeginPlayInternal();
			return component;
		}
		template<typename T>
		T* FindComponent()
		{
			for (const auto& component : mComponents)
			{
				if (T* typed = dynamic_cast<T*>(component.get())) return typed;
			}
			return nullptr;
		}
		template<typename T>
		const T* FindComponent() const
		{
			for (const auto& component : mComponents)
			{
				if (const T* typed = dynamic_cast<const T*>(component.get())) return typed;
			}
			return nullptr;
		}
		template<typename T, typename... Args>
		T& GetOrAddComponent(Args&&... args)
		{
			if (T* existing = FindComponent<T>()) return *existing;
			return AddComponent<T>(std::forward<Args>(args)...);
		}

		// Do not remove a component from its own TickComponent callback.
		bool RemoveComponent(ActorComponent& component);
		void ForEachComponent(const std::function<void(ActorComponent&)>& visitor);
		void ForEachComponent(const std::function<void(const ActorComponent&)>& visitor) const;

		//TICK WHEN PAUSED
		void SetTickWhenPaused(bool tickWhenPaused) { mTickWhenPaused = tickWhenPaused; }
		bool GetTickWhenPaused() const { return mTickWhenPaused; }
		void SetRenderLayer(RenderLayer renderLayer) { mRenderLayer = renderLayer; }
		RenderLayer GetRenderLayer() const { return mRenderLayer; }

		//DAMAGE
		virtual void ApplyDamage(float amt);


		template<typename ActorType>
		ActorType* GetActor() { return dynamic_cast<ActorType*>(this); }

		//SPRITE AND TRANSFORM
		// The sprite lives in SpriteComponent (FindComponent<SpriteComponent>()); the
		// transform stays here and is pushed into the sprite on every change.
		// Temporarily suppresses every base-actor render pass without changing a
		// sprite's authored tint/alpha. Portal transfer uses this instead of
		// SetVisibility so temporary invisibility cannot overwrite another effect.
		void SetRenderEnabled(bool enabled) { mRenderEnabled = enabled; }
		bool IsRenderEnabled() const { return mRenderEnabled; }
		// Transform-level convenience that asks the actor's SpriteComponent for its
		// global bounds; an actor without a sprite yields a zero-size rect at its
		// location. Used for physics body sizing and out-of-window checks.
		sf::FloatRect GetActorGlobalBounds() const;
		// Returns a conservative bounds for the base sprite/light render path.
		// Actors that render custom geometry without a base sprite or light return
		// no bounds so World keeps rendering them rather than risking a false cull.
		virtual std::optional<sf::FloatRect> GetRenderBounds() const;
		void SetActorLocation(const sf::Vector2f& newLoc);
		void SetActorRotation(float newRotation);
		void AddActorLocationOffset(const sf::Vector2f& offset);
		void AddActorRotationOffset(float offset);
		sf::Vector2f GetActorLocation() const;
		float GetActorRotation() const;
		// Attachment uses ordinary XY rotation (degrees, clockwise on screen).
		// World values stay cached; edits synchronously update the complete subtree.
		Actor* GetParent() const { return mParent; }
		const std::vector<Actor*>& GetChildren() const { return mChildren; }
		bool IsDescendantOf(const Actor& ancestor) const;
		bool AttachTo(Actor* parent, bool keepWorld = true);
		void Detach();
		sf::Vector2f GetRelativeLocation() const { return mRelativeLocation; }
		float GetRelativeRotation() const { return mRelativeRotation; }
		void SetRelativeLocation(const sf::Vector2f& location);
		void SetRelativeRotation(float rotation);
		sf::Vector2f GetActorForwardDirection() const;
		sf::Vector2f GetActorRightDirection() const;
		sf::Vector2u GetWindowSize() const;
		sf::Vector2f TransformLocalToWorld(const sf::Vector2f& localOffset) const;
		void AddActorLocalLocationOffset(const sf::Vector2f& localOffset);
		sf::Vector2f GetActorLocalLocation(const sf::Vector2f& worldLocation) const;
		sf::Vector2f GetVelocity() const;
		void SetVelocity(const sf::Vector2f& velocity);

		Delegate<Actor*> onActorDestroyed;

	protected:
		bool& GetCanCollide() { return mCanCollide; } 
		void SetCollisionRadius(float radius);

		sf::Vector2f mVelocity;

	private:
		void CacheComponent(ActorComponent& component);
		void TickComponents(float deltaTime);
		void EndPlayComponents();
		void SyncWorldTransform();
		void ResolveAttachmentTransform();

		EngineContext* mContext;
		World* mOwningWorld;
		SimulationTimeDomain mSimulationTimeDomain = SimulationTimeDomain::RealTime;
		bool mBeganPlay;

		bool mCanCollide;
		bool mTickWhenPaused;

		sf::Vector2f mActorLocation{ 0.f, 0.f };
		float mActorRotation = 0.f;
		Actor* mParent = nullptr;
		std::vector<Actor*> mChildren;
		sf::Vector2f mRelativeLocation{};
		float mRelativeRotation = 0.f;
		bool mDestroying = false;

		RenderLayer mRenderLayer = RenderLayer::World;
		bool mRenderEnabled = true;

		std::vector<std::unique_ptr<ActorComponent>> mComponents;
		// First component of each hot-path type, cached by AddComponent so transform
		// updates, physics queries and Render avoid scanning mComponents with
		// dynamic_cast. Owned by mComponents; same lifetime as the actor.
		SpriteComponent* mSprite = nullptr;
		CollisionComponent* mCollision = nullptr;
		LightComponent* mLights = nullptr;
		// World-owned manual spatial queries use this stamp to deduplicate an
		// actor that overlaps more than one grid cell without allocating a
		// per-query set.
		mutable std::uint64_t mLastManualSpatialQueryStamp = 0;

		friend class World;
	};
}
