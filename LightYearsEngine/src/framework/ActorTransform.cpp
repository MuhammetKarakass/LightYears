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
	void Actor::SetActorLocation(const sf::Vector2f& newLoc)
	{
		mActorLocation = newLoc;
		mRelativeLocation = mParent ? (newLoc - mParent->mActorLocation).rotatedBy(sf::degrees(-mParent->mActorRotation)) : newLoc;
		SyncWorldTransform();
	}
	
	void Actor::SetActorRotation(float newRotation)
	{
		mActorRotation = newRotation;
		mRelativeRotation = newRotation - (mParent ? mParent->mActorRotation : 0.f);
		SyncWorldTransform();
	}

	bool Actor::IsDescendantOf(const Actor& ancestor) const
	{
		for (const Actor* parent = mParent; parent; parent = parent->mParent)
			if (parent == &ancestor) return true;
		return false;
	}

	bool Actor::AttachTo(Actor* parent, bool keepWorld)
	{
		if (GetIsPendingDestroy() || mDestroying || (parent && (parent == this || parent->IsDescendantOf(*this) ||
			parent->GetWorld() != GetWorld() || parent->GetIsPendingDestroy() || parent->mDestroying))) return false;
		if (parent && IsPhysicsEnabled() && GetPhysicsBodyType() == PhysicsBodyType::Dynamic)
		{
			debug::Log::Write(debug::LogLevel::Warning, debug::LogChannel::Scene, LY_SOURCE_LOCATION,
				"Cannot attach a dynamic physics actor; disable physics or use a static body first");
			return false;
		}
		if (parent == mParent) return true;
		if (mParent)
		{
			auto& siblings = mParent->mChildren;
			siblings.erase(std::find(siblings.begin(), siblings.end(), this));
		}
		mParent = parent;
		if (parent) parent->mChildren.push_back(this);
		if (keepWorld)
		{
			mRelativeLocation = parent ? (mActorLocation - parent->mActorLocation).rotatedBy(sf::degrees(-parent->mActorRotation)) : mActorLocation;
			mRelativeRotation = mActorRotation - (parent ? parent->mActorRotation : 0.f);
		}
		if (mCollision) mCollision->SetPhysicsBodyType(GetPhysicsBodyType());
		ResolveAttachmentTransform();
		return true;
	}

	void Actor::Detach()
	{
		if (!mParent) return;
		auto& siblings = mParent->mChildren;
		siblings.erase(std::find(siblings.begin(), siblings.end(), this));
		mParent = nullptr;
		mRelativeLocation = mActorLocation;
		mRelativeRotation = mActorRotation;
		if (mCollision) mCollision->SetPhysicsBodyType(GetPhysicsBodyType());
	}

	void Actor::SetRelativeLocation(const sf::Vector2f& location)
	{
		mRelativeLocation = location;
		ResolveAttachmentTransform();
	}

	void Actor::SetRelativeRotation(float rotation)
	{
		mRelativeRotation = rotation;
		ResolveAttachmentTransform();
	}

	void Actor::ResolveAttachmentTransform()
	{
		mActorLocation = mParent ? mParent->mActorLocation + mRelativeLocation.rotatedBy(sf::degrees(mParent->mActorRotation)) : mRelativeLocation;
		mActorRotation = mRelativeRotation + (mParent ? mParent->mActorRotation : 0.f);
		SyncWorldTransform();
	}

	void Actor::SyncWorldTransform()
	{
		if (mSprite) { mSprite->SyncLocation(); mSprite->SyncRotation(); }
		if (mCollision) mCollision->UpdatePhysicsTransform();
		if (mOwningWorld) mOwningWorld->RefreshActorSpatialQuery(*this);
		for (Actor* child : mChildren) child->ResolveAttachmentTransform();
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

}
