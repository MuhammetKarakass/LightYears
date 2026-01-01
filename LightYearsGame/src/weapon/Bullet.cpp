#include "weapon/Bullet.h"
#include "framework/Core.h"

namespace ly
{
	Bullet::Bullet(World* world, Actor* owner, const BulletDefinition& def)
		: Actor(world, def.texturePath), mOwner(owner), mSpeed(def.speed), mDamage(def.damage)
	{
		SetupCollisionFromOwner();
		AddLight(GameTags::Bullet::Laser_Red, def.pointLightDef, def.lightOffset);

	}

	void Bullet::SetSpeed(float speed)
	{
		mSpeed = speed;
		SetVelocity(GetActorForwardDirection() * mSpeed);
	}

	void Bullet::SetDamage(float damage)
	{
		mDamage = damage;
	}

	void Bullet::BeginPlay()
	{
		Actor::BeginPlay();
		SetEnablePhysics(true);
		
	}

	void Bullet::Tick(float deltaTime)
	{
		Move(deltaTime);
		if (IsActorOutOfWindow())
		{
			Destroy();
		}
		Actor::Tick(deltaTime);
	}

	void Bullet::OnActorBeginOverlap(Actor* otherActor)
	{
		Actor::OnActorBeginOverlap(otherActor);
		if(GetCanCollide())
		{
			otherActor->ApplyDamage(GetDamage());
			LOG("damage: %f", GetDamage());
			Destroy(); // Çarpýþma sonrasý mermi yok olur
		}
	}

	void Bullet::Move(float deltaTime)
	{
		if (mOwner)
		{
			SetVelocity(GetActorForwardDirection() * mSpeed);
			AddActorLocationOffset(GetActorForwardDirection() * mSpeed * deltaTime);
		}
	}

	void Bullet::SetupCollisionFromOwner()
	{
		/*if (!mOwner)
		{
			SetCollisionLayer(CollisionLayer::None);
			SetCollisionMask(CollisionLayer::None);
			return;
		}*/

		// OWNER'IN LAYER'INA GÖRE BULLET'IN COLLISION SETUP'I
		switch (mOwner->GetCollisionLayer())
		{
		case CollisionLayer::Player:
			// Player bullet: düþmanlarla çarpýþýr
			SetCollisionLayer(CollisionLayer::PlayerBullet);
			SetCollisionMask(CollisionLayer::Enemy | CollisionLayer::EnemyBullet);
			break;
		case CollisionLayer::Enemy:
			// Enemy bullet: player ile çarpýþýr
			SetCollisionLayer(CollisionLayer::EnemyBullet);
			SetCollisionMask(CollisionLayer::Player | CollisionLayer::PlayerBullet);
			break;
		default:
			SetCollisionLayer(CollisionLayer::None);
			SetCollisionMask(CollisionLayer::None);
			break;
		}
	}

}