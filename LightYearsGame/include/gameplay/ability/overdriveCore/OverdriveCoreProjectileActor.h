#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/overdriveCore/OverdriveCorePresentationProfile.h"

#include <optional>

namespace ly
{
	class AreaTelegraphActor;
	class OverdriveCoreVisualActor;

	class OverdriveCoreProjectileActor final : public AbilityWorldActor
	{
	public:
		OverdriveCoreProjectileActor(
			World* world,
			Actor* owner,
			const OverdriveCorePresentationProfile& presentationProfile,
			std::optional<sf::Vector2f> targetLocation,
			weak_ptr<Actor> targetActor
		);

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		bool IsProjectileActor() const override { return true; }
		void Render(sf::RenderWindow& window) override;
		void Destroy() override;
		void OnActorBeginOverlap(Actor* otherActor) override;
		void ConfigureFromAttributes(const sas::GameplayAttributeList& attributes) override;
		bool BuildRelaySnapshot(ProjectileRelaySnapshot& snapshot) const override;
		weak_ptr<AbilityWorldActor> SpawnRelayClone(
			const ProjectileRelayCloneRequest& request
		) const override;

	private:
		void Move(float deltaTime);
		void Explode();
		void SpawnPresentation();
		void SynchronizePresentation();
		void DestroyTelegraph();
		void DestroyFlightVisual();

		OverdriveCorePresentationProfile mPresentationProfile;
		weak_ptr<AreaTelegraphActor> mTelegraph;
		weak_ptr<OverdriveCoreVisualActor> mVisualActor;
		weak_ptr<Actor> mTargetActor;
		float mProjectileSpeed = 0.f;
		float mMaximumRange = 0.f;
		float mTargetTravelDistance = 0.f;
		float mExplosionRadius = 0.f;
		float mTravelDistance = 0.f;
		std::optional<sf::Vector2f> mTargetLocation;
		sf::Vector2f mLaunchVelocity{};
		bool mHasExploded = false;
	};

	bool RegisterOverdriveCoreProjectileActorType();
}
