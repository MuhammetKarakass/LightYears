#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/portal/PortalDestinationRebaser.h"
#include "presentation/ability/mineLayer/MineLayerPresentationProfile.h"

#include <cstddef>

namespace ly
{
	class MineLayerMineActor final
		: public AbilityWorldActor,
		  public PortalDestinationRebaser
	{
	public:
		MineLayerMineActor(
			World* world,
			Actor* owner,
			const MineLayerPresentationProfile& presentationProfile
		);

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;
		void BeginPortalTransit() override;
		void RebasePortalDestination(const sf::Vector2f& exitLocation) override;
		// The actor is spawned at its owner, then travels to its already-resolved
		// formation point. Keeping destination selection in MineLayerAbility lets
		// this actor remain responsible only for its projectile-like deployment.
		void LaunchTo(const sf::Vector2f& targetLocation);

		float GetTriggerRadius() const { return mTriggerRadius; }
		float GetExplosionRadius() const { return mExplosionRadius; }
		float GetStunDuration() const { return mStunDuration; }
		float GetKnockbackStrength() const { return mKnockbackStrength; }
		bool IsDeploying() const { return mIsDeploying; }
		bool HasTriggered() const { return mTriggered; }
		std::size_t GetAffectedTargetCount() const { return mAffectedTargetCount; }

	private:
		List<shared_ptr<Actor>> FindTargets(float radius) const;
		void MoveTowardDeploymentTarget(float deltaTime);
		void Trigger();
		void ApplyControl(Actor& target);
		void DrawIdle(sf::RenderWindow& window) const;
		void DrawExplosion(sf::RenderWindow& window) const;

		MineLayerPresentationProfile mPresentationProfile;
		float mTriggerRadius = 0.f;
		float mExplosionRadius = 0.f;
		float mLaunchSpeed = 0.f;
		float mStunDuration = 0.f;
		float mKnockbackStrength = 0.f;
		float mVisualAge = 0.f;
		float mExplosionAge = 0.f;
		sf::Vector2f mDeploymentTarget{};
		sf::Vector2f mLaunchVelocity{};
		float mPortalRemainingDeploymentDistance = 0.f;
		std::size_t mAffectedTargetCount = 0;
		bool mIsDeploying = false;
		bool mTriggered = false;
	};

	bool RegisterMineLayerMineActorType();
}
