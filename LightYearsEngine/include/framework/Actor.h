#pragma once
#include "framework/Object.h"
#include "framework/Core.h"
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h> 
#include <optional>

namespace ly
{
	class World;
	class Actor : public Object
	{
	public:
        Actor(World* OwningWorld, const std::string& TexturePath = "");
		virtual ~Actor();

		void BeginPlayInternal();
		virtual void BeginPlay();
		void TickInternal(float deltaTime);
		virtual void Tick(float deltaTime);

		void Render(sf::RenderWindow& window);

		World* GetWorld() const { return mOwningWorld; }
		bool IsActorOutOfWindow(float allowance=10.f) const;

		void SetEnablePhysics(bool enable);
		void InitializePhysics();
		void UnInitializePhysics();
		void UpdatePhysicsTransform();

		// Collision filtering helper (optional to use by game code)
		bool CanCollideWith(const Actor* other) const;

		// Overlap events called by PhysicsSystem
		virtual	void OnActorEndOverlap(Actor* otherActor);
		virtual void OnActorBeginOverlap(Actor* otherActor);
		virtual void Destroy() override;

		CollisionLayer GetCollisionLayer() const { return mCollisionLayer; }
		CollisionLayer GetCollisionMask() const { return mCollisionMask; }
		void SetCollisionLayer(CollisionLayer layer) { mCollisionLayer = layer; }
		void SetCollisionMask(CollisionLayer mask) { mCollisionMask = mask; }

		virtual void ApplyDamage(float amt);

		sf::Sprite& GetSprite() { return mSprite.value(); }

		sf::FloatRect GetActorGlobalBounds() const;
		void SetTexture(const std::string& texturePath);
		void SetActorLocation(const sf::Vector2f& newLoc);
		void SetActorRotation(float newRotation);
		void AddActorLocationOffset(const sf::Vector2f& offset);
		void AddActorRotationOffset(float offset);
		void CenterPivot();
		sf::Vector2f GetActorLocation() const;
		float GetActorRotation() const;
		sf::Vector2f GetActorForwardDirection() const;
		sf::Vector2f GetActorRightDirection() const;
		sf::Vector2u GetWindowSize() const;
		
		// Local space transformations - sprite'ýn kendi koordinat sisteminde
		sf::Vector2f TransformLocalToWorld(const sf::Vector2f& localOffset) const;
		void AddActorLocalLocationOffset(const sf::Vector2f& localOffset);
		sf::Vector2f GetActorLocalLocation(const sf::Vector2f& worldLocation) const;

	protected:
		bool& GetCanCollide() { return mCanCollide; } 

	private:
		World* mOwningWorld;
		bool mBeganPlay;

		bool mCanCollide;

		std::optional<sf::Sprite> mSprite;
		shared_ptr<sf::Texture> mTexture;

		bool mPhysicsEnabled;
		std::optional<b2BodyId> mPhysicsBodyId;

		CollisionLayer mCollisionLayer;
		CollisionLayer mCollisionMask;
	};
}