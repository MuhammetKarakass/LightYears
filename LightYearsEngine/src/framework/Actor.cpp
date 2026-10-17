#include "framework/Actor.h"
#include "framework/World.h"
#include "framework/MathUtility.h"
#include "framework/LightComponent.h"
#include "framework/SpriteComponent.h"
#include "framework/reflection/Reflection.h"

#include <algorithm>
#include <cmath>
#include <framework/debug/Log.h>

namespace ly
{
	void Actor::ForEachComponent(const std::function<void(ActorComponent&)>& visitor)
	{
		for (const auto& component : mComponents) visitor(*component);
	}

	void Actor::ForEachComponent(const std::function<void(const ActorComponent&)>& visitor) const
	{
		for (const auto& component : mComponents) visitor(*component);
	}

	Actor::Actor(World* OwningWorld, const std::string& TexturePath)
		: mContext{ OwningWorld ? &OwningWorld->GetEngineContext() : &EngineContext::Current() },
		mOwningWorld{ OwningWorld },
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

	bool Actor::RemoveComponent(ActorComponent& component)
	{
		if (GetIsPendingDestroy()) return false;
		const auto found = std::find_if(mComponents.begin(), mComponents.end(),
			[&](const auto& owned) { return owned.get() == &component; });
		if (found == mComponents.end()) return false;
		const auto& registry = GetEngineContext().Types();
		const auto* actorType = registry.FindMostDerived(*this);
		const auto* componentType = registry.Find(typeid(component));
		if (componentType && (!componentType->removable || (actorType && actorType->RequiresComponent(componentType->name)))) return false;
		component.EndPlayInternal();
		// Physics can be enabled before BeginPlay, including in the edit world.
		if (auto* collision = dynamic_cast<CollisionComponent*>(&component)) collision->UnInitializePhysics();
		// EndPlay may append components, invalidating vector iterators.
		const auto removal = std::find_if(mComponents.begin(), mComponents.end(),
			[&](const auto& owned) { return owned.get() == &component; });
		if (removal == mComponents.end()) return false;
		mComponents.erase(removal);
		mSprite = nullptr;
		mCollision = nullptr;
		mLights = nullptr;
		for (const auto& remaining : mComponents) CacheComponent(*remaining);
		if (mOwningWorld) mOwningWorld->RefreshActorSpatialQuery(*this);
		return true;
	}

	void Actor::CacheComponent(ActorComponent& component)
	{
		if (!mSprite) mSprite = dynamic_cast<SpriteComponent*>(&component);
		if (!mCollision) mCollision = dynamic_cast<CollisionComponent*>(&component);
		if (!mLights) mLights = dynamic_cast<LightComponent*>(&component);
	}
	
	Actor::~Actor()
	{
		// World teardown may release actors in any order, including external owners.
		Detach();
		while (!mChildren.empty()) mChildren.back()->Detach();
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
				// A callback may remove an earlier component. Continue after this
				// component's new position instead of skipping the next survivor.
				if (index >= mComponents.size() || mComponents[index].get() != &component)
				{
					const auto current = std::find_if(mComponents.begin(), mComponents.end(),
						[&](const auto& owned) { return owned.get() == &component; });
					index = static_cast<std::size_t>(std::distance(mComponents.begin(), current));
				}
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
		if (GetIsPendingDestroy() || mDestroying)
		{
			return;
		}

		mDestroying = true;
		while (!mChildren.empty())
		{
			Actor* child = mChildren.back();
			child->Detach();
			child->Destroy();
		}
		Detach();
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

	VEGA_REFLECT_ROOT(Actor)
	{
		VEGA_CATEGORY("Transform");
		VEGA_PROPERTY("Location", GetActorLocation, SetActorLocation).Speed(1.f);
		VEGA_PROPERTY("Rotation", GetActorRotation, SetActorRotation).Speed(0.5f);
		VEGA_READONLY("Velocity", GetVelocity);
		VEGA_CATEGORY("Rendering");
		VEGA_PROPERTY("RenderLayer", GetRenderLayer, SetRenderLayer)
			.Labels({ { 0, "Background" }, { 1, "GroundDecal" }, { 2, "World" }, { 3, "Projectile" }, { 4, "WorldVfx" }, { 5, "Foreground" } });
		VEGA_PROPERTY("Render Enabled", IsRenderEnabled, SetRenderEnabled);
		VEGA_CATEGORY("Tick");
		VEGA_PROPERTY("Tick When Paused", GetTickWhenPaused, SetTickWhenPaused);
	}
}
