#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "gameplay/galaxy/GalaxyRoute.h"

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>

#ifndef LIGHT_YEARS_PROJECT_SOURCE_DIR
#define LIGHT_YEARS_PROJECT_SOURCE_DIR "."
#endif

namespace
{
	int gFailures = 0;

#define CHECK(condition) \
	do \
	{ \
		if (!(condition)) \
		{ \
			std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #condition "\n"; \
			++gFailures; \
		} \
	} while (false)

	bool SameStop(const vega::RouteStop& a, const vega::RouteStop& b)
	{
		if (a.id != b.id || a.name != b.name || a.real != b.real || a.distancePc != b.distancePc)
		{
			return false;
		}
		if (a.position.radiusKpc != b.position.radiusKpc || a.position.heightKpc != b.position.heightKpc
			|| a.position.azimuthRad != b.position.azimuthRad)
		{
			return false;
		}
		if (a.star.temperatureK != b.star.temperatureK || a.star.luminositySolar != b.star.luminositySolar
			|| a.star.radiusSolar != b.star.radiusSolar || a.star.massSolar != b.star.massSolar)
		{
			return false;
		}
		if (a.planets.size() != b.planets.size())
		{
			return false;
		}
		for (std::size_t index = 0; index < a.planets.size(); ++index)
		{
			const vega::RoutePlanet& pa = a.planets[index];
			const vega::RoutePlanet& pb = b.planets[index];
			if (pa.name != pb.name || pa.eqTempK != pb.eqTempK || pa.semiMajorAU != pb.semiMajorAU)
			{
				return false;
			}
		}
		return true;
	}
}

int main()
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	const std::filesystem::path root = LIGHT_YEARS_PROJECT_SOURCE_DIR;
	const std::filesystem::path file = root / "LightYearsGame" / "assets" / "content" / "data" / "galaxy_route.json";

	vega::GalaxyRoute route;
	std::string error;
	const bool loaded = route.LoadFromFile(file, error);
	CHECK(loaded);
	if (!loaded)
	{
		std::cerr << error << "\n";
		return 1;
	}

	const std::vector<vega::RouteStop>& stops = route.Stops();
	CHECK(stops.size() >= 10);
	if (stops.size() < 10)
	{
		return 1;
	}
	CHECK(stops.front().name == "Sol");
	CHECK(std::fabs(stops.front().position.radiusKpc - 8.2f) <= 0.05f);

	for (const vega::RouteStop& stop : stops)
	{
		CHECK(stop.position.radiusKpc > 0.f && stop.position.radiusKpc < 20.f);
		if (stop.real)
		{
			CHECK(stop.star.temperatureK >= 2000.f && stop.star.temperatureK <= 50000.f);
		}
	}

	// Missing file is reported, not thrown.
	{
		vega::GalaxyRoute missing;
		std::string missingError;
		CHECK(!missing.LoadFromFile(root / "does-not-exist-galaxy-route.json", missingError));
		CHECK(!missingError.empty());
	}

	// Procedural tail: 12 stops, ending at Sgr A*.
	vega::GalaxyRoute extended = route;
	const std::size_t realCount = stops.size();
	extended.AppendProceduralStops(12, 7);
	CHECK(extended.Stops().size() == realCount + 12);
	if (extended.Stops().size() == realCount + 12)
	{
		CHECK(extended.Stops().back().name == "Sgr A*");
		CHECK(extended.Stops().back().position.radiusKpc < 0.01f);
	}

	// Same seed on fresh copies gives identical stops.
	vega::GalaxyRoute first = route;
	vega::GalaxyRoute second = route;
	first.AppendProceduralStops(12, 7);
	second.AppendProceduralStops(12, 7);
	CHECK(first.Stops().size() == second.Stops().size());
	if (first.Stops().size() == second.Stops().size())
	{
		for (std::size_t index = 0; index < first.Stops().size(); ++index)
		{
			CHECK(SameStop(first.Stops()[index], second.Stops()[index]));
		}
	}

	// Galactic Centre tour: the game appends 14 procedural stops (same seed) before the tour, so Sgr A* ends the route.
	{
		vega::GalaxyRoute game = route;
		game.AppendProceduralStops(14, 7);
		const std::size_t proceduralEnd = game.Stops().size();
		CHECK(proceduralEnd == realCount + 14);
		const std::filesystem::path tourFile = root / "LightYearsGame" / "assets" / "content" / "data" / "galaxy_centre_tour.json";
		std::string tourError;
		CHECK(game.AppendStopsFromFile(tourFile, tourError));
		CHECK(tourError.empty());
		CHECK(game.Stops().size() == proceduralEnd + 9);
		if (game.Stops().size() == proceduralEnd + 9)
		{
			CHECK(game.Stops()[proceduralEnd - 1].id == "sgr-a-star");
			CHECK(game.Stops()[proceduralEnd].id == "gc-ringed-giant");
		}

		// The same file again repeats its ids, so nothing is appended.
		std::string duplicateError;
		CHECK(!game.AppendStopsFromFile(tourFile, duplicateError));
		CHECK(!duplicateError.empty());
		CHECK(game.Stops().size() == proceduralEnd + 9);
	}

	// Progress maps to the nearest stop by radius.
	CHECK(route.StopIndexAtProgress(0.f) == 0);
	CHECK(route.StopIndexAtProgress(1.f) == route.Stops().size() - 1);
	CHECK(extended.StopIndexAtProgress(1.f) == extended.Stops().size() - 1);

	if (gFailures == 0)
	{
		std::cout << "GalaxyRouteTests: all checks passed (" << stops.size() << " real stops)\n";
		return 0;
	}
	std::cerr << "GalaxyRouteTests: " << gFailures << " check(s) failed\n";
	return 1;
}
