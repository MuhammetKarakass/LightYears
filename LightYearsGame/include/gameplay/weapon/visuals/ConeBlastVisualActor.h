#pragma once

#include "framework/Actor.h"

namespace ly
{
	// Presentation-only flash for an instant cone blast.
	class ConeBlastVisualActor final : public Actor
	{
	public:
		ConeBlastVisualActor(
			World* world,
			const sf::Vector2f& apex,
			const sf::Vector2f& direction,
			float range,
			float halfAngleDegrees,
			const sf::Color& color
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;

	private:
		void RebuildGeometry();

		sf::Vector2f mApex;
		sf::Vector2f mDirection;
		float mRange = 0.f;
		float mHalfAngleDegrees = 0.f;
		sf::Color mColor;
		sf::VertexArray mFan{ sf::PrimitiveType::TriangleFan };
		float mRemainingLifetime = 0.12f;
	};
}
