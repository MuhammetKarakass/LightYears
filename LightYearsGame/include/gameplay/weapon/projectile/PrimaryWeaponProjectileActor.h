#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameConfigs/combat/WeaponStructs.h"

#include <unordered_set>


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
		bool CanBeReflected() const override { return true; }

		virtual void OnActorBeginOverlap(Actor* otherActor) override;
		weak_ptr<AbilityWorldActor> SpawnRelayClone(
			const ProjectileRelayCloneRequest& request
		) const override;
		bool TryReflectProjectile(
			const ProjectileReflectionRequest& request
		) override;

		float GetDamage() const { return AbilityWorldActor::GetDamage(); };
		void SetLaunchVelocity(const sf::Vector2f& launchVelocity);
		void SetImpactBehavior(
			const shared_ptr<ProjectileImpactBehavior>& impactBehavior
		);
		bool IsEmpowered() const { return mShotMetadata.isEmpowered; }
		const PrimaryWeaponShotMetadata& GetShotMetadata() const { return mShotMetadata; }
		void SetShotMetadata(const PrimaryWeaponShotMetadata& metadata);
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
		PrimaryWeaponShotMetadata mShotMetadata;
		bool mHasLaunchVelocity = false;
		bool mImpactBehaviorCompleted = false;
		std::unordered_set<unsigned int> mProcessedImpactTargets;
	};
}
