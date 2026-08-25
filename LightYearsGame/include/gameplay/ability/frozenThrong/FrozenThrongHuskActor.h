#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/frozenThrong/FrozenThrongPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>

#include <optional>

namespace ly
{
	class FrozenThrongHuskActor final : public AbilityWorldActor
	{
	public:
		FrozenThrongHuskActor(
			World* world,
			Actor* owner,
			const FrozenThrongPresentationProfile& presentationProfile,
			std::optional<sf::Vector2f> targetLocation,
			weak_ptr<Actor> targetActor
		);

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void OnActorBeginOverlap(Actor* otherActor) override;
		bool IsProjectileActor() const override { return true; }
		void SetLaunchDelay(float delaySeconds);
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;

	private:
		void Move(float deltaTime);
		void Explode();
		sf::Vector2f ResolveTargetPosition() const;

		FrozenThrongPresentationProfile mPresentationProfile;
		weak_ptr<Actor> mTargetActor;
		std::optional<sf::Vector2f> mTargetLocation;
		sf::Vector2f mLaunchDirection{ 0.f, -1.f };
		float mProjectileSpeed = 1100.f;
		float mMaximumRange = 900.f;
		float mExplosionRadius = 120.f;
		float mTargetTravelDistance = 0.f;
		float mTravelDistance = 0.f;
		float mExplosionAge = 0.f;
		float mLaunchDelayRemaining = 0.f;
		bool mHasExploded = false;
	};

	bool RegisterFrozenThrongHuskActorType();
}
