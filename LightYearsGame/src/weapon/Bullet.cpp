#include "weapon/Bullet.h"

namespace ly
{
	Bullet::Bullet(World* world, Actor* actor, const std::string& texturePath, float speed, float damage)
		: Actor(world, texturePath), mActor(actor), mSpeed(speed), mDamage(damage)
	{
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
		LOG("Bullet BeginPlay");
	}
	void Bullet::Tick(float deltaTime)
	{
		Move(deltaTime);
		if (IsActorOutOfWindow())
		{
			Destroy();
		}
	}
	void Bullet::Move(float deltaTime)
	{
		if (mActor)
		{
			AddActorLocationOffset(GetActorRightDirection()*-1.f * mSpeed * deltaTime);
		}
	}
} // namespace ly