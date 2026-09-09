#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/seismicCharge/SeismicChargePresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>

namespace ly
{
	class SeismicChargeBombActor final : public AbilityWorldActor
	{
	public:
		SeismicChargeBombActor(
			World* world,
			Actor* owner,
			const SeismicChargePresentationProfile& presentationProfile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;
		bool CanBeCapturedByRelay() const override { return false; }

	private:
		enum class Phase { Deployment, Fuse, Shockwave };

		void BeginShockwave();
		void ApplyShockwaveHits(float previousRadius, float currentRadius);
		void ConfigureCircle(sf::CircleShape& shape, float radius) const;

		SeismicChargePresentationProfile mPresentationProfile;
		Phase mPhase = Phase::Deployment;
		sf::Vector2f mDeploymentStart{};
		sf::Vector2f mDropLocation{};
		sf::Vector2f mDeploymentDirection{ 0.f, 1.f };
		sf::CircleShape mBombGlow;
		sf::CircleShape mBombCore;
		sf::CircleShape mMaximumRangeTelegraph;
		sf::CircleShape mShockwave;
		Set<Actor*> mHitActors;
		float mPhaseAge = 0.f;
		float mDropOffset = 0.f;
		float mDeploymentDuration = 0.f;
		float mFuseDuration = 0.f;
		float mShockwaveDuration = 0.f;
		float mShockwaveThickness = 0.f;
		float mMaximumRadius = 0.f;
		float mCurrentRadius = 0.f;
	};

	bool RegisterSeismicChargeBombActorType();
}
