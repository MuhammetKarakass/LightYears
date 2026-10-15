#include "framework/reflection/Reflection.h"
#include "framework/Actor.h"
#include "framework/SpriteComponent.h"
#include "framework/LightComponent.h"
#include "framework/CollisionComponent.h"
#include "framework/SoundComponent.h"
#include "framework/scene/SceneEntityComponent.h"

namespace ly::reflect
{
	void RegisterEngineTypes()
	{
		static bool registered = false;
		if (registered) return;
		registered = true;
		Class<Actor>("Actor")
			.Property("Location", &Actor::GetActorLocation, &Actor::SetActorLocation).Speed(1.f).Category("Transform")
			.Property("Rotation", &Actor::GetActorRotation, &Actor::SetActorRotation).Speed(0.5f).Category("Transform")
			.Property("Velocity", &Actor::GetVelocity).Category("Transform")
			.Property("RenderLayer", &Actor::GetRenderLayer, &Actor::SetRenderLayer).Category("Rendering")
			.Labels({ { 0, "Background" }, { 1, "GroundDecal" }, { 2, "World" }, { 3, "Projectile" }, { 4, "WorldVfx" }, { 5, "Foreground" } })
			.Property("Render Enabled", &Actor::IsRenderEnabled, &Actor::SetRenderEnabled).Category("Rendering")
			.Property("Tick When Paused", &Actor::GetTickWhenPaused, &Actor::SetTickWhenPaused).Category("Tick");
		Class<ActorComponent>("ActorComponent")
			.Property("Active", &ActorComponent::IsActive, &ActorComponent::SetActive).Category("Lifecycle")
			.Property("Can Ever Tick", &ActorComponent::CanEverTick).Category("Lifecycle")
			.Property("Has Begun Play", &ActorComponent::HasBegunPlay).Category("Lifecycle");
		Class<SpriteComponent, ActorComponent>("SpriteComponent").Addable()
			.Property("Has Sprite", &SpriteComponent::HasSprite).Category("Sprite")
			.Property("Texture", &SpriteComponent::GetTexturePath,
				[](SpriteComponent& sprite, const std::string& path)
				{
					if (!path.empty() && !sprite.SetTexture(path)) throw std::runtime_error("Cannot load texture: " + path);
				}).Category("Sprite")
			.Property("Scale", &SpriteComponent::GetScale, &SpriteComponent::SetScale).Category("Sprite");
		Class<SceneEntityComponent, ActorComponent>("SceneEntityComponent")
			.Property("Name", &SceneEntityComponent::GetName, &SceneEntityComponent::SetName).Category("Scene")
			.Property("GUID", &SceneEntityComponent::GetGuid).Category("Scene");
		// Light operations need a tag; Phase 1 reflects scalar object properties only.
		Class<LightComponent, ActorComponent>("LightComponent").Addable();
		EnumLabels collisionLabels;
		const char* bits[]{ "Player", "PlayerBullet", "Enemy", "EnemyBullet", "Powerup", "Environment", "RelayProjectile", "FriendlySummon" };
		for (int bit = 0; bit < 8; ++bit) collisionLabels.emplace_back(1 << bit, bits[bit]);
		Class<CollisionComponent, ActorComponent>("CollisionComponent").Addable()
			.Property("Physics Enabled", &CollisionComponent::IsPhysicsEnabled, &CollisionComponent::SetEnablePhysics).Category("Physics")
			.Property("Has Physics Body", &CollisionComponent::HasPhysicsBody).Category("Physics")
			.Property("Body Type", &CollisionComponent::GetPhysicsBodyType, &CollisionComponent::SetPhysicsBodyType).Category("Physics")
			.Labels({ { 0, "Dynamic" }, { 1, "Static" } })
			.Property("Collision Layer", &CollisionComponent::GetCollisionLayer, &CollisionComponent::SetCollisionLayer).Flags(collisionLabels).Category("Collision")
			.Property("Collision Mask", &CollisionComponent::GetCollisionMask, &CollisionComponent::SetCollisionMask).Flags(collisionLabels).Category("Collision");
		// SoundComponent is independently owned, so it is registered for lookup by
		// name but does not participate in ActorComponent enumeration or discovery.
		Class<SoundComponent>("SoundComponent")
			.Property("Playing", &SoundComponent::IsPlaying)
			.Property("Stopped", &SoundComponent::IsStopped);
	}
}
