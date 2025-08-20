#include "framework/PhysicsSystem.h"
#include "framework/Actor.h"	
#include "framework/MathUtility.h"

namespace ly
{
	unique_ptr<PhysicsSystem> PhysicsSystem::physicsSystem{ nullptr };

	PhysicsSystem& PhysicsSystem::Get()
	{
		if (!physicsSystem)
		{
			physicsSystem = std::move(unique_ptr<PhysicsSystem>{new PhysicsSystem});
		}
		return *physicsSystem;
	}

	void PhysicsSystem::Step(float deltaTime)
	{
		// Box2D v3.0 C-style API kullanýyor
		if (mPhysicsWorld.index1 != 0)  // Null check
		{
			ProcessPendingRemoveListeners();
			b2World_Step(mPhysicsWorld, deltaTime, 4);
			
			// Step sonrasý contact event'leri iþle
			ProcessContactEvents();
		}
	}

	void PhysicsSystem::InitializeWorld(b2Vec2 gravity)
	{
		// Eski world'ü temizle (eðer varsa)
		Cleanup();

		// Yeni world oluþtur
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = gravity;
		
		mPhysicsWorld = b2CreateWorld(&worldDef);

		// Uyku nedeniyle event'lerin gecikmesini engelle
		b2World_EnableSleeping(mPhysicsWorld, false);
	}

	void PhysicsSystem::Cleanup()
	{
		if (mPhysicsWorld.index1 != 0)
		{
			b2DestroyWorld(mPhysicsWorld);
			mPhysicsWorld = b2WorldId{0, 0};  // Null ID
		}
	}

	b2BodyId PhysicsSystem::AddListener(Actor* listener)
	{
		if (listener->GetIsPendingDestroy()) return b2BodyId{0,0,0};

		// Box2D v3.x API kullanýr
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = b2_dynamicBody;                  // Dynamic body - contact events yaratýr
		
		// userData artýk doðrudan void* pointer
		bodyDef.userData = listener;
		
		// position ve rotation ayrý ayrý ayarlanýr
		sf::Vector2f actorLocation = listener->GetActorLocation();
		bodyDef.position = {actorLocation.x * GetPhysicsRate(), actorLocation.y * GetPhysicsRate()};
		bodyDef.rotation = b2MakeRot(DegreesToRadians(listener->GetActorRotation()));

		// Body oluþtur
		b2BodyId bodyId = b2CreateBody(mPhysicsWorld, &bodyDef);
		
		// Shape oluþtur - Box2D v3.x API
		sf::FloatRect bounds = listener->GetActorGlobalBounds();
		float halfWidth = bounds.size.x / 2.0f * GetPhysicsRate();
		float halfHeight = bounds.size.y / 2.0f * GetPhysicsRate();
		
		// Polygon (kutu) oluþtur
		b2Polygon box = b2MakeBox(halfWidth, halfHeight);
		
		// Shape definition - Box2D v3.x'te b2FixtureDef yerine b2ShapeDef kullanýlýr
		b2ShapeDef shapeDef = b2DefaultShapeDef();
		
		// Fiziksel özellikler (eski b2FixtureDef özelliklerinin karþýlýðý)
		shapeDef.density = 1.0f;                        // eski: fixtureDef.density
		shapeDef.material.friction = 0.3f;              // eski: fixtureDef.friction
		shapeDef.material.restitution = 0.6f;           // eski: fixtureDef.restitution
		
		// Contact events için dynamic body kullan - overlap detection için
		shapeDef.isSensor = false;                      // Solid shape - contact events için

		// Event'lerin etkinleþtirilmesi
		shapeDef.enableContactEvents = true;            // Contact event'leri aktif et
		shapeDef.enableSensorEvents = false;            // Sensor event'leri kapalý
		
		// Kritik: Ýlk frame'de temas çiftlerini oluþtur (initial overlap ve uyku problemleri için)
		shapeDef.invokeContactCreation = true;
		
		// Shape'i body'ye ekle
		b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &box);
		
		return bodyId;
	}

	void PhysicsSystem::ProcessContactEvents()
	{
		if (mPhysicsWorld.index1 == 0) return;  // World yok ise çýk
		
		// Box2D v3.x event-based contact system
		b2ContactEvents contactEvents = b2World_GetContactEvents(mPhysicsWorld);
		
		// Begin touch events - solid body'ler için contact detection
		for (int i = 0; i < contactEvents.beginCount; ++i)
		{
			b2ContactBeginTouchEvent beginEvent = contactEvents.beginEvents[i];
			
			Actor* actorA = nullptr;
			Actor* actorB = nullptr;
			
			// Shape A'dan Actor'ü al
			if (b2Shape_IsValid(beginEvent.shapeIdA))
			{
				b2BodyId bodyIdA = b2Shape_GetBody(beginEvent.shapeIdA);
				if (b2Body_IsValid(bodyIdA))
				{
					void* userDataA = b2Body_GetUserData(bodyIdA);
					if (userDataA)
					{
						actorA = static_cast<Actor*>(userDataA);
					}
				}
			}
			
			// Shape B'den Actor'ü al
			if (b2Shape_IsValid(beginEvent.shapeIdB))
			{
				b2BodyId bodyIdB = b2Shape_GetBody(beginEvent.shapeIdB);
				if (b2Body_IsValid(bodyIdB))
				{
					void* userDataB = b2Body_GetUserData(bodyIdB);
					if (userDataB)
					{
						actorB = static_cast<Actor*>(userDataB);
					}
				}
			}
			
			// Her iki actor da geçerliyse overlap event'lerini çaðýr
			if (actorA && !actorA->GetIsPendingDestroy())
			{
				actorA->OnActorBeginOverlap(actorB);
			}

			if (actorB && !actorB->GetIsPendingDestroy())
			{
				actorB->OnActorBeginOverlap(actorA);
			}
		}
		
		// End touch events - solid body'ler için contact detection
		for (int i = 0; i < contactEvents.endCount; ++i)
		{
			b2ContactEndTouchEvent endEvent = contactEvents.endEvents[i];
			
			Actor* actorA = nullptr;
			Actor* actorB = nullptr;
			
			// Shape A'dan Actor'ü al
			if (b2Shape_IsValid(endEvent.shapeIdA))
			{
				b2BodyId bodyIdA = b2Shape_GetBody(endEvent.shapeIdA);
				if (b2Body_IsValid(bodyIdA))
				{
					void* userDataA = b2Body_GetUserData(bodyIdA);
					if (userDataA)
					{
						actorA = static_cast<Actor*>(userDataA);
					}
				}
			}
			
			// Shape B'den Actor'ü al
			if (b2Shape_IsValid(endEvent.shapeIdB))
			{
				b2BodyId bodyIdB = b2Shape_GetBody(endEvent.shapeIdB);
				if (b2Body_IsValid(bodyIdB))
				{
					void* userDataB = b2Body_GetUserData(bodyIdB);
					if (userDataB)
					{
						actorB = static_cast<Actor*>(userDataB);
					}
				}
			}
			
			if (actorA && !actorA->GetIsPendingDestroy())
			{
				actorA->OnActorEndOverlap(actorB);
			}

			if (actorB && !actorB->GetIsPendingDestroy())
			{
				actorB->OnActorEndOverlap(actorA);
			}
		}
	}

	void PhysicsSystem::RemoveListener(b2BodyId bodyId)
	{
		mPendingRemoveListeners.push_back(bodyId);
	}

	bool PhysicsSystem::IsInitialized() const
	{
		return mPhysicsWorld.index1 != 0;
	}

	PhysicsSystem::PhysicsSystem():
		mPhysicsWorld{0, 0},  // Null WorldId ile baþlat
		mPhysicsRate{0.01f},
		mPendingRemoveListeners{}
	{
		// Varsayýlan gravity ile world'ü initialize et
		InitializeWorld({0.0f, 0.0f});
	}

	void PhysicsSystem::ProcessPendingRemoveListeners()
	{
		for(auto bodyId : mPendingRemoveListeners)
		{
			if (b2Body_IsValid(bodyId))
			{
				// Body'yi world'den kaldýr
				b2DestroyBody(bodyId);
			}
		}
		mPendingRemoveListeners.clear();
	}

	PhysicsSystem::~PhysicsSystem()
	{
		Cleanup();
	}
}