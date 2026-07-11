#pragma once

#include "framework/Actor.h"
#include "gameConfigs/AbilityVisualStructs.h"

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

	private:
		void UpdateVisuals();

		AreaTelegraphVisualDefinition mDefinition;
		sf::CircleShape mFill;
		sf::CircleShape mOutline;
		float mLifeTime = 0.f;
		float mAge = 0.f;
	};
}
