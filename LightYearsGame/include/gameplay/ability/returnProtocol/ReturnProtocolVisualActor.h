#pragma once

#include "framework/Actor.h"
#include "presentation/ability/returnProtocol/ReturnProtocolPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>

namespace ly
{
	class ReturnProtocolVisualActor final : public Actor
	{
	public:
		ReturnProtocolVisualActor(
			World* world,
			Actor* owner,
			const ReturnProtocolPresentationProfile& profile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;

	private:
		Actor* mOwner = nullptr;
		ReturnProtocolPresentationProfile mProfile;
		sf::CircleShape mOuterRing;
		sf::CircleShape mInnerRing;
		float mAge = 0.f;
	};
}
