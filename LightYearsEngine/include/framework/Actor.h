#pragma once
#include "framework/Object.h"
#include "framework/Core.h"
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h> 
#include <optional>

//TODO: : Actor sýnýfýnýn destructor'ýna (~Actor) UnInitializePhysics() eklemen.  
// World sýnýfýnýn yýkýcý metodunda (~World) o dünyaya ait tüm timerlarýn temizlendiðinden emin olmak daha güvenli olurdu, ama weak_ptr ve BindAction yapýn þu an bunu güvenli kýlýyor.
//Çözüm: World yýkýcý metodunda (Destructor) veya LoadWorld ile yeni dünya yüklenirken eski dünyanýn temizlendiðinden emin olunmalý. Þu anki C++ shared_ptr yapýsý bunu büyük 
// oranda hallediyor, ancak mPendingActors içindekiler hiç sahneye çýkmadan silinecek. Bu genellikle sorun yaratmaz ama aklýnda bulunsun.

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

		virtual void Render(sf::RenderWindow& window);

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
		
		void SetTickWhenPaused(bool tickWhenPaused) { mTickWhenPaused = tickWhenPaused; }
		bool GetTickWhenPaused() const { return mTickWhenPaused; }

		virtual void ApplyDamage(float amt);

		sf::Sprite& GetSprite() { return mSprite.value(); }

		template<typename ActorType>
		ActorType* GetActor() { return dynamic_cast<ActorType*>(this); }

		void SetVisibility(bool visible) { if (mSprite) mSprite->setColor(visible ? sf::Color::White : sf::Color::Transparent); }

		sf::FloatRect GetActorGlobalBounds() const;
		void SetTexture(const std::string& texturePath);
		void SetActorLocation(const sf::Vector2f& newLoc);
		void SetActorRotation(float newRotation);
		void SetTextureRepeated(bool repeated);
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

		Delegate<Actor*> onActorDestroyed;

	protected:
		bool& GetCanCollide() { return mCanCollide; } 
		void SetCollisionRadius(float radius);

	private:
		World* mOwningWorld;
		bool mBeganPlay;

		bool mCanCollide;
		bool mTickWhenPaused;

		std::optional<sf::Sprite> mSprite;
		shared_ptr<sf::Texture> mTexture;

		bool mPhysicsEnabled;
		std::optional<b2BodyId> mPhysicsBodyId;

		CollisionLayer mCollisionLayer;
		CollisionLayer mCollisionMask;
	};
}