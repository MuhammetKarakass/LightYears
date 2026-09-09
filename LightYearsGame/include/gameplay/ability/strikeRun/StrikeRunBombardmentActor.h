#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/strikeRun/StrikeRunPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>

#include <array>

namespace ly
{
	class StrikeRunBombardmentActor final : public AbilityWorldActor
	{
	public:
		StrikeRunBombardmentActor(
			World* world,
			Actor* owner,
			const StrikeRunPresentationProfile& presentationProfile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;

		void SetPreviewDirection(const sf::Vector2f& direction);
		const sf::Vector2f& GetDirection() const { return mDirection; }
		void Confirm();
		void CancelPreview();
		bool IsConfirmed() const { return mConfirmed; }
		int GetDetonatedImpactCount() const { return mDetonatedImpactCount; }

	private:
		enum class Phase
		{
			Preview,
			Telegraph,
			Impact,
			Finished
		};

		void RebuildImpactLocations();
		void DetonateImpact(int index);
		void RenderTelegraph(sf::RenderWindow& window, float pulse);
		void RenderExplosions(sf::RenderWindow& window) const;
		void RenderStrikeCraft(sf::RenderWindow& window);

		StrikeRunPresentationProfile mPresentationProfile;
		std::array<sf::Vector2f, 5> mImpactLocations{};
		std::array<float, 5> mExplosionAges{};
		std::array<bool, 5> mImpactDetonated{};
		sf::Vector2f mDirection{ 1.f, 0.f };
		float mExplosionRadius = 200.f;
		float mImpactSpan = 1400.f;
		int mImpactCount = 5;
		float mFinalTelegraphDuration = 0.8f;
		float mImpactDelay = 0.10f;
		float mPhaseElapsed = 0.f;
		int mDetonatedImpactCount = 0;
		Phase mPhase = Phase::Preview;
		bool mConfirmed = false;
		sf::RectangleShape mStrikeLine;
		sf::ConvexShape mStrikeCraft;
	};

	bool RegisterStrikeRunBombardmentActorType();
}
