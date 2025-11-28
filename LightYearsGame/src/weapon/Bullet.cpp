#include "weapon/Bullet.h"
#include "framework/Core.h"  // CollisionLayer için

namespace ly
{
	Bullet::Bullet(World* world, Actor* owner, const std::string& texturePath, float speed, float damage)
		: Actor(world, texturePath), mOwner(owner), mSpeed(speed), mDamage(damage)
	{
		// AUTOMATIC COLLISION SETUP - Owner'a göre collision layer/mask belirle
		SetupCollisionFromOwner();
	}

	void Bullet::SetSpeed(float speed)
	{
		mSpeed = speed;
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
	}

	void Bullet::OnActorBeginOverlap(Actor* otherActor)
	{
		Actor::OnActorBeginOverlap(otherActor);
		if(GetCanCollide())
		{
			otherActor->ApplyDamage(GetDamage());
			Destroy(); // Çarpýþma sonrasý mermi yok olur
		}
	}

	void Bullet::Move(float deltaTime)
	{
		if (mOwner)
		{
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

} // namespace ly