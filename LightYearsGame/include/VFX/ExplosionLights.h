#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include <vector>

namespace vega
{
	// A short-lived explosion flash in world units. The visual pipeline converts it to an output-pixel point light.
	struct ExplosionFlash
	{
		sf::Vector2f position;
		float radius = 0.f; // World units.
		sf::Color color = sf::Color::White;
		float intensity = 1.f;
		float age = 0.f;
		float lifetime = 0.35f;
	};

	// Explosions add a flash here; the game visual pipeline ages it and submits it as a point light that fades out.
	class ExplosionLights
	{
	public:
		static void Add(const ExplosionFlash& flash);
		static void Update(float deltaSeconds);
		static const std::vector<ExplosionFlash>& Active();
	};
}
