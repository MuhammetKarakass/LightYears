#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/rocket/RocketPresentationProfile.h"

#include <optional>

namespace ly
{
	class AreaTelegraphActor;
	class RocketVisualActor;

	class RocketProjectileActor final : public AbilityWorldActor
	{
	public:
		RocketProjectileActor(
			World* world,
			Actor* owner,
			const RocketPresentationProfile& presentationProfile,
			std::optional<sf::Vector2f> targetLocation
		);

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		bool IsProjectileActor() const override { return true; }
		bool CanBeReflected() const override { return true; }
		void Render(sf::RenderWindow& window) override;
		void Destroy() override;
		void OnActorBeginOverlap(Actor* otherActor) override;
		void ConfigureFromAttributes(const sas::GameplayAttributeList& attributes) override;
		weak_ptr<AbilityWorldActor> SpawnRelayClone(
			const ProjectileRelayCloneRequest& request
		) const override;
		bool TryReflectProjectile(
			const ProjectileReflectionRequest& request
		) override;

		float GetProjectileSpeed() const { return mProjectileSpeed; }
		float GetMaximumRange() const { return mMaximumRange; }
		float GetTargetTravelDistance() const { return mTargetTravelDistance; }
		float GetExplosionRadius() const { return mExplosionRadius; }
		float GetTravelDistance() const { return mTravelDistance; }
		const sf::Vector2f& GetPredictedImpactLocation() const { return mPredictedImpactLocation; }
		bool HasTelegraph() const { return !mTelegraph.expired(); }
		bool HasVisualActor() const { return !mVisualActor.expired(); }

	private:
		void Move(float deltaTime);
		void Explode();
		void SpawnPresentation();
		void SynchronizePresentation();
		void DestroyTelegraph();
		void DestroyFlightVisual();

		RocketPresentationProfile mPresentationProfile;
		weak_ptr<AreaTelegraphActor> mTelegraph;
		weak_ptr<RocketVisualActor> mVisualActor;
		float mProjectileSpeed = 0.f;
		float mMaximumRange = 0.f;
		float mTargetTravelDistance = 0.f;
		float mExplosionRadius = 0.f;
		float mTravelDistance = 0.f;
		std::optional<sf::Vector2f> mTargetLocation;
		sf::Vector2f mLaunchVelocity{};
		sf::Vector2f mPredictedImpactLocation{};
		bool mHasExploded = false;
	};

	bool RegisterRocketProjectileActorType();
}
