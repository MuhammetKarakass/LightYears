#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameConfigs/WeaponStructs.h"


namespace ly
{
	class Bullet : public AbilityWorldActor
	{
	public:
		Bullet(World* world, Actor* owner, const WeaponPresentationDefinition& presentation, const GameplayAttributeList& values);

		Actor* GetOwner() const { return GetOwnerActor(); }

		virtual void BeginPlay() override;
		virtual void Tick(float deltaTime) override;

		virtual void OnActorBeginOverlap(Actor* otherActor) override;

		float GetDamage() const { return AbilityWorldActor::GetDamage(); };
		virtual void Destroy() override;
	private:
		void SetVisualScale(float scale);
		void Move(float deltaTime);
		void ApplyImpactDamage(Actor* directHitActor);
		void ApplyAreaDamage();

		float mSpeed;
		float mMaxTravelDistance;
		float mTravelDistance;
		float mAreaDamageRadius;
		float mVisualScale;
		int mRemainingPierces;
	};
}


