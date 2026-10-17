#pragma once

#include "framework/Actor.h"
#include "presentation/ability/phaseDrift/PhaseDriftPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>

namespace vega
{
	class PhaseDriftVisualActor final : public Actor
	{
	public:
		PhaseDriftVisualActor(
			World* world,
			Actor* owner,
			const PhaseDriftPresentationProfile& profile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderTarget& window) override;

	private:
		void UpdateVisuals();

		Actor* mOwner = nullptr;
		PhaseDriftPresentationProfile mProfile;
		sf::CircleShape mAura;
		sf::CircleShape mOutline;
		float mAge = 0.f;
	};
}
