#pragma once

#include "framework/Actor.h"
#include "presentation/ability/shieldGraft/ShieldGraftPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>

namespace ly
{
	// A short-lived, non-colliding visual actor centered on the owner.
	// Renders a brief outer-to-inner converging visual effect that collapses
	// onto the owner ship, shifting from shield to health/graft tones.
	class ShieldGraftVisualActor final : public Actor
	{
	public:
		ShieldGraftVisualActor(
			World* world,
			Actor* owner,
			const ShieldGraftPresentationProfile& profile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;

	private:
		void UpdateVisuals();
		void OnOwnerDestroyed(Actor* destroyedActor);

		Actor* mOwner = nullptr;
		ShieldGraftPresentationProfile mProfile;
		sf::CircleShape mContractingRing;
		sf::CircleShape mInnerGlow;
		float mAge = 0.f;
	};
}
