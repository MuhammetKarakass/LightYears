#pragma once

#include "framework/Actor.h"

namespace ly
{
	// Presentation-only actor for an active continuous beam.
	class ContinuousBeamVisualActor final : public Actor
	{
	public:
		ContinuousBeamVisualActor(World* world, Actor* owner);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;

		void UpdateBeam(
			const sf::Vector2f& start,
			float directionRotation,
			float range,
			float width,
			float heatRatio,
			const sf::Color& baseColor
		);
		bool IsValidDamageTarget(const Actor* actor) const;

	private:
		Actor* mOwner;
		sf::RectangleShape mOuterBeam;
		sf::RectangleShape mCoreBeam;
	};
}
