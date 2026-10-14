#pragma once

#include <visual/GalaxyModel.h>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace ly
{
	struct RoutePlanet
	{
		std::string name;
		std::optional<float> semiMajorAU, radiusEarth, massEarth, eqTempK;
		std::string method;
	};

	struct RouteStar
	{
		float temperatureK = 5772.f;
		float luminositySolar = 1.f;
		float radiusSolar = 1.f;
		float massSolar = 1.f;
		std::string spectralType;
		bool estimated = false;
	};

	struct RouteStop
	{
		std::string id;
		std::string name;
		bool real = true;
		float distancePc = 0.f;
		visual::galaxy::GalacticPosition position;
		RouteStar star;
		std::vector<RoutePlanet> planets;
	};

	// The campaign route: real exoplanet hosts from galaxy_route.json, then deterministic procedural stops to Sgr A*,
	// then the optional Galactic Centre tour from galaxy_centre_tour.json (fictional orbit showcase stops after Sgr A*).
	class GalaxyRoute
	{
	public:
		bool LoadFromFile(const std::filesystem::path& file, std::string& error);
		// Appends deterministic procedural stops from the last real stop to Sgr A*: count stops spaced along
		// visual::galaxy::RoutePosition, host temperature sampled from ComputeSky(...).stellarMix with std::mt19937(seed),
		// luminosity/radius from a main-sequence relation, planet count ~ Poisson-like with giant-planet chance
		// scaled by 10^(2·[Fe/H]) (planet–metallicity correlation, Fischer & Valenti 2005). The final stop is
		// "Sgr A*" (real = true, no planets, star fields describing the S2 star: 22 000 K, ~20 Msun, at 0.0001 kpc).
		void AppendProceduralStops(int count, unsigned seed);
		// Appends the stops of a file with the route's schema ({"version": 1, "stops": [...]}) after the current last
		// stop. These stops are past the route's end, so they are not ordered by radius. An id already in the route
		// is an error; on any error nothing is appended.
		bool AppendStopsFromFile(const std::filesystem::path& file, std::string& error);
		const std::vector<RouteStop>& Stops() const;
		// progress 0..1 over the whole route by galactocentric radius; returns the nearest stop index.
		std::size_t StopIndexAtProgress(float progress) const;

	private:
		std::vector<RouteStop> mStops;
	};
}
