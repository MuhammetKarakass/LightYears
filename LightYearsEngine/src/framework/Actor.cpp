#include "framework/Actor.h"
#include "framework/Core.h"
#include "framework/AssetManager.h"
#include "framework/World.h"
#include "framework/MathUtility.h"
#include <framework/PhysicsSystem.h>

namespace ly
{
	// Actor constructor - Oyun d�nyas�ndaki her nesnenin temel kurucusu
	Actor::Actor(World* OwningWorld, const std::string& TexturePath)
		: mOwningWorld{ OwningWorld },  // Bu actor'�n sahibi olan d�nya
		mBeganPlay{ false },            // Hen�z BeginPlay �a�r�lmad�
		mSprite{},                      // Grafik sprite (bo� ba�l�yor)
		mTexture{},                     // Texture dosyas� referans�
		mPhysicsBodyId{},               // Box2D fizik body'si (bo� ba�l�yor)
		mPhysicsEnabled{ false },       // Fizik sistemi kapal� ba�l�yor
		mCollisionLayer{ CollisionLayer::None },  // Hangi katmanda �arp���r
		mCollisionMask{ CollisionLayer::None },   // Hangi katmanlar� alg�lar
		mCanCollide{ false }            // �u an �arp��ma durumu
	{
		SetTexture(TexturePath);  // Verilen texture'� y�kle ve sprite'a ata
	}
	
	Actor::~Actor()
	{
		LOG("Actor destroyed");  
	}
	
	void Actor::BeginPlayInternal()
	{
		if (!mBeganPlay)  
		{
			mBeganPlay = true;    
			BeginPlay();          
		}
	}
	
	void Actor::BeginPlay()
	{
		
	}

	void Actor::TickInternal(float deltaTime)
	{
		if (mBeganPlay && !GetIsPendingDestroy())
		{
			Tick(deltaTime);  // Ger�ek Tick'i �a��r (virtual)
		}
	}

	void Actor::Tick(float deltaTime)
	{
		// Base class'ta bo� - t�retilen s�n�flar kendi mant���n� ekler
	}
	
	void Actor::Destroy()
	{
		UnInitializePhysics();  // �nce fizik sisteminden ��kar
		Object::Destroy();      // Sonra base class destruction
	}
	
	void Actor::Render(sf::RenderWindow& window)
	{
		if (!mSprite.has_value() || GetIsPendingDestroy()) return;
		window.draw(mSprite.value());  
	}
	
	bool Actor::IsActorOutOfWindow(float allowance) const
	{
		float windowWidth = GetWindowSize().x;   
		float windowHeight = GetWindowSize().y;  

		float width = GetActorGlobalBounds().size.x;   
		float height = GetActorGlobalBounds().size.y;  

		sf::Vector2f actorPosition = GetActorLocation();  

		if(actorPosition.x < -width-allowance-10.f)
			return true;

		if(actorPosition.x > windowWidth + width+allowance+10.f)
			return true;

		if (actorPosition.y < -height-allowance-10.f)
			return true;

		if (actorPosition.y > windowHeight + height+allowance+10.f)
			return true;

		return false;  
	}
	
	void Actor::SetEnablePhysics(bool enable)
	{
		mPhysicsEnabled = enable;

		if (mPhysicsEnabled)
		{
			InitializePhysics();    
		}
		else
		{
			UnInitializePhysics();  
		}
	}
	
	void Actor::InitializePhysics()
	{
		if (!mPhysicsBodyId)  
		{
			mPhysicsBodyId = PhysicsSystem::Get().AddListener(this);
		}
	}
	
	void Actor::UnInitializePhysics()
	{
		if(mPhysicsBodyId)
		{
			PhysicsSystem::Get().RemoveListener(*mPhysicsBodyId);
			mPhysicsBodyId.reset();  
		}
	}

	void Actor::UpdatePhysicsTransform()
	{
		if (mPhysicsBodyId)  // Fizik body'si varsa
		{
			float physicsRate = PhysicsSystem::Get().GetPhysicsRate();  // �l�ek fakt�r�
			sf::Vector2f actorLocation = GetActorLocation();            // Sprite pozisyonu
			float actorRotation = GetActorRotation();                   // Sprite rotasyonu
			
			b2Vec2 position{ actorLocation.x * physicsRate, actorLocation.y * physicsRate };
			b2Rot rotation = b2MakeRot(DegreesToRadians(actorRotation));  // Derece ? Radian
			
			b2Body_SetTransform(*mPhysicsBodyId, position, rotation);
		}
	}

	bool Actor::CanCollideWith(const Actor* other) const
	{
		if (other == nullptr) return false;
		return HasCollisionLayer(mCollisionMask, other->GetCollisionLayer());
	}

	void Actor::OnActorBeginOverlap(Actor* otherActor)
	{
		if (!otherActor) return;
		
		// Her iki taraf da birbirini maskelemeli (�ift y�nl� filtre)
		if (!CanCollideWith(otherActor) || !otherActor->CanCollideWith(this))
		{
			return; 
		}

		mCanCollide = !(!CanCollideWith(otherActor) || !otherActor->CanCollideWith(this));
	}

	void Actor::OnActorEndOverlap(Actor* otherActor)
	{
		if (!otherActor) return;
		
		if (!CanCollideWith(otherActor) || !otherActor->CanCollideWith(this))
		{
			return; 
		}
	}

	// Bu actor'a hasar verilmesi (virtual - override edilebilir)
	void Actor::ApplyDamage(float amt)
	{
		
	}

	sf::FloatRect Actor::GetActorGlobalBounds() const
	{
		return mSprite.value().getGlobalBounds(); 
	}
	
	void Actor::SetTexture(const std::string& texturePath)
	{
		mTexture = AssetManager::GetAssetManager().LoadTexture(texturePath);
		if (!mTexture) return;  

		mSprite.emplace(*mTexture);

		int width = mTexture->getSize().x;
		int height = mTexture->getSize().y;
		mSprite.value().setTextureRect(sf::IntRect{ sf::Vector2i{}, sf::Vector2i{width,height} });
		
		CenterPivot();  
	}

	sf::Vector2u Actor::GetWindowSize() const
	{
		return mOwningWorld->GetWindowSize();
	}

	void Actor::SetActorLocation(const sf::Vector2f& newLoc)
	{
		mSprite.value().setPosition(newLoc); 
		UpdatePhysicsTransform();            
	}
	
	void Actor::SetActorRotation(float newRotation)
	{
		mSprite.value().setRotation(sf::degrees(newRotation));  
		UpdatePhysicsTransform();                               
	}
	
	void Actor::AddActorLocationOffset(const sf::Vector2f& offset)
	{
		SetActorLocation(GetActorLocation() + offset);  
	}
	
	void Actor::AddActorRotationOffset(float offset)
	{
		SetActorRotation(GetActorRotation() + offset);  
	}
	
	void Actor::CenterPivot()
	{
		sf::FloatRect rectBounds = mSprite.value().getGlobalBounds();  
		mSprite.value().setOrigin(sf::Vector2f{rectBounds.size.x/2.f, rectBounds.size.y/2.f});
	}
	
	sf::Vector2f Actor::GetActorLocation() const
	{
		return mSprite.value().getPosition();  
	}
	
	float Actor::GetActorRotation() const
	{
		return mSprite.value().getRotation().asDegrees(); 
	}
	
	sf::Vector2f Actor::GetActorForwardDirection() const
	{
		return RotationToVector(GetActorRotation() - 90.f); 
	}
	
	sf::Vector2f Actor::GetActorRightDirection() const
	{
		return RotationToVector(GetActorRotation());  
	}
	
	// ========================================================================
	// LOCAL SPACE TRANSFORMATION SYSTEM - SPRITE'IN KEND� KOORD�NAT S�STEM�
	// ========================================================================
	sf::Vector2f Actor::TransformLocalToWorld(const sf::Vector2f& localOffset) const
	{

		sf::Vector2f right = GetActorRightDirection();        // Sa� = direkt rotasyon 
		sf::Vector2f forward = GetActorForwardDirection();    // �leri = rotasyon - 90�
		
		// Linear combination ile local offset'i world'e �evir
		return localOffset.x * right + localOffset.y * forward;
	}
	
	void Actor::AddActorLocalLocationOffset(const sf::Vector2f& localOffset)
	{
		sf::Vector2f worldOffset = TransformLocalToWorld(localOffset);  // Local ? World d�n���m�
		AddActorLocationOffset(worldOffset);                           // World space'de hareket ettir
	}
	
	sf::Vector2f Actor::GetActorLocalLocation(const sf::Vector2f& worldLocation) const
	{
		// INVERSE TRANSFORMATION
		sf::Vector2f deltaWorld = worldLocation - GetActorLocation();  // D�nya'da sprite'dan target'a vekt�r
		float rotation = GetActorRotation();
		float radians = DegreesToRadians(rotation - 90.f);            // MathUtility haz�r fonksiyonunu kullan
		
		// INVERSE ROTATION MATRIX UYGULA
		float cosR = std::cos(-radians);  // Inverse rotation cosine
		float sinR = std::sin(-radians);  // Inverse rotation sine
		
		return sf::Vector2f{
			deltaWorld.x * cosR - deltaWorld.y * sinR,  // Local X (sprite'�n sa�/sol ekseni)
			deltaWorld.x * sinR + deltaWorld.y * cosR   // Local Y (sprite'�n ileri/geri ekseni)  
		};
	}
}