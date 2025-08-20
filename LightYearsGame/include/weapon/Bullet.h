#pragma once

#include "framework/Actor.h"

namespace ly
{
	class Bullet : public Actor
	{
	public:
		Bullet(World* world, Actor* owner, const std::string& texturePath, float speed=600.f, float damage=10.f);

		void SetSpeed(float speed);
		void SetDamage(float damage);

		Actor* GetOwner() const { return mOwner; }

		virtual void BeginPlay() override;
		virtual void Tick(float deltaTime) override;

		virtual void OnActorBeginOverlap(Actor* otherActor) override;

		float GetDamage() { return mDamage; };
	private:
		void Move(float deltaTime);
		void SetupCollisionFromOwner();

		Actor* mOwner;
		float mSpeed;
		float mDamage;
	};
}