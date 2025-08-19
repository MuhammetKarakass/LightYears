#include "framework/Actor.h"
#include "framework/Core.h"
#include "framework/AssetManager.h"
#include "framework/World.h"
#include "framework/MathUtility.h"
#include <framework/PhysicsSystem.h>

namespace ly
{
	Actor::Actor(World* OwningWorld,const std::string& TexturePath)
		: mOwningWorld{OwningWorld},
		  mBeganPlay{false},
		mSprite{},
		mTexture{},
		mPhysicsBodyId{},
		mPhysicsEnabled{ false },
		mCollisionLayer{ CollisionLayer::None },
		mCollisionMask{ CollisionLayer::None }
	{
		SetTexture(TexturePath);
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
		if (mBeganPlay&&!GetIsPendingDestroy())
		{
			Tick(deltaTime);
		}
	}

	void Actor::Tick(float deltaTime)
	{
		
	}
	void Actor::Destroy()
	{

		UnInitializePhysics();

		Object::Destroy();
	}
	void Actor::Render(sf::RenderWindow& window)
	{

		if (!mSprite.has_value() || GetIsPendingDestroy()) return;
			window.draw(mSprite.value());
	}
	bool Actor::IsActorOutOfWindow() const
	{
		float windowWidth = GetWindowSize().x;
		float windowHeight = GetWindowSize().y;

		float width = GetActorGlobalBounds().size.x;
		float height = GetActorGlobalBounds().size.y;

		sf::Vector2f actorPosition = GetActorLocation();

		if(actorPosition.x<-width)
			return true;

		if(actorPosition.x>windowWidth+width)
			return true;

		if (actorPosition.y < -height)
			return true;

		if (actorPosition.y > windowHeight + height)
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
		if (mPhysicsBodyId)
		{
			float physicsRate = PhysicsSystem::Get().GetPhysicsRate();
			sf::Vector2f actorLocation = GetActorLocation();
			float actorRotation = GetActorRotation();
			b2Vec2 position{ actorLocation.x * physicsRate, actorLocation.y * physicsRate };
			b2Rot rotation = b2MakeRot(DegreesToRadians(actorRotation));
			b2Body_SetTransform(*mPhysicsBodyId, position, rotation);
		}
	}

	void Actor::OnActorBeginOverlap(Actor* otherActor)
	{
		LOG("OVERLAPPED");
	}

	void Actor::OnActorEndOverlap(Actor* otherActor)
	{
		LOG("END OVERLAP");
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

		mSprite.value().setTextureRect(sf::IntRect{ sf::Vector2i{},sf::Vector2i{width,height} });
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
		sf::FloatRect rectBounds= mSprite.value().getGlobalBounds();
		mSprite.value().setOrigin(sf::Vector2f{rectBounds.size.x/2.f,rectBounds.size.y/2.f});
	}
	sf::Vector2f Actor::GetActorLocation() const
	{
		return mSprite.value().getPosition();
	}
	float Actor::GetActorRotation() const
	{
		return mSprite.value().getRotation().asRadians();
	}
	sf::Vector2f Actor::GetActorForwardDirection()
	{
		return RotationToVector(GetActorRotation());
	}
	sf::Vector2f Actor::GetActorRightDirection()
	{
		return RotationToVector(GetActorRotation()+90);
	}
}