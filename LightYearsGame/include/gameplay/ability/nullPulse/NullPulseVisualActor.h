#pragma once

#include "presentation/ability/common/TimedPresentationActor.h"
#include "presentation/ability/nullPulse/NullPulsePresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>

namespace vega
{
	// A short-lived, non-colliding world visual for the instant pulse. The
	// ability owns the gameplay query; this actor only renders its result.
	class NullPulseVisualActor final : public TimedPresentationActor
	{
	public:
		NullPulseVisualActor(World* world, const sf::Vector2f& worldLocation, float radius, const NullPulsePresentationProfile& profile);

		void Render(sf::RenderTarget& window) override;

	private:
		void UpdateVisuals() override;

		NullPulsePresentationProfile mProfile;
		sf::CircleShape mPulseRing;
		sf::CircleShape mCore;
		float mRadius = 1.f;
	};
}
