#pragma once
#include "presentation/visual/ProceduralBackdrop.h"
#include <stdexcept>
#include <utility>

namespace vega
{
	// Test-only friend seam; no game, asset loader or editor calls these methods.
	class ProceduralBackdropTestAccess
	{
	public:
		using Body = ProceduralBackdrop::PlanetBodyDescription;
		using Pass = ProceduralBackdrop::PlanetBodyPass;
		using Observer = ProceduralBackdrop::PlanetBodyObserver;
		static void SetAdditionalPlanetBodies(ProceduralBackdrop& backdrop, std::vector<Body> bodies)
		{
			if (bodies.size() > 7) throw std::invalid_argument("Stage 1 supports at most seven additional bodies");
			backdrop.mAdditionalPlanetBodies = std::move(bodies);
		}
		static void Observe(ProceduralBackdrop& backdrop, Observer observer) { backdrop.mPlanetBodyObserver = std::move(observer); }
		static sf::Vector2f Center(const ProceduralBackdrop& backdrop) { return backdrop.mPlanetCenter; }
		static float Radius(const ProceduralBackdrop& backdrop) { return backdrop.mPlanetRadius; }
		static sf::Vector2f StarDirection(const ProceduralBackdrop& backdrop) { return backdrop.mStarDirection; }
		static bool HasShatteredMoon(const ProceduralBackdrop& backdrop) { return backdrop.mMoon.has_value(); }
	};
}
