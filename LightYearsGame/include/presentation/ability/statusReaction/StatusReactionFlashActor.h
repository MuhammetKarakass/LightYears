#pragma once

#include "presentation/ability/common/TimedPresentationActor.h"
#include "presentation/ability/statusReaction/StatusReactionPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>

namespace vega
{
	// Non-colliding world flash for one status reaction; gameplay never reads it.
	class StatusReactionFlashActor final : public TimedPresentationActor
	{
	public:
		StatusReactionFlashActor(World* world, const sf::Vector2f& worldLocation, float radius, const StatusReactionPresentationProfile& profile);
		void Render(sf::RenderTarget& window) override;

		// Spawns the registered profile's flash; radius 0 uses the profile's default radius.
		static void Spawn(World& world, const sas::ContentId& profileId, const sf::Vector2f& location, float radius = 0.f);

	private:
		void UpdateVisuals() override;

		StatusReactionPresentationProfile mProfile;
		sf::CircleShape mRing;
		sf::CircleShape mCore;
		float mRadius = 1.f;
	};
}
