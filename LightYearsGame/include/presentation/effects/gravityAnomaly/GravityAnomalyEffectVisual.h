#pragma once

#include "presentation/effects/GameplayEffectVisual.h"
#include "presentation/effects/gravityAnomaly/GravityAnomalyEffectVisualDefinition.h"

#include <SFML/Graphics/CircleShape.hpp>

namespace ly
{
	class GravityAnomalyEffectVisual final : public GameplayEffectVisual
	{
	public:
		GravityAnomalyEffectVisual(
			World* world,
			Actor* owner,
			const GravityAnomalyEffectVisualDefinition& definition
		);

		void Render(sf::RenderWindow& window) override;
		void SynchronizeState(const GameplayEffectVisualStateView& state) override;

	protected:
		void TickVisual(float deltaTime) override;

	private:
		GravityAnomalyEffectVisualDefinition mDefinition;
		sf::CircleShape mCore;
		sf::CircleShape mRing;
		float mAge = 0.f;
	};
}
