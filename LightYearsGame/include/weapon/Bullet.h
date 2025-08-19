#pragma once

#include "framework/Actor.h"

namespace ly
{
	class Bullet : public Actor
	{
	public:
		Bullet(World* world, Actor* actor, const std::string& texturePath, float speed=600.f, float damage=10.f);

		void SetSpeed(float speed);
		void SetDamage(float damage);

		virtual void BeginPlay() override;
		virtual void Tick(float deltaTime) override;
	private:
		void Move(float deltaTime);

		Actor* mActor;
		float mSpeed;
		float mDamage;
	};
}