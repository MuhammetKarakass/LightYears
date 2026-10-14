#include "gameplay/galaxy/GalaxyRoute.h"

#include "framework/JsonDocumentLoader.h"

#include <visual/Astro.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <set>
#include <stdexcept>
#include <utility>

namespace ly
{
	namespace
	{
		using Json = JsonDocumentLoader::Json;

		// Same tolerance as tools/galaxy/build_route.py (projection noise between consecutive stops).
		constexpr float RouteRadiusToleranceKpc = 0.25f;
		constexpr float SolarTemperatureK = 5772.f;
		constexpr float SolarRadiusAU = 0.00465f;
		constexpr float PlanetAlbedo = 0.3f;
		// Planet occurrence: giant-planet chance at [Fe/H] = 0 and the mean planet count per host.
		constexpr float GiantChanceAtSolar = 0.06f;
		constexpr float MeanPlanetsPerHost = 1.5f;
		constexpr std::size_t MaxProceduralPlanets = 8;

		// Main-sequence relations: L ~ M^4 (0.43 Msun and up), L ~ 0.23 M^2.3 below; R ~ M^0.8.
		float MainSequenceLuminosity(float mass)
		{
			return mass >= 0.43f ? std::pow(mass, 4.f) : 0.23f * std::pow(mass, 2.3f);
		}

		float MainSequenceRadius(float mass)
		{
			return std::pow(mass, 0.8f);
		}

		// Inverse of T = 5772 K * M^0.6 (L ~ M^4, R ~ M^0.8 with Stefan-Boltzmann).
		float MassFromTemperature(float temperatureK)
		{
			return std::pow(temperatureK / SolarTemperatureK, 1.f / 0.6f);
		}

		const char* SpectralClass(float temperatureK)
		{
			if (temperatureK >= 30000.f) return "O";
			if (temperatureK >= 10000.f) return "B";
			if (temperatureK >= 7500.f) return "A";
			if (temperatureK >= 6000.f) return "F";
			if (temperatureK >= 5200.f) return "G";
			if (temperatureK >= 3700.f) return "K";
			return "M";
		}

		RouteStar StarFromTemperature(float temperatureK)
		{
			RouteStar star;
			star.temperatureK = temperatureK;
			star.massSolar = MassFromTemperature(temperatureK);
			star.luminositySolar = MainSequenceLuminosity(star.massSolar);
			star.radiusSolar = MainSequenceRadius(star.massSolar);
			star.spectralType = SpectralClass(temperatureK);
			star.estimated = true;
			return star;
		}

		float DistanceFromSunPc(const visual::galaxy::GalacticPosition& position)
		{
			// Inverse of the frame in build_route.py: x = R0 - R cos(az), y = R sin(az) (heliocentric, kpc).
			const float sunRadius = visual::galaxy::SunRadiusKpc;
			const float x = sunRadius - position.radiusKpc * std::cos(position.azimuthRad);
			const float y = position.radiusKpc * std::sin(position.azimuthRad);
			const float dz = position.heightKpc - visual::galaxy::SunHeightKpc;
			return std::sqrt(x * x + y * y + dz * dz) * 1000.f;
		}

		std::vector<RoutePlanet> SamplePlanets(std::mt19937& rng, const std::string& hostName, const RouteStar& star, float metallicity)
		{
			std::uniform_real_distribution<float> uniform(0.f, 1.f);
			const float meanCount = MeanPlanetsPerHost;
			std::poisson_distribution<int> countDistribution(meanCount);
			const int count = std::min(countDistribution(rng), static_cast<int>(MaxProceduralPlanets));

			// Giant-planet chance grows as 10^(2·[Fe/H]) (Fischer & Valenti 2005), capped to stay plausible.
			const float giantChance = std::min(0.5f, GiantChanceAtSolar * std::pow(10.f, 2.f * metallicity));

			std::vector<RoutePlanet> planets;
			planets.reserve(static_cast<std::size_t>(count));
			for (int index = 0; index < count; ++index)
			{
				RoutePlanet planet;
				planet.name = hostName + " " + static_cast<char>('b' + index);
				const bool giant = uniform(rng) < giantChance;
				const float semiMajorAU = std::pow(10.f, -1.5f + 3.f * uniform(rng));
				const float radiusEarth = giant ? std::pow(10.f, 0.9f + 0.2f * uniform(rng)) : std::pow(10.f, -0.3f + 0.9f * uniform(rng));
				const float massEarth = giant ? std::pow(10.f, 1.5f + 1.5f * uniform(rng)) : std::pow(radiusEarth, 2.06f);
				// Equilibrium temperature from the host: T * sqrt(R / 2a) * (1 - A)^(1/4), R and a in AU.
				const float eqTempK = star.temperatureK * std::sqrt(star.radiusSolar * SolarRadiusAU / (2.f * semiMajorAU))
					* std::pow(1.f - PlanetAlbedo, 0.25f);
				planet.semiMajorAU = semiMajorAU;
				planet.radiusEarth = radiusEarth;
				planet.massEarth = massEarth;
				planet.eqTempK = eqTempK;
				planet.method = "Procedural";
				planets.push_back(std::move(planet));
			}
			return planets;
		}

		[[noreturn]] void Fail(const std::string& message)
		{
			throw std::runtime_error(message);
		}

		const Json& RequireField(const Json& object, const char* key, const std::string& context)
		{
			if (!object.is_object() || !object.contains(key))
			{
				Fail(context + "." + key + " is missing");
			}
			return object.at(key);
		}

		float ReadFloat(const Json& object, const char* key, const std::string& context)
		{
			const Json& value = RequireField(object, key, context);
			if (!value.is_number())
			{
				Fail(context + "." + key + " must be a number");
			}
			const float number = value.get<float>();
			if (!std::isfinite(number))
			{
				Fail(context + "." + key + " must be finite");
			}
			return number;
		}

		float ReadPositiveFloat(const Json& object, const char* key, const std::string& context)
		{
			const float number = ReadFloat(object, key, context);
			if (number <= 0.f)
			{
				Fail(context + "." + key + " must be positive");
			}
			return number;
		}

		// Missing or null fields are unknown.
		std::optional<float> ReadOptionalPositiveFloat(const Json& object, const char* key, const std::string& context)
		{
			if (!object.is_object() || !object.contains(key) || object.at(key).is_null())
			{
				return std::nullopt;
			}
			return ReadPositiveFloat(object, key, context);
		}

		std::string ReadString(const Json& object, const char* key, const std::string& context)
		{
			const Json& value = RequireField(object, key, context);
			if (!value.is_string())
			{
				Fail(context + "." + key + " must be a string");
			}
			return value.get<std::string>();
		}

		std::string ReadOptionalString(const Json& object, const char* key, const std::string& context)
		{
			if (!object.is_object() || !object.contains(key) || object.at(key).is_null())
			{
				return std::string();
			}
			return ReadString(object, key, context);
		}

		bool ReadOptionalBool(const Json& object, const char* key, bool fallback, const std::string& context)
		{
			if (!object.is_object() || !object.contains(key))
			{
				return fallback;
			}
			if (!object.at(key).is_boolean())
			{
				Fail(context + "." + key + " must be a boolean");
			}
			return object.at(key).get<bool>();
		}

		RoutePlanet ParsePlanet(const Json& json, const std::string& context)
		{
			if (!json.is_object())
			{
				Fail(context + " must be an object");
			}
			RoutePlanet planet;
			planet.name = ReadString(json, "name", context);
			if (planet.name.empty())
			{
				Fail(context + ".name must not be empty");
			}
			planet.semiMajorAU = ReadOptionalPositiveFloat(json, "semiMajorAU", context);
			planet.radiusEarth = ReadOptionalPositiveFloat(json, "radiusEarth", context);
			planet.massEarth = ReadOptionalPositiveFloat(json, "massEarth", context);
			planet.eqTempK = ReadOptionalPositiveFloat(json, "eqTempK", context);
			planet.method = ReadOptionalString(json, "method", context);
			return planet;
		}

		RouteStop ParseStop(const Json& json, const std::string& context)
		{
			if (!json.is_object())
			{
				Fail(context + " must be an object");
			}
			RouteStop stop;
			stop.id = ReadString(json, "id", context);
			if (stop.id.empty())
			{
				Fail(context + ".id must not be empty");
			}
			stop.name = ReadString(json, "name", context);
			stop.real = ReadOptionalBool(json, "real", true, context);
			stop.distancePc = ReadFloat(json, "distancePc", context);
			if (stop.distancePc < 0.f)
			{
				Fail(context + ".distancePc must not be negative");
			}
			stop.position.radiusKpc = ReadPositiveFloat(json, "radiusKpc", context);
			stop.position.heightKpc = ReadFloat(json, "heightKpc", context);
			stop.position.azimuthRad = ReadFloat(json, "azimuthRad", context);

			const Json& starJson = RequireField(json, "star", context);
			const std::string starContext = context + ".star";
			if (!starJson.is_object())
			{
				Fail(starContext + " must be an object");
			}
			stop.star.temperatureK = ReadPositiveFloat(starJson, "temperatureK", starContext);
			stop.star.luminositySolar = ReadPositiveFloat(starJson, "luminositySolar", starContext);
			stop.star.radiusSolar = ReadPositiveFloat(starJson, "radiusSolar", starContext);
			stop.star.massSolar = ReadPositiveFloat(starJson, "massSolar", starContext);
			stop.star.spectralType = ReadOptionalString(starJson, "spectralType", starContext);
			stop.star.estimated = ReadOptionalBool(starJson, "estimated", false, starContext);

			const Json& planetsJson = RequireField(json, "planets", context);
			if (!planetsJson.is_array())
			{
				Fail(context + ".planets must be an array");
			}
			for (std::size_t index = 0; index < planetsJson.size(); ++index)
			{
				stop.planets.push_back(ParsePlanet(planetsJson[index], context + ".planets[" + std::to_string(index) + "]"));
			}
			return stop;
		}

		// Checks the route envelope (object, version 1, non-empty stops array) and returns the stops array.
		const Json& RequireStopsArray(const Json& document)
		{
			if (!document.is_object())
			{
				Fail("root must be an object");
			}
			const Json& version = RequireField(document, "version", "root");
			if (!version.is_number_integer() || version.get<int>() != 1)
			{
				Fail("root.version must be 1");
			}
			const Json& stopsJson = RequireField(document, "stops", "root");
			if (!stopsJson.is_array() || stopsJson.empty())
			{
				Fail("root.stops must be a non-empty array");
			}
			return stopsJson;
		}

		std::vector<RouteStop> ParseRoute(const Json& document)
		{
			const Json& stopsJson = RequireStopsArray(document);

			std::vector<RouteStop> stops;
			stops.reserve(stopsJson.size());
			std::set<std::string> ids;
			for (std::size_t index = 0; index < stopsJson.size(); ++index)
			{
				const std::string context = "stops[" + std::to_string(index) + "]";
				RouteStop stop = ParseStop(stopsJson[index], context);
				if (!ids.insert(stop.id).second)
				{
					Fail(context + ".id '" + stop.id + "' is duplicated");
				}
				if (!stops.empty() && stop.position.radiusKpc > stops.back().position.radiusKpc + RouteRadiusToleranceKpc)
				{
					Fail(context + " lies further from the centre than the previous stop");
				}
				stops.push_back(std::move(stop));
			}
			return stops;
		}
	}

	bool GalaxyRoute::LoadFromFile(const std::filesystem::path& file, std::string& error)
	{
		error.clear();
		const JsonDocumentLoader::Result loaded = JsonDocumentLoader::LoadFromFile(file);
		if (!loaded.Succeeded())
		{
			error = loaded.error;
			return false;
		}
		try
		{
			mStops = ParseRoute(*loaded.document);
		}
		catch (const std::exception& exception)
		{
			error = "galaxy route " + file.string() + ": " + exception.what();
			return false;
		}
		return true;
	}

	bool GalaxyRoute::AppendStopsFromFile(const std::filesystem::path& file, std::string& error)
	{
		error.clear();
		const JsonDocumentLoader::Result loaded = JsonDocumentLoader::LoadFromFile(file);
		if (!loaded.Succeeded())
		{
			error = loaded.error;
			return false;
		}
		std::vector<RouteStop> appended;
		try
		{
			const Json& stopsJson = RequireStopsArray(*loaded.document);
			std::set<std::string> ids;
			for (const RouteStop& stop : mStops)
			{
				ids.insert(stop.id);
			}
			for (std::size_t index = 0; index < stopsJson.size(); ++index)
			{
				const std::string context = "stops[" + std::to_string(index) + "]";
				RouteStop stop = ParseStop(stopsJson[index], context);
				if (!ids.insert(stop.id).second)
				{
					Fail(context + ".id '" + stop.id + "' is already in the route");
				}
				appended.push_back(std::move(stop));
			}
		}
		catch (const std::exception& exception)
		{
			error = "galaxy stops " + file.string() + ": " + exception.what();
			return false;
		}
		mStops.insert(mStops.end(), appended.begin(), appended.end());
		return true;
	}

	void GalaxyRoute::AppendProceduralStops(int count, unsigned seed)
	{
		if (count <= 0)
		{
			return;
		}

		const float sunRadius = visual::galaxy::RoutePosition(0.f).radiusKpc;
		const float endRadius = visual::galaxy::RoutePosition(1.f).radiusKpc;
		float startProgress = 0.f;
		float startAzimuth = 0.f;
		if (!mStops.empty())
		{
			// Route already ends at Sgr A*: nothing left to add.
			const RouteStop& last = mStops.back();
			startProgress = std::clamp((sunRadius - last.position.radiusKpc) / (sunRadius - endRadius), 0.f, 1.f);
			startAzimuth = last.position.azimuthRad;
			if (startProgress >= 1.f)
			{
				return;
			}
		}

		std::mt19937 rng(seed);
		std::uniform_real_distribution<float> uniform(0.f, 1.f);
		const std::size_t firstIndex = mStops.size();
		for (int index = 1; index <= count; ++index)
		{
			const bool finalStop = index == count;
			const float q = static_cast<float>(index) / static_cast<float>(count);
			visual::galaxy::GalacticPosition position = visual::galaxy::RoutePosition(
				finalStop ? 1.f : startProgress + (1.f - startProgress) * q);
			position.azimuthRad = startAzimuth * (1.f - q);

			RouteStop stop;
			stop.id = "route-" + std::to_string(firstIndex + static_cast<std::size_t>(index));
			stop.position = position;
			stop.distancePc = DistanceFromSunPc(position);

			if (finalStop)
			{
				// S2-like star at Sgr A*: values from the route spec, luminosity and radius from the main sequence.
				stop.id = "sgr-a-star";
				stop.name = "Sgr A*";
				stop.real = true;
				stop.star.temperatureK = 22000.f;
				stop.star.massSolar = 20.f;
				stop.star.luminositySolar = MainSequenceLuminosity(stop.star.massSolar);
				stop.star.radiusSolar = MainSequenceRadius(stop.star.massSolar);
				stop.star.spectralType = "B";
				stop.star.estimated = true;
			}
			else
			{
				const visual::galaxy::SkyState sky = visual::galaxy::ComputeSky(position);
				const float uClass = uniform(rng);
				const float uTemp = uniform(rng);
				stop.star = StarFromTemperature(visual::astro::SampleStarTemperature(sky.stellarMix, uClass, uTemp));
				stop.name = "Procedural " + std::to_string(firstIndex + static_cast<std::size_t>(index));
				stop.real = false;
				stop.planets = SamplePlanets(rng, stop.name, stop.star, sky.metallicity);
			}
			mStops.push_back(std::move(stop));
		}
	}

	const std::vector<RouteStop>& GalaxyRoute::Stops() const
	{
		return mStops;
	}

	std::size_t GalaxyRoute::StopIndexAtProgress(float progress) const
	{
		if (mStops.empty())
		{
			return 0;
		}
		const float targetRadius = visual::galaxy::RoutePosition(progress).radiusKpc;
		std::size_t best = 0;
		float bestDistance = std::fabs(mStops[0].position.radiusKpc - targetRadius);
		for (std::size_t index = 1; index < mStops.size(); ++index)
		{
			const float distance = std::fabs(mStops[index].position.radiusKpc - targetRadius);
			if (distance < bestDistance)
			{
				bestDistance = distance;
				best = index;
			}
		}
		return best;
	}
}
