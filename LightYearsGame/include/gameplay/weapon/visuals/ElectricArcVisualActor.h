#pragma once

#include "framework/Actor.h"

namespace ly
{
	// Presentation-only actor for a short-lived electric arc segment.
	class ElectricArcVisualActor final : public Actor
	{
	public:
		ElectricArcVisualActor(
			World* world,
			const sf::Vector2f& start,
			const sf::Vector2f& end,
			const sf::Color& color
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;

	private:
		void RebuildGeometry();

		sf::Vector2f mStart;
		sf::Vector2f mEnd;
		sf::Color mColor;
		sf::VertexArray mOuterBolt{ sf::PrimitiveType::LineStrip };
		sf::VertexArray mCoreBolt{ sf::PrimitiveType::LineStrip };
		float mAge = 0.f;
		float mRemainingLifetime = 0.10f;
	};
}
