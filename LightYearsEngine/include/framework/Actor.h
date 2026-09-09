#pragma once
#include "framework/Object.h"
#include "framework/Core.h"
#include "framework/SimulationTime.h"
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h> 
#include <cstddef>
#include <optional>
#include "engineConfigs/EngineStructs.h"

//TODO: : Actor sınıfının destructor'ına (~Actor) UnInitializePhysics() eklemen.  
// World sınıfının yıkıcı metodunda (~World) o dünyaya ait tüm timerların temizlendiğinden emin olmak daha güvenli olurdu, ama weak_ptr ve BindAction yapın şu an bunu güvenli kılıyor.
//Çözüm: World yıkıcı metodunda (Destructor) veya LoadWorld ile yeni dünya yüklenirken eski dünyanın temizlendiğinden emin olunmalı. Şu anki C++ shared_ptr yapısı bunu büyük 
// oranda hallediyor, ancak mPendingActors içindekiler hiç sahneye çıkmadan silinecek. Bu genellikle sorun yaratmaz ama aklında bulunsun.



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
	public:
        Actor(World* OwningWorld, const std::string& TexturePath = "");
		virtual ~Actor();

		void BeginPlayInternal();
		virtual void BeginPlay();
		void TickInternal(float deltaTime);
		virtual void Tick(float deltaTime);

		virtual void Render(sf::RenderWindow& window);

		World* GetWorld() const { return mOwningWorld; }
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
		void SetEnablePhysics(bool enable);
		bool IsPhysicsEnabled() const { return mPhysicsEnabled; }
		bool HasPhysicsBody() const { return mPhysicsBodyId.has_value(); }
		void InitializePhysics();
		void UnInitializePhysics();
		void UpdatePhysicsTransform();
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
		PhysicsBodyType GetPhysicsBodyType() const { return mPhysicsBodyType; }
		bool CanCollideWith(const Actor* other) const;
		virtual	void OnActorEndOverlap(Actor* otherActor);
		virtual void OnActorBeginOverlap(Actor* otherActor);
		virtual void Destroy() override;
		CollisionLayer GetCollisionLayer() const { return mCollisionLayer; }
		CollisionLayer GetCollisionMask() const { return mCollisionMask; }
		void SetCollisionLayer(CollisionLayer layer);
		void SetCollisionMask(CollisionLayer mask);


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
		void SetRenderLayer(RenderLayer renderLayer) { mRenderLayer = renderLayer; }
		RenderLayer GetRenderLayer() const { return mRenderLayer; }

		//DAMAGE
		virtual void ApplyDamage(float amt);


		template<typename ActorType>
		ActorType* GetActor() { return dynamic_cast<ActorType*>(this); }

		//SPRITE AND TRANSFORM
		void SetVisibility(bool visible) { if (mSprite) mSprite->setColor(visible ? sf::Color::White : sf::Color::Transparent); }
		// Temporarily suppresses every base-actor render pass without changing a
		// sprite's authored tint/alpha. Portal transfer uses this instead of
		// SetVisibility so temporary invisibility cannot overwrite another effect.
		void SetRenderEnabled(bool enabled) { mRenderEnabled = enabled; }
		bool IsRenderEnabled() const { return mRenderEnabled; }
		std::optional<sf::Sprite>& GetSprite() { return mSprite; }
		sf::FloatRect GetActorGlobalBounds() const;
		// Returns a conservative bounds for the base sprite/light render path.
		// Actors that render custom geometry without a base sprite or light return
		// no bounds so World keeps rendering them rather than risking a false cull.
		virtual std::optional<sf::FloatRect> GetRenderBounds() const;
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
		SimulationTimeDomain mSimulationTimeDomain = SimulationTimeDomain::RealTime;
		bool mBeganPlay;

		bool mCanCollide;
		bool mTickWhenPaused;

		std::optional<sf::Sprite> mSprite;
		shared_ptr<sf::Texture> mTexture;
		sf::Vector2f mActorLocation{ 0.f, 0.f };
		float mActorRotation = 0.f;

		bool mPhysicsEnabled;
		std::optional<b2BodyId> mPhysicsBodyId;
		PhysicsBodyType mPhysicsBodyType = PhysicsBodyType::Dynamic;

		CollisionLayer mCollisionLayer;
		CollisionLayer mCollisionMask;
		RenderLayer mRenderLayer = RenderLayer::World;
		bool mRenderEnabled = true;

		Dictionary<GameplayTag, LightData, GameplayTagHash> mLightShaders;
		// World-owned manual spatial queries use this stamp to deduplicate an
		// actor that overlaps more than one grid cell without allocating a
		// per-query set.
		mutable std::uint64_t mLastManualSpatialQueryStamp = 0;

		friend class World;
	};
}
