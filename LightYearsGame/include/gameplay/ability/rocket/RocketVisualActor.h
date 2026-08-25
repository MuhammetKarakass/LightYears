#pragma once

#include "framework/Actor.h"
#include "presentation/ability/rocket/RocketPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>

namespace ly
{
	class RocketVisualActor final : public Actor
	{
	public:
		RocketVisualActor(World* world, const RocketPresentationProfile& profile);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;

		void SetFlightState(
			const sf::Vector2f& worldLocation,
			const sf::Vector2f& forwardDirection,
			float explosionRadius,
			float normalizedTravel
		);
		void BeginImpact(const sf::Vector2f& worldLocation, float explosionRadius);
		void SetFlightVisible(bool visible) { mFlightVisible = visible; }

		bool IsImpacting() const { return mPhase == Phase::Impact; }
		float GetImpactVisualDuration() const { return mDefinition.impactVisualDuration; }

	private:
		enum class Phase
		{
			Flight,
			Impact
		};

		void DrawFlight(sf::RenderWindow& window);
		void DrawImpact(sf::RenderWindow& window);
		void ConfigureTrailGeometry(float widthScale);

		RocketVisualDefinition mDefinition;
		ExplosionType mExplosionType = ExplosionType::Small;
		sf::ConvexShape mOuterTrail;
		sf::ConvexShape mCoreTrail;
		sf::CircleShape mFlightGlow;
		sf::CircleShape mImpactFlash;
		sf::CircleShape mImpactRing;
		sf::Vector2f mForwardDirection{ 0.f, -1.f };
		float mExplosionRadius = 1.f;
		float mNormalizedTravel = 0.f;
		float mPhaseAge = 0.f;
		Phase mPhase = Phase::Flight;
		bool mFlightVisible = true;
	};
}
