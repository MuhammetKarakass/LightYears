#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/scorchDrive/ScorchDrivePresentationProfile.h"

#include <SFML/Graphics/RectangleShape.hpp>

namespace ly
{
	class ScorchDriveTrailCoordinatorActor;

	class ScorchDriveFireSegmentActor final : public AbilityWorldActor
	{
	public:
		ScorchDriveFireSegmentActor(
			World* world,
			Actor* owner,
			const ScorchDrivePresentationProfile& presentationProfile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void ConfigureFromAttributes(const sas::GameplayAttributeList& attributes) override;

		void ConfigureSegment(
			const sf::Vector2f& location,
			const sf::Vector2f& direction,
			float lifetime,
			const weak_ptr<ScorchDriveTrailCoordinatorActor>& coordinator
		);

		const sf::Vector2f& GetSegmentDirection() const { return mDirection; }
		float GetSegmentWidth() const { return mWidth; }
		float GetSegmentLength() const { return mLength; }

	private:
		ScorchDrivePresentationProfile mPresentationProfile;
		sf::RectangleShape mOuter;
		sf::RectangleShape mCore;
		sf::Vector2f mDirection{ 0.f, -1.f };
		weak_ptr<ScorchDriveTrailCoordinatorActor> mCoordinator;
		float mWidth = 70.f;
		float mLength = 80.f;
		float mVisualAge = 0.f;
	};

	bool RegisterScorchDriveFireSegmentActorType();
}
