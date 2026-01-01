#pragma once
#include "framework/Object.h"
#include "framework/Core.h"
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h> 
#include <optional>
#include "engineConfigs/EngineStructs.h"

//TODO: : Actor sýnýfýnýn destructor'ýna (~Actor) UnInitializePhysics() eklemen.  
// World sýnýfýnýn yýkýcý metodunda (~World) o dünyaya ait tüm timerlarýn temizlendiðinden emin olmak daha güvenli olurdu, ama weak_ptr ve BindAction yapýn þu an bunu güvenli kýlýyor.
//Çözüm: World yýkýcý metodunda (Destructor) veya LoadWorld ile yeni dünya yüklenirken eski dünyanýn temizlendiðinden emin olunmalý. Þu anki C++ shared_ptr yapýsý bunu büyük 
// oranda hallediyor, ancak mPendingActors içindekiler hiç sahneye çýkmadan silinecek. Bu genellikle sorun yaratmaz ama aklýnda bulunsun.



namespace ly
{
	enum class LightSpace : uint8_t
	{
		Local =0,
		World = 1
	};

	struct LightData
	{
		GameplayTag tag;
		LightSpace lightSpace;
		shared_ptr<sf::Shader> shader;
		sf::Color color;
		float intensity;
		sf::Vector2f size;
		sf::Vector2f offset;
		bool isEnabled;
		bool shouldStretch;
		bool useComplexTrail;
		float currentStretchFactor;
		float currentRotationOffset;
		float taperAmount;
		float edgeSoftness;
		float shapeRoundness;
		LightData() :
			tag{}, color(sf::Color::White), intensity(1.f), size{100.f, 100.f},
			offset{ 0.f, 0.f }, isEnabled(true), shouldStretch(false), currentStretchFactor(1.f),
			currentRotationOffset(0.f), useComplexTrail{false}, taperAmount(0.f), edgeSoftness(1.f),
			shapeRoundness(1.f), lightSpace(LightSpace::Local) {
		}
	};

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

		//PHYSICS
		void SetEnablePhysics(bool enable);
		void InitializePhysics();
		void UnInitializePhysics();
		void UpdatePhysicsTransform();
		bool CanCollideWith(const Actor* other) const;
		virtual	void OnActorEndOverlap(Actor* otherActor);
		virtual void OnActorBeginOverlap(Actor* otherActor);
		virtual void Destroy() override;
		CollisionLayer GetCollisionLayer() const { return mCollisionLayer; }
		CollisionLayer GetCollisionMask() const { return mCollisionMask; }
		void SetCollisionLayer(CollisionLayer layer) { mCollisionLayer = layer; }
		void SetCollisionMask(CollisionLayer mask) { mCollisionMask = mask; }


		// LIGHT SYSTEM - GameplayTag Based (Auto-Indexed)
		void SetLightEnabled(const GameplayTag& tag, bool enabled);
		void SetLightColor(const GameplayTag& tag, const sf::Color& color);
		void SetLightIntensity(const GameplayTag& tag, float intensity);
		void SetLightSize(const GameplayTag& tag, const sf::Vector2f& size);
		void SetLightOffset(const GameplayTag& tag, const sf::Vector2f& offset);
		void SetLightWorldPosition(const GameplayTag& tag, const sf::Vector2f& worldPosition);


		GameplayTag AddLight(const GameplayTag& tag, const std::string& lightPath, sf::Color color,
			float intensity, sf::Vector2f size, sf::Vector2f offset, bool shouldStretch,
			bool useComplexTrail, float taperAmount, float edgeSoftness, float shapeRoundness = 1.f, LightSpace lightSpace = LightSpace::Local);
		GameplayTag AddLight(const GameplayTag& tag, const PointLightDefinition& def, const sf::Vector2f& offset, LightSpace lightSpace = LightSpace::Local);

		LightData* GetLightData(const GameplayTag& tag);
		bool HasLight(const GameplayTag& tag) const;
		void RemoveLight(const GameplayTag& tag);


		List<GameplayTag> GetLightsByBaseTag(const GameplayTag& baseTag) const;
		
		void SetAllLightsEnabled(const GameplayTag& baseTag, bool enabled);
		void SetAllLightsColor(const GameplayTag& baseTag, const sf::Color& color);
		void SetAllLightsIntensity(const GameplayTag& baseTag, float intensity);

		//TICK WHEN PAUSED
		void SetTickWhenPaused(bool tickWhenPaused) { mTickWhenPaused = tickWhenPaused; }
		bool GetTickWhenPaused() const { return mTickWhenPaused; }

		//DAMAGE
		virtual void ApplyDamage(float amt);


		template<typename ActorType>
		ActorType* GetActor() { return dynamic_cast<ActorType*>(this); }

		//SPRITE AND TRANSFORM
		void SetVisibility(bool visible) { if (mSprite) mSprite->setColor(visible ? sf::Color::White : sf::Color::Transparent); }
		std::optional<sf::Sprite>& GetSprite() { return mSprite; }
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
		sf::Vector2f TransformLocalToWorld(const sf::Vector2f& localOffset) const;
		void AddActorLocalLocationOffset(const sf::Vector2f& localOffset);
		sf::Vector2f GetActorLocalLocation(const sf::Vector2f& worldLocation) const;
		sf::Vector2f GetVelocity() const;
		void SetVelocity(const sf::Vector2f& velocity);

		Delegate<Actor*> onActorDestroyed;

	protected:
		bool& GetCanCollide() { return mCanCollide; } 
		void SetCollisionRadius(float radius);

		void RenderLights(sf::RenderWindow& window);

		sf::Vector2f mVelocity;

	private:
		int GetNextLightIndex(const GameplayTag& baseTag) const;

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

		Dictionary<GameplayTag, LightData, GameplayTagHash> mLightShaders;
	};
}