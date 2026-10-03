#include "framework/Actor.h"
#include "framework/World.h"
#include "framework/MathUtility.h"
#include "framework/LightComponent.h"
#include "framework/SpriteComponent.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	Actor::Actor(World* OwningWorld, const std::string& TexturePath)
		: mOwningWorld{ OwningWorld },
		mBeganPlay{ false },
		mActorLocation{ 0.f, 0.f },
		mActorRotation{ 0.f },
		mVelocity{},
		mCanCollide{ false },
		mTickWhenPaused{ false }
	{
		if (!TexturePath.empty())
		{
			AddComponent<SpriteComponent>(TexturePath);
		}
	}

	void Actor::CacheComponent(ActorComponent& component)
	{
		if (!mSprite) mSprite = dynamic_cast<SpriteComponent*>(&component);
		if (!mCollision) mCollision = dynamic_cast<CollisionComponent*>(&component);
		if (!mLights) mLights = dynamic_cast<LightComponent*>(&component);
	}
	
	Actor::~Actor()
	{
		EndPlayComponents();
		UnInitializePhysics();
	}
	
	void Actor::BeginPlayInternal()
	{
		if (!mBeganPlay)  
		{
			mBeganPlay = true;
			// Components begin before the actor so BeginPlay can rely on them. Index
			// loop: a component added meanwhile is appended and begun here or by AddComponent.
			for (std::size_t index = 0; index < mComponents.size(); ++index)
			{
				mComponents[index]->BeginPlayInternal();
			}
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
			Tick(deltaTime);
		}
	}

	void Actor::Tick(float deltaTime)
	{
		TickComponents(deltaTime);
	}

	void Actor::TickComponents(float deltaTime)
	{
		for (std::size_t index = 0; index < mComponents.size(); ++index)
		{
			ActorComponent& component = *mComponents[index];
			if (component.CanEverTick() && component.IsActive() && component.HasBegunPlay())
			{
				component.TickComponent(deltaTime);
			}
		}
	}
	
	void Actor::EndPlayComponents()
	{
		for (std::size_t index = 0; index < mComponents.size(); ++index)
		{
			mComponents[index]->EndPlayInternal();
		}
	}

	weak_ptr<Actor> Actor::GetWeakActor()
	{
		const shared_ptr<Object> object = GetWeakPtr().lock();
		return object ? std::static_pointer_cast<Actor>(object) : weak_ptr<Actor>{};
	}

	weak_ptr<const Actor> Actor::GetWeakActor() const
	{
		const shared_ptr<const Object> object = GetWeakPtr().lock();
		return object ? std::static_pointer_cast<const Actor>(object) : weak_ptr<const Actor>{};
	}

	void Actor::Destroy()
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		if (mOwningWorld)
		{
			mOwningWorld->RemoveActorSpatialQuery(*this);
		}
		UnInitializePhysics();
		EndPlayComponents();
		onActorDestroyed.Broadcast(this);
		Object::Destroy();
	}

	void Actor::Render(sf::RenderTarget& window)
	{
		if (GetIsPendingDestroy() || !mRenderEnabled) return;
		if (mLights)
		{
			mLights->Render(window);
		}

		if (mSprite)
		{
			mSprite->Render(window);
		}
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
		GetOrAddComponent<CollisionComponent>().SetEnablePhysics(enable);
	}

	void Actor::UnInitializePhysics()
	{
		if (mCollision) mCollision->UnInitializePhysics();
	}

	void Actor::SetPhysicsBodyType(PhysicsBodyType bodyType)
	{
		GetOrAddComponent<CollisionComponent>().SetPhysicsBodyType(bodyType);
	}

	bool Actor::CanCollideWith(const Actor* other) const
	{
		if (mCollision) return mCollision->CanCollideWith(other);
		return other != nullptr && HasCollisionLayer(CollisionLayer::None, other->GetCollisionLayer());
	}

	void Actor::SetCollisionLayer(CollisionLayer layer)
	{
		GetOrAddComponent<CollisionComponent>().SetCollisionLayer(layer);
	}

	void Actor::SetCollisionMask(CollisionLayer mask)
	{
		GetOrAddComponent<CollisionComponent>().SetCollisionMask(mask);
	}

	void Actor::OnActorBeginOverlap(Actor* otherActor)
	{
		if (!otherActor) return;
		
		if (!CanCollideWith(otherActor) || !otherActor->CanCollideWith(this))
		{
			return; 
		}

		mCanCollide = (CanCollideWith(otherActor) || otherActor->CanCollideWith(this));
	}

	void Actor::OnActorEndOverlap(Actor* otherActor)
	{
		if (!otherActor) return;
		
		if (!CanCollideWith(otherActor) || !otherActor->CanCollideWith(this))
		{
			return; 
		}
	}

	void Actor::ApplyDamage(float amt)
	{
		
	}

	sf::FloatRect Actor::GetActorGlobalBounds() const
	{
		return mSprite && mSprite->HasSprite()
			? mSprite->GetGlobalBounds()
			: sf::FloatRect{ mActorLocation, { 0.f, 0.f } };
	}

	std::optional<sf::FloatRect> Actor::GetRenderBounds() const
	{
		std::optional<sf::FloatRect> result;
		if (mSprite && mSprite->HasSprite())
		{
			result = mSprite->GetGlobalBounds();
		}

		if (mLights)
		{
			mLights->ExpandRenderBounds(result);
		}

		return result;
	}
	
	sf::Vector2u Actor::GetWindowSize() const
	{
		return mOwningWorld->GetWindowSize();
	}

	void Actor::SetActorLocation(const sf::Vector2f& newLoc)
	{
		mActorLocation = newLoc;
		if (mSprite)
		{
			mSprite->SyncLocation();
		}
		if (mCollision)
		{
			mCollision->UpdatePhysicsTransform();
		}
		if (mOwningWorld)
		{
			mOwningWorld->RefreshActorSpatialQuery(*this);
		}
	}
	
	void Actor::SetActorRotation(float newRotation)
	{
		mActorRotation = newRotation;
		if (mSprite)
		{
			mSprite->SyncRotation();
		}
		if (mCollision)
		{
			mCollision->UpdatePhysicsTransform();
		}
		if (mOwningWorld)
		{
			mOwningWorld->RefreshActorSpatialQuery(*this);
		}
	}

	void Actor::AddActorLocationOffset(const sf::Vector2f& offset)
	{
		SetActorLocation(GetActorLocation() + offset);  
	}
	
	void Actor::AddActorRotationOffset(float offset)
	{
		SetActorRotation(GetActorRotation() + offset);  
	}
	
	sf::Vector2f Actor::GetActorLocation() const
	{
		return mActorLocation;
	}
	
	float Actor::GetActorRotation() const
	{
		return mActorRotation;
	}
	
	sf::Vector2f Actor::GetActorForwardDirection() const
	{
		return RotationToVector(GetActorRotation() - 90.f); 
	}
	
	sf::Vector2f Actor::GetActorRightDirection() const
	{
		return RotationToVector(GetActorRotation());  
	}
	
	sf::Vector2f Actor::TransformLocalToWorld(const sf::Vector2f& localOffset) const
	{
		sf::Vector2f right = GetActorRightDirection();
		sf::Vector2f forward = GetActorForwardDirection();
		
		return localOffset.x * right + localOffset.y * forward;
	}
	
	void Actor::AddActorLocalLocationOffset(const sf::Vector2f& localOffset)
	{
		sf::Vector2f worldOffset = TransformLocalToWorld(localOffset);
		AddActorLocationOffset(worldOffset);
	}
	
	sf::Vector2f Actor::GetActorLocalLocation(const sf::Vector2f& worldLocation) const
	{
		const sf::Vector2f deltaWorld = worldLocation - GetActorLocation();
		return sf::Vector2f{ deltaWorld.dot(GetActorRightDirection()), deltaWorld.dot(GetActorForwardDirection()) };
	}

	void Actor::SetCollisionRadius(float radius)
	{
		if (mCollision) mCollision->SetCollisionRadius(radius);
	}

	sf::Vector2f Actor::GetVelocity() const
	{
		return mVelocity;
	}

	void Actor::SetVelocity(const sf::Vector2f& velocity)
	{
		mVelocity = velocity;
	}
}
