#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameConfigs/combat/WeaponStructs.h"


namespace ly
{
	class ProjectileImpactBehavior;

	class PrimaryWeaponProjectileActor : public AbilityWorldActor
	{
	public:
		PrimaryWeaponProjectileActor(
			World* world,
			Actor* owner,
			const WeaponPresentationDefinition& presentation,
			const sas::GameplayAttributeList& values
		);

		Actor* GetOwner() const { return GetOwnerActor(); }

		virtual void BeginPlay() override;
		virtual void Tick(float deltaTime) override;
		bool IsProjectileActor() const override { return true; }

		virtual void OnActorBeginOverlap(Actor* otherActor) override;
		weak_ptr<AbilityWorldActor> SpawnRelayClone(
			const ProjectileRelayCloneRequest& request
		) const override;

		float GetDamage() const { return AbilityWorldActor::GetDamage(); };
		void SetLaunchVelocity(const sf::Vector2f& launchVelocity);
		void SetImpactBehavior(
			const shared_ptr<ProjectileImpactBehavior>& impactBehavior
		);
		virtual void Destroy() override;
	private:
		void SetVisualScale(float scale);
		void Move(float deltaTime);
		void ApplyImpactDamage(Actor* directHitActor);
		void ApplyAreaDamage();

		float mSpeed;
		sf::Vector2f mLaunchVelocity;
		float mMaxTravelDistance;
		float mTravelDistance;
		float mAreaDamageRadius;
		float mVisualScale;
		int mRemainingPierces;
		WeaponPresentationDefinition mPresentationDefinition;
		shared_ptr<ProjectileImpactBehavior> mImpactBehavior;
		bool mHasLaunchVelocity = false;
		bool mImpactBehaviorCompleted = false;
	};
}
