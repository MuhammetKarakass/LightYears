#pragma once

#include "framework/Actor.h"
#include "presentation/ability/nullPulse/NullPulsePresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>

namespace ly
{
	// A short-lived, non-colliding world visual for the instant pulse. The
	// ability owns the gameplay query; this actor only renders its result.
	class NullPulseVisualActor final : public Actor
	{
	public:
		NullPulseVisualActor(
			World* world,
			const sf::Vector2f& worldLocation,
			float radius,
			const NullPulsePresentationProfile& profile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;

	private:
		void UpdateVisuals();

		NullPulsePresentationProfile mProfile;
		sf::CircleShape mPulseRing;
		sf::CircleShape mCore;
		float mRadius = 1.f;
		float mAge = 0.f;
	};
}
