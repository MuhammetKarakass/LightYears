#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include <vector>

namespace vega
{
	class World;

	// A short-lived explosion flash in world units. The visual pipeline converts it to an output-pixel point light.
	struct ExplosionFlash
	{
		sf::Vector2f position;
		float radius = 0.f; // World units.
		sf::Color color = sf::Color::White;
		float intensity = 1.f;
		float age = 0.f;
		float lifetime = 0.35f;
		// The store is process-wide for cheap call sites, but a flash must only be
		// aged and submitted by the pipeline rendering the world that spawned it.
		const World* world = nullptr;
	};

	// Explosions add a flash here; the game visual pipeline ages it and submits it as a point light that fades out.
	class ExplosionLights
	{
	public:
		static void Add(const ExplosionFlash& flash);
		static void Update(float deltaSeconds, const World* world);
		// Only the flashes spawned in the rendered world; other worlds must not
		// reuse or age another world's light.
		static std::vector<ExplosionFlash> Active(const World* world);
		// Called when the pipeline binds to another world so no stale flash can
		// survive a world change or a destroyed world's address being reused.
		static void Clear();
	};
}
