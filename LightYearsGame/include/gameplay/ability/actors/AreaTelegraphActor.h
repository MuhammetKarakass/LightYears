#pragma once

#include "framework/Actor.h"
#include "presentation/ability/common/AreaTelegraphVisualDefinition.h"

#include <SFML/Graphics/CircleShape.hpp>

namespace ly
{
	// Reusable fixed-world warning for any circular delayed impact.
	class AreaTelegraphActor final : public Actor
	{
	public:
		AreaTelegraphActor(
			World* world,
			const sf::Vector2f& worldLocation,
			float radius,
			float lifeTime,
			const AreaTelegraphVisualDefinition& definition
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void SetCountdownProgress(float normalizedProgress);

	private:
		void UpdateVisuals();

		AreaTelegraphVisualDefinition mDefinition;
		sf::CircleShape mFill;
		sf::CircleShape mCountdownRing;
		sf::CircleShape mOutline;
		float mLifeTime = 0.f;
		float mAge = 0.f;
		float mCountdownProgress = 0.f;
		bool mHasExternalCountdown = false;
	};
}
