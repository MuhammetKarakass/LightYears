#pragma once

#include "framework/Actor.h"
#include "gameConfigs/GameplayStructs.h"


namespace ly
{
	class Bullet : public Actor
	{
	public:
		Bullet(World* world, Actor* owner, const WeaponPresentationDefinition& presentation, const PrimaryWeaponAttributes& attributes);

		void SetSpeed(float speed);
		void SetDamage(float damage);
		void SetVisualScale(float scale);
		void SetProjectileCollisionRadius(float radius);
		void SetProjectileAreaRadius(float radius);

		Actor* GetOwner() const { return mOwner; }

		virtual void BeginPlay() override;
		virtual void Tick(float deltaTime) override;

		virtual void OnActorBeginOverlap(Actor* otherActor) override;

		float GetDamage() { return mDamage; };
		virtual void Destroy() override;
	private:
		void Move(float deltaTime);
		void SetupCollisionFromOwner();
		void ApplyImpactDamage(Actor* directHitActor);
		void ApplyAreaDamage();
		bool IsValidAreaDamageTarget(const Actor* actor) const;

		Actor* mOwner;
		float mSpeed;
		float mDamage;
		float mLifeTime;
		float mAge;
		float mMaxTravelDistance;
		float mTravelDistance;
		float mAreaDamageRadius;
		float mCollisionRadius;
		float mVisualScale;
		int mRemainingPierces;
	};
}
