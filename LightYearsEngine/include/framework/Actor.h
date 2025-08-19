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
		bool IsActorOutOfWindow() const;

		void SetEnablePhysics(bool enable);
		void InitializePhysics();
		void UnInitializePhysics();
		void UpdatePhysicsTransform();

		void OnActorBeginOverlap(Actor* otherActor);
		void OnActorEndOverlap(Actor* otherActor);
		virtual void Destroy() override;

		CollisionLayer GetCollisionLayer() const { return mCollisionLayer; }
		CollisionLayer GetCollisionMask() const { return mCollisionMask; }
		void SetCollisionLayer(CollisionLayer layer) { mCollisionLayer = layer; }
		void SetCollisionMask(CollisionLayer mask) { mCollisionMask = mask; }

		sf::FloatRect GetActorGlobalBounds() const;
		void SetTexture(const std::string& texturePath);
		void SetActorLocation(const sf::Vector2f& newLoc);
		void SetActorRotation(float newRotation);
		void AddActorLocationOffset(const sf::Vector2f& offset);
		void AddActorRotationOffset(float offset);
		void CenterPivot();
		sf::Vector2f GetActorLocation() const;
		float GetActorRotation() const;
		sf::Vector2f GetActorForwardDirection();
		sf::Vector2f GetActorRightDirection();
		sf::Vector2u GetWindowSize() const;


	private:
		World* mOwningWorld;
		bool mBeganPlay;

		std::optional<sf::Sprite> mSprite;
		shared_ptr<sf::Texture> mTexture;

		bool mPhysicsEnabled;
		std::optional<b2BodyId> mPhysicsBodyId;

		CollisionLayer mCollisionLayer;
		CollisionLayer mCollisionMask;
	}
}