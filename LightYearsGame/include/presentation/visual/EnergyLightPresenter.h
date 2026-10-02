#pragma once

#include <SFML/Graphics.hpp>
#include <visual/VisualAssets.h>
#include <visual/VisualPipeline.h>

namespace ly
{
	class World;

	// Submits energy glows and short trails to the visual pipeline from a read-only look at the world:
	// the player ship's engine, player projectiles (Player tier) and enemy projectiles (EnemyProjectile tier).
	// Stateless: nothing is retained between frames, so no actor identity or lifetime has to be tracked.
	class EnergyLightPresenter
	{
	public:
		void Submit(const World& world, const sf::View& worldView, visual::VisualAssets& assets, visual::VisualPipeline& pipeline) const;
	};
}
