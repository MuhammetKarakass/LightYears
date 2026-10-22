#include "presentation/visual/SkyMapping.h"
#include "presentation/visual/ProceduralBackdrop.h"

#include "framework/World.h"
#include "gameplay/galaxy/GalaxyRoute.h"

#include <visual/Astro.h>
#include <visual/Backdrop.h>
#include <visual/LuminanceTier.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <string>

namespace vega
{
	namespace
	{
		// Z of the surface -> light direction: the in-plane part is the star's screen direction, this lifts it toward the viewer.
		constexpr float PlanetLightLift = 0.2f;
		constexpr float MoonOrbitPerPlanetRadius = 1.6f;
		// The backdrop planet turns 4x the pipeline's rotationSpeed, so its surface visibly moves.
		constexpr float PlanetSpinScale = 4.f;
		constexpr float IlluminatorTemperatureK = 15000.f;
		constexpr float IonizingMinK = 30000.f;
		constexpr float IonizingMaxK = 45000.f;
		// The emission nebula is a distant HII region, not the sky itself: only in the arms, and only in ~60% of them.
		constexpr float EmissionMinArmProximity = 0.45f;
		constexpr float EmissionChance = 0.6f;
		constexpr float DustMinDensity = 0.8f;
		constexpr float DustMinArmProximity = 0.5f;
		constexpr float DustChance = 0.5f;
		constexpr float PulsarMinProgress = 0.2f;
		constexpr float LensMinProgress = 0.999f;
		// The lens sits in the upper right, off the play-area centre, so the swirl stays out of the fight.
		constexpr float LensCenterX = 0.76f;
		constexpr float LensCenterY = 0.30f;
		constexpr int RouteProgressSamples = 200;
		constexpr const char* SgrAStarStopId = "sgr-a-star";
		// The orbit's angles are authored in degrees.
		constexpr float DegToRad = 0.01745329252f;
		// Orbit speed multipliers the dev keys step through; index 3 (x1) is the authored speed.
		constexpr float OrbitSpeedSteps[] = {0.f, 0.25f, 0.5f, 1.f, 2.f, 4.f, 8.f, 16.f, 32.f};

		// Deterministic per-location numbers (splitmix64), the same on every platform and standard library.
		class SeededRandom
		{
		public:
			explicit SeededRandom(std::uint64_t seed)
				: mState(seed)
			{
			}

			std::uint64_t NextU64()
			{
				mState += 0x9e3779b97f4a7c15ULL;
				std::uint64_t z = mState;
				z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
				z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
				return z ^ (z >> 31);
			}

			// Uniform in [0, 1).
			float Next01()
			{
				return static_cast<float>(NextU64() >> 40) / 16777216.f;
			}

			float Range(float low, float high)
			{
				return low + (high - low) * Next01();
			}

		private:
			std::uint64_t mState;
		};

		struct BackdropComposition
		{
			sf::Vector2f planetAt;
			sf::Vector2f starAt;
			float planetScale = 1.f;
			// The planet sits on the left instead of the configured right.
			bool mirrored = false;
			visual::MilkyWaySettings band;
		};

		BackdropComposition BuildBackdropComposition(const visual::PipelineSettings& settings, std::uint64_t seed)
		{
			SeededRandom random(seed ^ 0x8cb92baa3f3d8dd7ULL);
			const bool mirrored = random.Next01() < 0.5f;
			BackdropComposition composition;
			composition.mirrored = mirrored;
			composition.planetAt = {mirrored ? 1.f - settings.planet.screenX : settings.planet.screenX, settings.planet.screenY};
			composition.planetAt.x += random.Range(-0.025f, 0.025f);
			composition.planetAt.y += random.Range(-0.04f, 0.12f);
			composition.planetScale = random.Range(0.9f, 1.06f);
			composition.starAt = {settings.hostStar.screenX + random.Range(-0.18f, 0.18f), settings.hostStar.screenY + random.Range(0.f, 0.1f)};
			if (mirrored) composition.starAt.x = 1.f - composition.starAt.x;
			composition.band = settings.milkyWay;
			composition.band.angleDeg = (settings.milkyWay.angleDeg + random.Range(-14.f, 14.f)) * (mirrored ? -1.f : 1.f);
			composition.band.offset += random.Range(-0.14f, 0.14f);
			return composition;
		}

		// The composition with the stop's deep-sky overrides of the band and the planet applied.
		BackdropComposition BuildStopComposition(const visual::PipelineSettings& settings, std::uint64_t seed, const visual::DeepSkyStop* deepSky)
		{
			BackdropComposition composition = BuildBackdropComposition(settings, seed);
			if (deepSky == nullptr)
			{
				return composition;
			}
			const visual::DeepSkyBand& band = deepSky->band;
			if (band.intensityScale) composition.band.intensity *= *band.intensityScale;
			if (band.angleDeg) composition.band.angleDeg = *band.angleDeg * (composition.mirrored ? -1.f : 1.f);
			if (band.offset) composition.band.offset = *band.offset;
			if (band.width) composition.band.width = *band.width;
			if (band.dust) composition.band.dustLane *= *band.dust;
			const visual::DeepSkyPlanet& planet = deepSky->planet;
			if (planet.at)
			{
				composition.planetAt = {composition.mirrored ? 1.f - planet.at->x : planet.at->x, planet.at->y};
			}
			if (planet.scale) composition.planetScale *= *planet.scale;
			if (deepSky->hostStar.at)
			{
				composition.starAt = {composition.mirrored ? 1.f - deepSky->hostStar.at->x : deepSky->hostStar.at->x, deepSky->hostStar.at->y};
			}
			return composition;
		}

		std::uint64_t Mix(std::uint64_t hash, std::uint64_t value)
		{
			return SeededRandom(hash ^ (value + 0x9e3779b97f4a7c15ULL)).NextU64();
		}

		// FNV-1a over the stop id; std::hash is not stable across standard libraries.
		std::uint64_t HashText(const std::string& text)
		{
			std::uint64_t hash = 14695981039346656037ULL;
			for (const char c : text)
			{
				hash ^= static_cast<std::uint8_t>(c);
				hash *= 1099511628211ULL;
			}
			return hash;
		}

		std::uint64_t HashFloat(std::uint64_t hash, float value)
		{
			std::uint32_t bits = 0;
			std::memcpy(&bits, &value, sizeof bits);
			return Mix(hash, bits);
		}

		std::uint64_t HashPosition(const visual::galaxy::GalacticPosition& position)
		{
			return HashFloat(HashFloat(HashFloat(0x5bd1e995ULL, position.radiusKpc), position.heightKpc), position.azimuthRad);
		}

		// Space gradient colour: the region's shadow colour scaled by factor, so the sky stays near black.
		sf::Color ShadowColor(const visual::SkyPalette& palette, float factor)
		{
			auto channel = [factor](float v) { return static_cast<std::uint8_t>(std::clamp(v * factor, 0.f, 1.f) * 255.f); };
			return sf::Color(channel(palette.shadow.x), channel(palette.shadow.y), channel(palette.shadow.z));
		}

		// Output pixels per game unit, so the drift does not depend on the window size.
		float UnitsToPixels(float outputHeight, float viewHeight)
		{
			return viewHeight > 0.f ? outputHeight / viewHeight : 0.f;
		}

		// A backdrop body sits at a fraction of the output at the camera start and drifts against camera motion by parallax.
		sf::Vector2f BackdropCenter(sf::Vector2f output, float screenX, float screenY, sf::Vector2f cameraDrift, float parallax, float unitsToPixels)
		{
			return sf::Vector2f(output.x * screenX, output.y * screenY) - cameraDrift * (parallax * unitsToPixels);
		}

		// Display-space colour of a blackbody, 0..1 per channel.
		sf::Glsl::Vec3 BlackbodyDisplay(float temperatureK)
		{
			visual::astro::Rgb8 rgb = visual::astro::ToSrgb8(visual::astro::BlackbodyColor(temperatureK));
			return sf::Glsl::Vec3(rgb.r / 255.f, rgb.g / 255.f, rgb.b / 255.f);
		}

		// Two draws in a fixed order: unsequenced arguments would make the seeded sequence depend on the compiler.
		sf::Vector2f RandomPoint(SeededRandom& random, float low, float high, float lowY, float highY)
		{
			const float x = random.Range(low, high);
			const float y = random.Range(lowY, highY);
			return sf::Vector2f(x, y);
		}

		// Route progress of the sample nearest to a position (radius and height), so a stop gets its place on the route.
		float RouteProgressOf(const visual::galaxy::GalacticPosition& position)
		{
			float best = 0.f;
			float bestDistance = std::numeric_limits<float>::max();
			for (int i = 0; i <= RouteProgressSamples; ++i)
			{
				const float progress = static_cast<float>(i) / static_cast<float>(RouteProgressSamples);
				const visual::galaxy::GalacticPosition sample = visual::galaxy::RoutePosition(progress);
				const float dr = sample.radiusKpc - position.radiusKpc;
				const float dz = sample.heightKpc - position.heightKpc;
				const float distance = dr * dr + dz * dz;
				if (distance < bestDistance)
				{
					bestDistance = distance;
					best = progress;
				}
			}
			return best;
		}

		// Host star of a route stop. Its distance is the first planet's semi-major axis when known, else the temperate
		// distance where the star gives Earth-like irradiance (sqrt of its luminosity in AU).
		visual::HostStar HostStarOf(const RouteStop& stop)
		{
			visual::HostStar star;
			star.temperatureK = stop.star.temperatureK;
			star.luminositySolar = stop.star.luminositySolar;
			star.radiusSolar = stop.star.radiusSolar;
			star.distanceAU = std::sqrt(std::max(stop.star.luminositySolar, 1e-6f));
			if (!stop.planets.empty() && stop.planets.front().semiMajorAU.has_value() && *stop.planets.front().semiMajorAU > 0.f)
			{
				star.distanceAU = *stop.planets.front().semiMajorAU;
			}
			return star;
		}

		// Kind of one planet, from its radius and equilibrium temperature (ClassifyPlanet).
		visual::PlanetKind PlanetKindOf(const RoutePlanet& planet, unsigned seed)
		{
			return visual::ClassifyPlanet(planet.radiusEarth.value_or(0.f), planet.eqTempK.value_or(0.f), seed);
		}

		// 32-bit seed from a 64-bit location hash.
		unsigned FoldSeed(std::uint64_t seed)
		{
			return static_cast<unsigned>(seed ^ (seed >> 32));
		}

		// Deterministic per-entry surface and moon seeds for authored planet lists; entry 0 keeps the legacy seed.
		float EntrySurfaceSeed(std::uint64_t locationSeed, std::size_t index)
		{
			const std::uint64_t mixed = Mix(locationSeed, 0x51ed2701f3f5a2c7ULL + static_cast<std::uint64_t>(index));
			return static_cast<float>(mixed % 97u) + 1.f;
		}

		unsigned EntryMoonSeed(std::uint64_t locationSeed, std::size_t index)
		{
			return FoldSeed(Mix(locationSeed, 0x6d2b79f5ULL + static_cast<std::uint64_t>(index)));
		}

		// Authored drift is clamped this far outside the output, so a long-running body stays near the screen.
		constexpr float DriftScreenMargin = 0.25f;

		// Space gradient: the region's shadow colour near black; a stop's sky tint moves it toward the stop's own colour.
		void DrawSkyGradient(sf::RenderTarget& target, const visual::SkyPalette& palette, const visual::DeepSkyStop* deepSky, float cap)
		{
			visual::SkyPalette gradientPalette = palette;
			if (deepSky != nullptr && deepSky->skyTint)
			{
				// The stop's own sky colour, still near black: the tint is scaled to the shadow's brightness range.
				const float k = deepSky->skyTintStrength;
				const sf::Glsl::Vec3 tint = *deepSky->skyTint * 0.12f;
				gradientPalette.shadow = palette.shadow + (tint - palette.shadow) * k;
			}
			visual::DrawGradient(target, ShadowColor(gradientPalette, 0.6f), ShadowColor(gradientPalette, 1.f), cap);
		}

		// The Milky Way band's inputs for one stop: sky, band settings, region tint and the stop's band style.
		visual::GalaxyLayer::Params BandParams(const visual::galaxy::SkyState& sky, std::uint64_t seed, const visual::MilkyWaySettings& band, const visual::SkyPalette& palette, const visual::DeepSkyStop* deepSky, float cap)
		{
			visual::GalaxyLayer::Params params;
			params.sky = sky;
			params.tint = palette;
			params.settings = band;
			if (deepSky != nullptr)
			{
				params.style = deepSky->band.style;
			}
			params.seed = FoldSeed(seed);
			params.maxBrightness = cap;
			return params;
		}

		// The resolved star count of a stop: starScale multiplies the base and the maximum count.
		visual::StarFieldSettings StarFieldFor(const visual::StarFieldSettings& base, const visual::DeepSkyStop* deepSky)
		{
			visual::StarFieldSettings starField = base;
			if (deepSky != nullptr && deepSky->starScale)
			{
				starField.baseCount = static_cast<int>(std::lround(starField.baseCount * *deepSky->starScale));
				starField.maxCount = std::max(1, static_cast<int>(std::lround(starField.maxCount * *deepSky->starScale)));
			}
			return starField;
		}

		// Colour and strength of the light on a planet from the host star. Without a star only the faint, cool light of the sky falls on it.
		void SetStarLight(visual::PlanetParams& planet, const visual::HostStar& star, bool shown)
		{
			if (!shown)
			{
				planet.lightColor = sf::Glsl::Vec3(0.55f, 0.65f, 1.f);
				planet.lightIntensity = 0.15f;
				return;
			}
			const float irradiance = visual::HostStarIrradiance(star);
			planet.lightColor = BlackbodyDisplay(star.temperatureK);
			planet.lightIntensity = std::clamp(0.4f + 0.6f * (1.f - std::exp(-1.5f * irradiance)), 0.4f, 1.2f);
		}

		// Light reflected from the stop's planet (planetshine): the fill light comes from the planet's direction,
		// carries its surface colour and strengthens with its apparent size.
		void SetPlanetshineFill(visual::VisualPipeline& pipeline, const visual::ColorRamp& ramp, sf::Vector2f planetCentre, float planetRadius)
		{
			const sf::Vector2f output(pipeline.OutputSize());
			if (output.y <= 0.f || ramp.Empty())
			{
				return;
			}
			const sf::Color sample = ramp.Sample(0.7f);
			const float largest = std::max({ static_cast<float>(sample.r), static_cast<float>(sample.g), static_cast<float>(sample.b) });
			sf::Vector3f colour{ 1.f, 1.f, 1.f };
			if (largest > 0.f)
			{
				colour = { sample.r / largest, sample.g / largest, sample.b / largest };
			}
			// Half white, like the key light's white balance: the eye adapts to the reflected colour.
			const sf::Vector3f balanced{ 0.5f + 0.5f * colour.x, 0.5f + 0.5f * colour.y, 0.5f + 0.5f * colour.z };
			const float strength = 0.35f * std::clamp(planetRadius / output.y, 0.f, 1.f);
			pipeline.SetFillLight(sky::KeyLightFromStar(planetCentre, output * 0.5f, 10.f, balanced, strength));
		}
	}

	ProceduralBackdrop::ProceduralBackdrop()
		: mStars(1337)
		, mPosition(visual::galaxy::RoutePosition(0.f))
		, mSky(visual::galaxy::ComputeSky(mPosition))
	{
	}

	void ProceduralBackdrop::SetRouteStop(const RouteStop& stop)
	{
		const std::uint64_t seed = HashText(stop.id);
		if (seed == mLocationSeed)
		{
			return;
		}
		mLocationSeed = seed;
		mStopId = stop.id;
		mPosition = stop.position;
		mSky = visual::galaxy::ComputeSky(mPosition);
		mProcedural = !stop.real;
		mRouteProgress = RouteProgressOf(mPosition);
		mLensLocation = stop.id == SgrAStarStopId;
		mPlanetCount = stop.planets.size();
		mHasPlanet = !stop.planets.empty();
		mHostStar = HostStarOf(stop);
		// Planet 0 keeps the place's seed; the other planets take the following seeds.
		const unsigned folded = FoldSeed(seed);
		mPlanetKinds.clear();
		for (std::size_t i = 0; i < stop.planets.size(); ++i)
		{
			mPlanetKinds.push_back(PlanetKindOf(stop.planets[i], folded + static_cast<unsigned>(i)));
		}
		mPlanetKind = mPlanetKinds.empty() ? visual::PlanetKind::Terran : mPlanetKinds.front();
		mLocationChanged = true;
		mStarsDirty = true;
		mOrbit.Invalidate();
	}

	void ProceduralBackdrop::SetGalacticPosition(const visual::galaxy::GalacticPosition& position)
	{
		const std::uint64_t seed = HashPosition(position);
		if (seed == mLocationSeed)
		{
			return;
		}
		mLocationSeed = seed;
		mStopId.clear();
		mPosition = position;
		mSky = visual::galaxy::ComputeSky(mPosition);
		mProcedural = true;
		mRouteProgress = RouteProgressOf(mPosition);
		mLensLocation = mRouteProgress >= LensMinProgress;
		mPlanetCount = 0;
		mHasPlanet = true;
		mHostStar = visual::HostStar{};
		mPlanetKinds.clear();
		mPlanetKind = visual::ClassifyPlanet(0.f, 0.f, FoldSeed(seed));
		mLocationChanged = true;
		mStarsDirty = true;
		mOrbit.Invalidate();
	}

	ProceduralBackdrop::SkyGeometry ProceduralBackdrop::EditorGeometry(const visual::VisualAssets& assets, sf::Vector2u output) const
	{
		const auto* sky = assets.DeepSky().Find(SkyIdForRouteStop(mStopId));
		const auto composition = BuildStopComposition(assets.Settings(), mLocationSeed, sky);
		SkyGeometry result;
		if (output.x && output.y)
		{
			result.planetAt = {mPlanetCenter.x / output.x, mPlanetCenter.y / output.y};
			result.planetRadius = mPlanetRadius / output.y;
			const auto drift = (mLastCameraCenter - mCameraStart) * (UnitsToPixels(static_cast<float>(output.y), mAnchoredViewSize.y) * composition.band.parallax);
			const float margin = static_cast<float>(visual::GalaxyLayer::CacheMargin);
			result.bandDrift = {std::clamp(drift.x, -margin, margin) / output.x, std::clamp(drift.y, -margin, margin) / output.y};
		}
		result.planetBaseScale = BuildBackdropComposition(assets.Settings(), mLocationSeed).planetScale;
		result.band = composition.band;
		result.mirrored = composition.mirrored;
		result.hasPlanet = mHasPlanet && !(sky && sky->planet.hide);
		// Legacy-only skies keep the original accessor/handle. A mixed list includes its synthesized owner.
		if (sky && !sky->planetEntries.empty() && output.x && output.y)
		{
			const auto& settings = assets.Settings();
			const auto* sceneOrbit = sky->planets.front().orbit ? &*sky->planets.front().orbit : nullptr;
			EntryBodyContext context;
			context.output = sf::Vector2f(output);
			context.fallbackAt = composition.planetAt;
			context.baseRadius = output.y * settings.planet.radius * result.planetBaseScale;
			context.driftPixels = (mLastCameraCenter - mCameraStart) * UnitsToPixels(static_cast<float>(output.y), mAnchoredViewSize.y);
			context.planetParallax = settings.planet.parallax;
			context.bandParallax = composition.band.parallax;
			context.elapsedSeconds = std::max(mLastTime - mLocationStart, 0.f);
			context.pipelineRotationSpeed = settings.planet.rotationSpeed;
			context.mirrored = composition.mirrored;
			context.sceneOrbit = sceneOrbit;
			for (std::size_t i = 0; i < sky->planets.size(); ++i)
			{
				const auto& entry = sky->planets[i];
				const auto body = BuildEntryBody(entry, i, context);
				SkyPlanetGeometry geometry;
				// Legacy entry 0 has its own historical placement rules; its last drawn owner state is authoritative.
				const auto center = entry.legacy ? mPlanetCenter : body.center;
				const float radius = entry.legacy ? mPlanetRadius : body.radius;
				geometry.at = {center.x / output.x, center.y / output.y};
				geometry.radius = radius / output.y;
				geometry.baseScale = result.planetBaseScale;
				if (sceneOrbit && settings.planet.radius > 0.f)
				{
					// Resolve an unscaled body through the renderer's placement path, including per-entry framing.
					auto unscaled = entry;
					unscaled.planet.scale = 1.f;
					geometry.baseScale = BuildEntryBody(unscaled, i, context).radius / (output.y * settings.planet.radius);
				}
				geometry.index = i;
				geometry.hasBody = settings.planet.enabled && (!entry.legacy || mHasPlanet || (sceneOrbit && sceneOrbit->kind.has_value()));
				geometry.visible = !entry.planet.hide;
				result.planets.push_back(geometry);
			}
		}
		return result;
	}

	void ProceduralBackdrop::SetWarp(float targetBeta, sf::Vector2f direction)
	{
		mTargetBeta = std::clamp(targetBeta, 0.f, 0.99f);
		const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
		if (length > 0.f)
		{
			mDirection = direction * (1.f / length);
		}
	}

	void ProceduralBackdrop::Draw(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, const World& world, const sf::View& worldView, float timeSeconds)
	{
		const sf::Vector2f center = worldView.getCenter();
		mLastCameraCenter = center;
		// A new world, or its camera set up (the first frame still has the default view): parallax counts from here.
		if (mAnchoredWorld != &world || mAnchoredViewSize != worldView.getSize())
		{
			mAnchoredWorld = &world;
			mAnchoredViewSize = worldView.getSize();
			mCameraStart = center;
		}
		const visual::PipelineSettings& settings = assets.Settings();
		if (mLocationChanged)
		{
			mLocationChanged = false;
			mLocationStart = timeSeconds;
			mOrbitSeconds = 0.f;
			mOrbitClockTime = timeSeconds;
			mFeaturesDirty = true;
			// Same seed as the place, so its debris is the same every visit.
			mNearField.emplace(static_cast<unsigned>(mLocationSeed ^ (mLocationSeed >> 32)));
		}
		if (mFeaturesDirty || settings.physicalNebula != mBuiltNebula || settings.anomalies != mBuiltAnomalies)
		{
			RebuildFeatures(settings);
			mBuiltNebula = settings.physicalNebula;
			mBuiltAnomalies = settings.anomalies;
			mFeaturesDirty = false;
		}
		UpdateWarp(settings.warp, timeSeconds);
		DrawBackground(assets, pipeline, center, worldView.getSize().y, timeSeconds);
	}

	void ProceduralBackdrop::RebuildFeatures(const visual::PipelineSettings& settings)
	{
		const visual::NebulaSettings& nebula = settings.physicalNebula;
		const visual::AnomalySettings& anomaly = settings.anomalies;
		const float armProximity = mSky.armProximity;
		SeededRandom random(mLocationSeed);

		// The draws happen in a fixed order, so switching one feature off leaves the others unchanged.
		const sf::Vector2f emissionAt = RandomPoint(random, 0.2f, 0.8f, 0.15f, 0.6f);
		const float emissionTemperatureK = random.Range(IonizingMinK, IonizingMaxK);
		const float emissionSeed = random.Range(0.f, 100.f);
		const bool dustRoll = random.Next01() < DustChance;
		const sf::Vector2f dustAt = RandomPoint(random, 0.15f, 0.85f, 0.15f, 0.85f);
		const float dustAngle = random.Range(-1.5f, 1.5f);
		const float dustSeed = random.Range(0.f, 100.f);
		const sf::Vector2f strayAt = RandomPoint(random, 0.1f, 0.9f, 0.1f, 0.9f);
		const bool pulsarRoll = random.Next01() < anomaly.pulsarChance;
		const sf::Vector2f pulsarAt = RandomPoint(random, 0.1f, 0.9f, 0.1f, 0.9f);
		const float pulsarPeriod = random.Range(0.7f, 3.f);
		const bool moonRoll = random.Next01() < anomaly.shatteredMoonChance;
		const auto moonSeed = static_cast<std::uint32_t>(random.NextU64());
		// Drawn last, so the draws above keep their values when this roll changes.
		const bool emissionRoll = random.Next01() < EmissionChance;

		mEmission.present = nebula.enabled && armProximity > EmissionMinArmProximity && emissionRoll;
		mEmission.at = emissionAt;
		mEmission.temperatureK = emissionTemperatureK;
		mEmission.seed = emissionSeed;

		mDust.present = nebula.enabled && (mSky.dustDensity > DustMinDensity || armProximity > DustMinArmProximity) && dustRoll;
		mDust.at = dustAt;
		mDust.angleRad = dustAngle;
		mDust.seed = dustSeed;
		// The emission centre lights the dust when there is one, otherwise a stray point does.
		mDust.illuminatorAt = mEmission.present ? mEmission.at : strayAt;

		mPulsar.present = pulsarRoll && mProcedural && mRouteProgress > PulsarMinProgress;
		mPulsar.at = pulsarAt;
		mPulsar.periodSeconds = pulsarPeriod;

		if (moonRoll && mPlanetCount > 0)
		{
			mMoon.emplace(moonSeed);
		}
		else
		{
			mMoon.reset();
		}
	}

	void ProceduralBackdrop::UpdateWarp(const visual::WarpSettings& warp, float timeSeconds)
	{
		const float dt = std::clamp(timeSeconds - mLastTime, 0.f, 0.1f);
		mLastTime = timeSeconds;
		const float target = std::min(mTargetBeta, warp.maxBeta);
		// Exponential ease: about 98% of the way after rampSeconds, independent of the frame rate.
		const float rate = 4.f / std::max(warp.rampSeconds, 0.05f);
		mBeta += (target - mBeta) * (1.f - std::exp(-rate * dt));
	}

	void ProceduralBackdrop::DrawBackground(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f cameraCenter, float viewHeight, float timeSeconds)
	{
		using namespace visual;
		sf::RenderTarget& background = pipeline.Target(Layer::Background);
		sf::RenderTarget& nebulaTarget = pipeline.Target(Layer::Nebula);
		const PipelineSettings& settings = assets.Settings();
		const DeepSkyStop* deepSky = mStopId.empty() ? nullptr : assets.DeepSky().Find(SkyIdForRouteStop(mStopId));
		// A stop may raise the background ceiling and strengthen the grade for its own character.
		VisualPipeline::BackgroundLook look;
		if (deepSky != nullptr)
		{
			look.capScale = deepSky->capScale;
			look.contrast = deepSky->gradeContrast;
			look.saturation = deepSky->gradeSaturation;
		}
		pipeline.SetBackgroundLook(look);
		// The stop's whole-frame grade; without one the pipeline.json grade applies.
		pipeline.SetColorGrade(deepSky != nullptr ? deepSky->colorGrade : std::nullopt);
		// No visible star, a dim neutral light from above.
		mKeyLightLook = sky::KeyLightLookFor(deepSky);
		pipeline.SetKeyLight(sky::KeyLight{ {0.f, -0.57f, 0.82f}, {1.f, 1.f, 1.f}, 0.6f * mKeyLightLook.intensity });
		// Planetshine is off unless a visible owner planet turns it on while it is drawn below.
		pipeline.SetFillLight(sky::KeyLight{ {0.f, 0.f, 1.f}, {1.f, 1.f, 1.f}, 0.f });
		const float cap = std::min(settings.tierMaxBrightness[TierIndex(LuminanceTier::Background)] * look.capScale, 1.f);
		mOrbitRate.reset();
		// Only resolved entry 0 owns the scene. Legacy assets resolve to entry 0 as well; empty lists keep the generated fallback.
		sky::DeepSkyPlanetEntry synthesizedOwner;
		const sky::DeepSkyPlanetEntry* owner = deepSky != nullptr && !deepSky->planets.empty() ? &deepSky->planets.front() : nullptr;
		if (owner == nullptr && deepSky != nullptr && deepSky->orbit)
		{
			synthesizedOwner.planet = deepSky->planet;
			synthesizedOwner.orbit = deepSky->orbit;
			synthesizedOwner.legacy = true;
			owner = &synthesizedOwner;
		}
		const visual::OrbitSettings* sceneOrbit = owner != nullptr && owner->orbit ? &*owner->orbit : nullptr;
		if (sceneOrbit != nullptr)
		{
			DrawOrbitBackground(assets, pipeline, *deepSky, *owner, *sceneOrbit, cameraCenter, viewHeight, timeSeconds, cap);
			return;
		}
		const BackdropComposition composition = BuildStopComposition(settings, mLocationSeed, deepSky);

		RebuildStars(StarFieldFor(settings.starField, deepSky), composition.band, deepSky, composition.mirrored, sf::Vector2f(pipeline.OutputSize()), assets.DeepSky().Version());

		const sf::Vector2f output(pipeline.OutputSize());
		const float unitsToPixels = UnitsToPixels(output.y, viewHeight);
		const sf::Vector2f scrollPx = (cameraCenter - mCameraStart) * unitsToPixels;
		const float armProximity = mSky.armProximity;
		const NebulaSettings& nebula = settings.physicalNebula;

		// Region colour script: the gradient is the region's shadow colour, near black; the sky paint carries the colour.
		const SkyPalette palette = SkyPaletteFor(mSky);
		DrawSkyGradient(background, palette, deepSky, cap);

		// The Milky Way, rendered once per location and added with parallax; resolved stars go over it.
		GalaxyLayer::Params galaxyParams = BandParams(mSky, mLocationSeed, composition.band, palette, deepSky, cap);

		// The stop's nebulae and haze are rendered first and added inside the band, so its dust lanes cross them; dark
		// clouds then dim everything behind them.
		DeepSkyLayer::Params deepSkyParams;
		deepSkyParams.sky = deepSky;
		deepSkyParams.skyId = SkyIdForRouteStop(mStopId);
		deepSkyParams.catalogVersion = assets.DeepSky().Version();
		deepSkyParams.maxBrightness = cap;
		deepSkyParams.mirrored = composition.mirrored;
		mDeepSky.Prepare(assets.Shaders(), background.getSize(), deepSkyParams);
		galaxyParams.extraLight = mDeepSky.Light();
		galaxyParams.extraRevision = mDeepSky.Revision();
		mGalaxy.Draw(assets.Shaders(), background, galaxyParams, scrollPx);
		mDeepSky.DrawShade(background, scrollPx, composition.band.parallax);

		// Stars right after the gradient: the opaque planet hides the stars behind it, and the clouds sit over distant stars.
		mStars.SetMotion(mBeta, mDirection, settings.warp.fieldOfViewRad);
		mStars.Draw(background, scrollPx, timeSeconds, cap);

		// The host star and the planet are drawn over the stars and under the clouds (art direction).
		DrawHostStar(assets, pipeline, cameraCenter, viewHeight, cap);

		// An optional dim comet (anomalies.cometChance) with its tails pointing away from the host star.
		{
			SeededRandom cometRandom(mLocationSeed ^ 0x9e3779b97f4a7c15ull);
			const bool hasComet = cometRandom.Next01() < settings.anomalies.cometChance;
			const sf::Vector2f headAt = RandomPoint(cometRandom, 0.08f, 0.32f, 0.12f, 0.4f);
			const float bend = cometRandom.Range(-1.f, 1.f);
			const float lengthScale = cometRandom.Range(0.12f, 0.22f);
			if (hasComet)
			{
				const HostStarSettings& hostSettings = settings.hostStar;
				CometParams comet;
				comet.head = BackdropCenter(output, headAt.x, headAt.y, cameraCenter - mCameraStart, hostSettings.parallax * 2.f, unitsToPixels);
				comet.starAt = BackdropCenter(output, composition.starAt.x, composition.starAt.y, cameraCenter - mCameraStart, hostSettings.parallax, unitsToPixels);
				comet.length = lengthScale * output.y;
				comet.width = 0.009f * output.y;
				comet.bend = bend;
				comet.timeSeconds = timeSeconds;
				comet.maxBrightness = cap * 0.45f;
				visual::DrawComet(assets.Shaders(), background, comet);
			}
		}
		if (DrawPlanet(assets, pipeline, cameraCenter, viewHeight, timeSeconds) && mMoon)
		{
			ShatteredMoon::Params moon;
			moon.planetCenter = mPlanetCenter;
			moon.orbitRadius = MoonOrbitPerPlanetRadius * mPlanetRadius;
			moon.ageSeconds = timeSeconds - mLocationStart;
			moon.lightDirection = mStarDirection;
			moon.maxBrightness = cap;
			mMoon->Draw(background, moon);
		}
		DrawAdditionalPlanetBodies(assets, pipeline, cameraCenter, viewHeight, timeSeconds, nullptr);

		// Lit cloud layers, lit by the host star where it is now (the same position the host star and planet use).
		const HostStarSettings& host = settings.hostStar;
		const sf::Vector2f hostCenter = BackdropCenter(output, composition.starAt.x, composition.starAt.y, cameraCenter - mCameraStart, host.parallax, unitsToPixels);
		// The ramp-coloured full-screen clouds the game opened with: the ship flies inside them at this stop.
		if (deepSky != nullptr)
		{
			for (const DeepSkyNebulaLayer& layer : deepSky->nebula)
			{
				std::vector<ColorStop> stops;
				for (std::size_t k = 0; k < layer.ramp.size(); ++k)
				{
					const sf::Glsl::Vec3& c = layer.ramp[k];
					stops.push_back({static_cast<float>(k) / 3.f, sf::Color(static_cast<std::uint8_t>(c.x * 255.f), static_cast<std::uint8_t>(c.y * 255.f), static_cast<std::uint8_t>(c.z * 255.f))});
				}
				NebulaParams cloud;
				cloud.scroll = scrollPx * layer.parallax + sf::Vector2f(400.f, 150.f) * layer.seed;
				cloud.scale = 0.0021f / layer.scale;
				cloud.intensity = layer.intensity;
				cloud.seed = layer.seed;
				cloud.maxBrightness = cap;
				DrawNebula(assets.Shaders(), nebulaTarget, ColorRamp(std::move(stops)), cloud);
			}
		}

		const SkyPaintSettings& skyPaint = settings.skyPaint;
		const DeepSkyPaint* stopPaint = deepSky != nullptr && deepSky->skyPaint.enabled ? &deepSky->skyPaint : nullptr;
		if (skyPaint.enabled || stopPaint != nullptr)
		{
			SkyPaintParams paint;
			paint.scroll = scrollPx;
			paint.lightPos = hostCenter;
			paint.palette = palette;
			paint.coverage = skyPaint.coverage;
			paint.intensity = skyPaint.intensity;
			paint.centerCalm = skyPaint.centerCalm;
			if (stopPaint != nullptr)
			{
				// The stop flies through its own cloud: its colours and coverage replace the region's.
				paint.coverage = stopPaint->coverage;
				paint.intensity = stopPaint->intensity;
				paint.centerCalm = stopPaint->centerCalm;
				if (stopPaint->shadow) paint.palette.shadow = *stopPaint->shadow;
				if (stopPaint->mid) paint.palette.mid = *stopPaint->mid;
				if (stopPaint->lit) paint.palette.lit = *stopPaint->lit;
				if (stopPaint->accent) paint.palette.accent = *stopPaint->accent;
			}
			paint.timeSeconds = timeSeconds;
			paint.seed = static_cast<float>(mLocationSeed % 97u);
			paint.maxBrightness = cap;
			paint.farParallax = skyPaint.farParallax;
			paint.midParallax = skyPaint.midParallax;
			paint.nearParallax = skyPaint.nearParallax;
			DrawSkyPaint(assets.Shaders(), nebulaTarget, paint);
		}


		if (nebula.enabled && mEmission.present)
		{
			EmissionNebulaParams emission;
			emission.center = sf::Vector2f(output.x * mEmission.at.x, output.y * mEmission.at.y);
			emission.radius = nebula.emissionRadius * output.y * (0.6f + 0.8f * armProximity);
			emission.scroll = scrollPx;
			emission.parallax = nebula.parallax;
			emission.ionizingTemperatureK = mEmission.temperatureK;
			emission.density = nebula.emissionDensity * (0.5f + armProximity);
			emission.seed = mEmission.seed;
			emission.palette = nebula.palette;
			emission.saturation = nebula.saturation;
			emission.timeSeconds = timeSeconds;
			emission.maxBrightness = cap;
			visual::DrawEmissionNebula(assets.Shaders(), nebulaTarget, emission);
		}
		pipeline.CompositeNebula();

		if (nebula.enabled && mDust.present)
		{
			DustCloudParams dust;
			dust.center = sf::Vector2f(output.x * mDust.at.x, output.y * mDust.at.y);
			dust.size = sf::Vector2f(nebula.dustSize * output.y * 1.2f, nebula.dustSize * output.y * 0.55f);
			dust.angleRad = mDust.angleRad;
			dust.scroll = scrollPx;
			dust.parallax = nebula.parallax;
			dust.opticalDepth = nebula.dustOpticalDepth * std::clamp(mSky.dustDensity, 0.5f, 2.f);
			dust.reflection = nebula.reflection * armProximity;
			dust.illuminatorTemperatureK = IlluminatorTemperatureK;
			dust.illuminator = sf::Vector2f(output.x * mDust.illuminatorAt.x, output.y * mDust.illuminatorAt.y);
			dust.seed = mDust.seed;
			dust.maxBrightness = cap;
			dust.stylize = settings.backgroundStyle.enabled;
			visual::DrawDustCloud(assets.Shaders(), background, dust);
		}

		// Inside an asteroid belt the rocks pass in front of the clouds and the planet, the nearest close to the ship.
		if (deepSky != nullptr && deepSky->asteroids)
		{
			if (mAsteroidsStop != mStopId || mAsteroidsBuilt != *deepSky->asteroids)
			{
				mAsteroids.Build(FoldSeed(mLocationSeed), *deepSky->asteroids);
				mAsteroidsStop = mStopId;
				mAsteroidsBuilt = *deepSky->asteroids;
			}
			AsteroidField::Params rocks;
			rocks.scroll = scrollPx;
			rocks.timeSeconds = timeSeconds - mLocationStart;
			rocks.lightDirection = mStarDirection;
			rocks.maxBrightness = cap;
			rocks.mirrored = composition.mirrored;
			mAsteroids.Draw(assets.Shaders(), background, rocks);
		}

		if (mPulsar.present)
		{
			PulsarParams pulsar;
			pulsar.center = BackdropCenter(output, mPulsar.at.x, mPulsar.at.y, cameraCenter - mCameraStart, nebula.parallax, unitsToPixels);
			pulsar.periodSeconds = mPulsar.periodSeconds;
			pulsar.beamLength = 0.4f * output.y;
			pulsar.nebulaRadius = 0.03f * output.y;
			pulsar.timeSeconds = timeSeconds;
			pulsar.maxBrightness = cap;
			visual::DrawPulsar(assets.Shaders(), background, pulsar);
		}

		DrawMotes(assets, pipeline, deepSky, scrollPx, timeSeconds, cap);

		LensParams lens;
		lens.enabled = mLensLocation;
		lens.center = sf::Vector2f(output.x * LensCenterX, output.y * LensCenterY);
		lens.einsteinRadius = settings.anomalies.lensEinsteinRadius * output.y;
		lens.shadowRadius = settings.anomalies.lensShadowRadius * output.y;
		lens.ringBrightness = settings.anomalies.lensRingBrightness;
		pipeline.SetLens(lens);
	}

	void ProceduralBackdrop::DrawHostStar(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f cameraCenter, float viewHeight, float maxBrightness)
	{
		const visual::HostStarSettings& settings = assets.Settings().hostStar;
		if (!settings.enabled || viewHeight <= 0.f)
		{
			return;
		}
		const sf::Vector2f output(pipeline.OutputSize());
		const float unitsToPixels = UnitsToPixels(output.y, viewHeight);
		const visual::DeepSkyStop* deepSky = mStopId.empty() ? nullptr : assets.DeepSky().Find(SkyIdForRouteStop(mStopId));
		if (deepSky != nullptr && deepSky->hostStar.hide)
		{
			return;
		}
		const BackdropComposition composition = BuildStopComposition(assets.Settings(), mLocationSeed, deepSky);
		DrawHostStarAt(assets, pipeline, BackdropCenter(output, composition.starAt.x, composition.starAt.y, cameraCenter - mCameraStart, settings.parallax, unitsToPixels), maxBrightness, std::nullopt);
	}

	void ProceduralBackdrop::DrawHostStarAt(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f center, float maxBrightness, std::optional<float> radius)
	{
		const visual::HostStarSettings& settings = assets.Settings().hostStar;
		const sf::Vector2f output(pipeline.OutputSize());
		const float irradiance = visual::HostStarIrradiance(mHostStar);
		visual::HostStarParams star;
		star.center = center;
		// min/max rather than clamp: the bounds would invert on a degenerate output height
		star.radius = std::min(std::max(visual::HostStarApparentRadius(mHostStar, settings.radiusAt1AU * output.y), 1.5f), 0.018f * output.y);
		star.glowRadius = std::clamp(settings.glowRadius * output.y * std::sqrt(irradiance), 0.03f * output.y, 0.3f * output.y);
		if (radius)
		{
			// An orbit stop shows the star at its physical apparent size, with the halo kept outside the disc.
			star.radius = *radius;
			star.glowRadius = std::max(star.glowRadius, *radius * 1.8f);
		}
		star.color = BlackbodyDisplay(mHostStar.temperatureK);
		star.intensity = std::max(0.15f, settings.intensity * (1.f - std::exp(-1.5f * irradiance)));
		star.maxBrightness = maxBrightness;
		visual::DrawHostStar(assets.Shaders(), pipeline, star);
		// The drawn host star is the scene's key light: direction from the screen centre, colour from the star.
		// Half white balance: eyes and cameras adapt to the light's colour, so a white hull under an orange star reads warm,
		// not orange. The star's full colour stays in its disc and glare.
		const sf::Vector3f balanced{0.5f + 0.5f * star.color.x, 0.5f + 0.5f * star.color.y, 0.5f + 0.5f * star.color.z};
		pipeline.SetKeyLight(sky::KeyLightFromStar(center, output * 0.5f, mKeyLightLook.elevation, balanced, star.intensity * mKeyLightLook.intensity));
	}

	void ProceduralBackdrop::RebuildStars(const visual::StarFieldSettings& starField, const visual::MilkyWaySettings& band, const visual::DeepSkyStop* clusters, bool mirrored, sf::Vector2f outputSize, unsigned catalogVersion)
	{
		if (!(mStarsDirty || starField != mBuiltStars || band != mBuiltMilkyWay || mBuiltStarsCatalog != catalogVersion || mBuiltStarsStop != mStopId))
		{
			return;
		}
		mStars.Build(mSky, starField, band);
		// The stop's star clusters join the star field, so they are drawn like every other star.
		if (clusters != nullptr)
		{
			const float aspect = outputSize.y > 0.f ? outputSize.x / outputSize.y : 16.f / 9.f;
			unsigned clusterSeed = FoldSeed(mLocationSeed);
			for (const visual::DeepSkyObject& object : clusters->objects)
			{
				if (object.kind != visual::DeepSkyKind::Cluster)
				{
					continue;
				}
				const float x = (mirrored && clusters->followMirror) ? 1.f - object.at.x : object.at.x;
				mStars.AddCluster({x, object.at.y}, object.size, object.variant > 0.5f, aspect, ++clusterSeed);
			}
		}
		mBuiltStarsCatalog = catalogVersion;
		mBuiltStarsStop = mStopId;
		mBuiltStars = starField;
		mBuiltMilkyWay = band;
		mStarsDirty = false;
	}

	void ProceduralBackdrop::DrawMotes(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, const visual::DeepSkyStop* deepSky, sf::Vector2f scrollPx, float timeSeconds, float cap)
	{
		using namespace visual;
		sf::RenderTarget& background = pipeline.Target(Layer::Background);
		const ColorScript& script = assets.Script();
		const PipelineSettings& settings = assets.Settings();
		// Sparse, dim motes give camera motion a close reference and gently drift when the ship is still.
		if (mNearField && settings.nearField.enabled)
		{
			NearField::Params debris;
			debris.scroll = scrollPx;
			debris.parallax = 0.65f;
			debris.timeSeconds = timeSeconds;
			debris.pixelSize = std::clamp(pipeline.WorldPixelScale() * 0.5f, 1.f, 2.f);
			debris.lightDirection = mStarDirection;
			debris.moteRamp = &script.Role("hull");
			debris.maxBrightness = cap * 0.4f;
			debris.density = std::clamp(0.35f + 0.2f * mSky.dustDensity, 0.35f, 0.7f);
			if (deepSky != nullptr && deepSky->motes)
			{
				debris.density = std::clamp(debris.density * *deepSky->motes, 0.f, 2.f);
			}
			mNearField->Draw(background, debris);
		}
	}

	void ProceduralBackdrop::StepOrbitSpeed(int step)
	{
		const int last = static_cast<int>(std::size(OrbitSpeedSteps)) - 1;
		mOrbitSpeedStep = static_cast<std::size_t>(std::clamp(static_cast<int>(mOrbitSpeedStep) + step, 0, last));
	}

	float ProceduralBackdrop::OrbitSpeedScale() const
	{
		return OrbitSpeedSteps[mOrbitSpeedStep];
	}

	void ProceduralBackdrop::DrawOrbitBackground(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, const visual::DeepSkyStop& deepSky, const sky::DeepSkyPlanetEntry& owner, const visual::OrbitSettings& orbit, sf::Vector2f cameraCenter, float viewHeight, float timeSeconds, float cap)
	{
		using namespace visual;
		sf::RenderTarget& background = pipeline.Target(Layer::Background);
		const PipelineSettings& settings = assets.Settings();
		const sf::Vector2f output(pipeline.OutputSize());
		const float unitsToPixels = UnitsToPixels(output.y, viewHeight);
		const sf::Vector2f drift = (cameraCenter - mCameraStart) * unitsToPixels;
		const BackdropComposition composition = BuildStopComposition(settings, mLocationSeed, &deepSky);

		// The gradient and the band as on the normal path; the band is the sky itself here, so it turns with the orbit.
		const SkyPalette palette = SkyPaletteFor(mSky);
		DrawSkyGradient(background, palette, &deepSky, cap);
		mOrbitSeconds += std::max(timeSeconds - mOrbitClockTime, 0.f) * OrbitSpeedScale();
		mOrbitClockTime = timeSeconds;
		mOrbitRate = orbit.degreesPerMinute * OrbitSpeedScale();
		const OrbitSky::View view = OrbitSky::MakeView(orbit, output, mOrbitSeconds, drift * composition.band.parallax);
		mOrbit.DrawSky(assets.Shaders(), background, BandParams(mSky, mLocationSeed, composition.band, palette, &deepSky, cap), orbit, view);

		// Field stars only: the band is the panorama, and the stop's clusters are left out with its objects.
		MilkyWaySettings noBand = composition.band;
		noBand.enabled = false;
		RebuildStars(StarFieldFor(settings.starField, &deepSky), noBand, nullptr, composition.mirrored, output, assets.DeepSky().Version());
		mStars.SetMotion(mBeta, mDirection, settings.warp.fieldOfViewRad);
		mStars.DrawRigid(background, view.StarOffset(), timeSeconds, cap);

		// The host star is on screen only in front of the viewer; behind it, it still lights the planet.
		const float starLon = orbit.starDeg * DegToRad;
		const float starAcross = orbit.starAcrossDeg * DegToRad;
		const bool starShown = settings.hostStar.enabled && !deepSky.hostStar.hide;
		const sf::Vector3f starDirection = view.Direction(starLon, starAcross);
		if (starShown && starDirection.z < 0.f)
		{
			const float starRadius = std::max(std::min(OrbitSky::StarRadius(view, mHostStar.radiusSolar, mHostStar.distanceAU), 0.35f * output.y), 1.5f);
			DrawHostStarAt(assets, pipeline, view.ScreenPoint(starLon, starAcross), cap, starRadius);
		}
		pipeline.CompositeNebula();

		// The orbited planet's kind is the stop's override, else the kind of the planet it orbits.
		// A legacy owner keeps its historical ignored position/scale/hide behavior; an explicit array entry uses them.
		const bool legacyOwner = owner.legacy;
		const bool ownerVisible = settings.planet.enabled && (legacyOwner ? (mHasPlanet || orbit.kind.has_value()) : !owner.planet.hide);
		if (ownerVisible)
		{
			PlanetKind kind = mPlanetKind;
			if (orbit.planet >= 0 && static_cast<std::size_t>(orbit.planet) < mPlanetKinds.size())
			{
				kind = mPlanetKinds[static_cast<std::size_t>(orbit.planet)];
			}
			if (owner.planet.kind && !legacyOwner)
			{
				kind = *owner.planet.kind;
			}
			if (orbit.kind)
			{
				kind = *orbit.kind;
			}
			PlanetParams planet;
			SetPlanetKind(planet, kind);
			sky::ResolvePlanetAtmosphere(planet, assets.Settings(), &owner.planet);
			planet.brightness = settings.planet.brightness * orbit.brightness;
			planet.saturation = settings.planet.saturation;
			planet.dither = settings.planet.dither;
			// The orbited planet is the scene, not haze: it is opaque and drawn at the screen's pixel size.
			planet.opacity = 1.f;
			planet.pixelScale = 1.f;
			planet.surfacePixelScale = 1.f;
			planet.seed = static_cast<float>(FoldSeed(mLocationSeed) % 97u) + 1.f;
			planet.timeSeconds = timeSeconds;
			planet.light = starShown ? OrbitSky::PlanetLight(orbit, view, starLon, starAcross) : sf::Vector3f(-0.7f, -0.45f, 0.55f);
			SetStarLight(planet, mHostStar, starShown);
			OrbitSky::PlacePlanet(planet, orbit, view, -drift * settings.planet.parallax);
			if (!legacyOwner)
			{
				// An explicit owner uses its authored screen-fraction offset from the framing centre and radius scale.
				if (owner.planet.at)
				{
					const float offsetX = composition.mirrored ? -owner.planet.at->x : owner.planet.at->x;
					planet.center += sf::Vector2f(offsetX * output.x, owner.planet.at->y * output.y);
				}
				planet.radius *= owner.planet.scale.value_or(1.f);
			}
			// The ring and the moon pass behind the planet on the far side of their orbit and in front of it on the near side.
			const ColorRamp ramp = PlanetRamp(kind);
			const unsigned moonSeed = FoldSeed(mLocationSeed) ^ 0x6d2b79f5u;
			// Light reflected from the visible owner planet (planetshine).
			SetPlanetshineFill(pipeline, ramp, planet.center, planet.radius);
			visual::DrawPlanetRing(assets.Shaders(), pipeline, ramp, planet, false);
			if (orbit.moon)
			{
				visual::DrawPlanetMoon(assets.Shaders(), pipeline, planet, moonSeed, timeSeconds, false);
			}
			visual::DrawPlanet(assets.Shaders(), pipeline, ramp, planet);
			visual::DrawPlanetRing(assets.Shaders(), pipeline, ramp, planet, true);
			if (orbit.moon)
			{
				visual::DrawPlanetMoon(assets.Shaders(), pipeline, planet, moonSeed, timeSeconds, true);
			}
			mPlanetCenter = planet.center;
			mPlanetRadius = planet.radius;
			const float lightLength = std::sqrt(planet.light.x * planet.light.x + planet.light.y * planet.light.y);
			if (lightLength > 0.f)
			{
				mStarDirection = sf::Vector2f(planet.light.x / lightLength, planet.light.y / lightLength);
			}
		}

		// Additional bodies keep the owner's scene and suppression rules; they draw after entry 0 as their own groups.
		DrawAdditionalPlanetBodies(assets, pipeline, cameraCenter, viewHeight, timeSeconds, &orbit);

		DrawMotes(assets, pipeline, &deepSky, drift, timeSeconds, cap);

		LensParams lens;
		lens.enabled = false;
		pipeline.SetLens(lens);
	}

	bool ProceduralBackdrop::DrawPlanet(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f cameraCenter, float viewHeight, float timeSeconds)
	{
		const visual::PlanetSettings& settings = assets.Settings().planet;
		if (!settings.enabled || viewHeight <= 0.f)
		{
			return false;
		}
		const sf::Vector2f output(pipeline.OutputSize());
		const float unitsToPixels = UnitsToPixels(output.y, viewHeight);
		const sf::Vector2f drift = cameraCenter - mCameraStart;
		const visual::DeepSkyStop* deepSky = mStopId.empty() ? nullptr : assets.DeepSky().Find(SkyIdForRouteStop(mStopId));
		const BackdropComposition composition = BuildStopComposition(assets.Settings(), mLocationSeed, deepSky);
		// An array-only sky has an explicit resolved entry 0; a legacy entry 0 keeps the original path below.
		const sky::DeepSkyPlanetEntry* owner = deepSky != nullptr && !deepSky->planets.empty() ? &deepSky->planets.front() : nullptr;
		if (owner != nullptr && !owner->legacy)
		{
			EntryBodyContext context;
			context.output = output;
			context.fallbackAt = composition.planetAt;
			context.baseRadius = output.y * settings.radius * BuildBackdropComposition(assets.Settings(), mLocationSeed).planetScale;
			context.driftPixels = drift * unitsToPixels;
			context.planetParallax = settings.parallax;
			context.bandParallax = composition.band.parallax;
			context.elapsedSeconds = std::max(timeSeconds - mLocationStart, 0.f);
			context.pipelineRotationSpeed = settings.rotationSpeed;
			context.mirrored = composition.mirrored;
			const PlanetBodyDescription body = BuildEntryBody(*owner, 0, context);
			const auto& host = assets.Settings().hostStar;
			const sf::Vector2f hostCenter = BackdropCenter(output, composition.starAt.x, composition.starAt.y, drift, host.parallax, unitsToPixels);
			return DrawPlanetBody(assets, pipeline, body, hostCenter, deepSky != nullptr && deepSky->hostStar.hide, timeSeconds, 0);
		}
		PlanetBodyDescription body;
		body.center = BackdropCenter(output, composition.planetAt.x, composition.planetAt.y, drift, settings.parallax, unitsToPixels);
		body.radius = output.y * settings.radius * composition.planetScale;
		body.kind = mPlanetKind;
		body.rotationSpeed = settings.rotationSpeed;
		body.rotationScale = PlanetSpinScale;
		body.moonSeed = FoldSeed(mLocationSeed) ^ 0x6d2b79f5u;
		body.visible = mHasPlanet && !(deepSky != nullptr && deepSky->planet.hide);
		body.authored = deepSky != nullptr ? &deepSky->planet : nullptr;
		const auto& host = assets.Settings().hostStar;
		const sf::Vector2f hostCenter = BackdropCenter(output, composition.starAt.x, composition.starAt.y, drift, host.parallax, unitsToPixels);
		return DrawPlanetBody(assets, pipeline, body, hostCenter, deepSky != nullptr && deepSky->hostStar.hide, timeSeconds, 0);
	}

	ProceduralBackdrop::PlanetBodyDescription ProceduralBackdrop::BuildEntryBody(const sky::DeepSkyPlanetEntry& entry, std::size_t index, const EntryBodyContext& context) const
	{
		const visual::DeepSkyPlanet& authored = entry.planet;
		PlanetBodyDescription body;
		body.visible = !authored.hide;
		body.authored = &authored;
		body.kind = authored.kind ? *authored.kind : (entry.orbit && entry.orbit->kind ? *entry.orbit->kind : (index < mPlanetKinds.size() ? mPlanetKinds[index] : mPlanetKind));
		body.seed = index == 0 ? visual::PlanetParams{}.seed : EntrySurfaceSeed(mLocationSeed, index);
		body.moonSeed = index == 0 ? FoldSeed(mLocationSeed) ^ 0x6d2b79f5u : EntryMoonSeed(mLocationSeed, index);
		const float scale = authored.scale.value_or(1.f);
		if (context.sceneOrbit != nullptr)
		{
			// An additional orbit body inherits the owner's framing when it has no orbit block of its own;
			// only an authored block contributes a ring, a moon and its brightness.
			const bool authoredOrbit = entry.orbit.has_value();
			const visual::OrbitSettings& orbit = authoredOrbit ? *entry.orbit : *context.sceneOrbit;
			const visual::OrbitSky::View view = visual::OrbitSky::MakeView(*context.sceneOrbit, context.output, mOrbitSeconds, context.driftPixels * context.bandParallax);
			visual::PlanetParams placed;
			visual::OrbitSky::PlacePlanet(placed, orbit, view, -context.driftPixels * context.planetParallax);
			body.center = placed.center;
			if (authored.at)
			{
				// New orbit entries read `at` as a screen-fraction offset from the default framing centre.
				const float offsetX = context.mirrored ? -authored.at->x : authored.at->x;
				body.center += sf::Vector2f(offsetX * context.output.x, authored.at->y * context.output.y);
			}
			body.radius = placed.radius * scale;
			body.rotation = placed.rotation;
			body.spinAxis = placed.spinAxis;
			body.ring = authoredOrbit ? placed.ring : std::nullopt;
			body.brightnessScale = authoredOrbit ? orbit.brightness : 1.f;
			body.moon = authoredOrbit && orbit.moon;
			body.light = visual::OrbitSky::PlanetLight(orbit, view, context.sceneOrbit->starDeg * DegToRad, context.sceneOrbit->starAcrossDeg * DegToRad);
			return body;
		}
		// Ordinary scene: an absolute screen fraction at the camera start, mirrored like entry 0, with camera parallax.
		sf::Vector2f at = context.fallbackAt;
		if (authored.at) at = {context.mirrored ? 1.f - authored.at->x : authored.at->x, authored.at->y};
		body.center = sf::Vector2f(context.output.x * at.x, context.output.y * at.y) - context.driftPixels * context.planetParallax;
		if (authored.driftPerMinute)
		{
			// An authored drift moves the body from its composed position over the location clock; clamping keeps it near the screen.
			const float minutes = context.elapsedSeconds / 60.f;
			body.center += sf::Vector2f(authored.driftPerMinute->x * context.output.x * minutes, authored.driftPerMinute->y * context.output.y * minutes);
			body.center.x = std::clamp(body.center.x, -DriftScreenMargin * context.output.x, (1.f + DriftScreenMargin) * context.output.x);
			body.center.y = std::clamp(body.center.y, -DriftScreenMargin * context.output.y, (1.f + DriftScreenMargin) * context.output.y);
		}
		body.radius = context.baseRadius * scale;
		if (authored.rotationDegreesPerMinute)
		{
			// Signed degrees per minute -> radians per second, giving uRotation = value * elapsed seconds.
			body.rotationSpeed = *authored.rotationDegreesPerMinute * DegToRad / 60.f;
			body.rotationScale = 1.f;
		}
		else
		{
			body.rotationSpeed = context.pipelineRotationSpeed;
			body.rotationScale = PlanetSpinScale;
		}
		if (entry.orbit)
		{
			body.moon = entry.orbit->moon;
			if (entry.orbit->ring)
			{
				// In an ordinary scene an extra's orbit ring contributes its radii and opacity; the ordinary equatorial normal is used.
				visual::PlanetRing ring;
				ring.inner = entry.orbit->ring->inner;
				ring.outer = entry.orbit->ring->outer;
				ring.opacity = entry.orbit->ring->opacity;
				body.ring = ring;
			}
		}
		else
		{
			// Entry 0 keeps the ordinary planet's procedural moon; extra ordinary bodies have no attachments unless authored.
			body.moon = index == 0;
		}
		return body;
	}

	void ProceduralBackdrop::DrawAdditionalPlanetBodies(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f cameraCenter, float viewHeight, float timeSeconds, const visual::OrbitSettings* sceneOrbit)
	{
		const visual::PipelineSettings& settings = assets.Settings();
		if (!settings.planet.enabled || viewHeight <= 0.f) return;
		const sf::Vector2f output(pipeline.OutputSize());
		const sf::Vector2f drift = cameraCenter - mCameraStart;
		const float unitsToPixels = UnitsToPixels(output.y, viewHeight);
		const auto* deepSky = mStopId.empty() ? nullptr : assets.DeepSky().Find(SkyIdForRouteStop(mStopId));
		const auto composition = BuildStopComposition(settings, mLocationSeed, deepSky);
		const auto& host = settings.hostStar;
		const sf::Vector2f hostCenter = BackdropCenter(output, composition.starAt.x, composition.starAt.y, drift, host.parallax, unitsToPixels);
		const bool starHidden = sceneOrbit != nullptr ? !(host.enabled && !(deepSky != nullptr && deepSky->hostStar.hide)) : (deepSky != nullptr && deepSky->hostStar.hide);
		if (!mAdditionalPlanetBodies.empty())
		{
			// Fixture-only seam: stage-1 tests feed resolved bodies directly and do not change an orbit scene.
			if (sceneOrbit != nullptr) return;
			for (std::size_t i = 0; i < mAdditionalPlanetBodies.size(); ++i)
			{
				auto body = mAdditionalPlanetBodies[i];
				body.center -= drift * settings.planet.parallax * unitsToPixels;
				DrawPlanetBody(assets, pipeline, body, hostCenter, starHidden, timeSeconds, i + 1);
			}
			return;
		}
		if (deepSky == nullptr || deepSky->planets.size() <= 1) return;
		EntryBodyContext context;
		context.output = output;
		context.fallbackAt = composition.planetAt;
		context.baseRadius = output.y * settings.planet.radius * BuildBackdropComposition(settings, mLocationSeed).planetScale;
		context.driftPixels = drift * unitsToPixels;
		context.planetParallax = settings.planet.parallax;
		context.bandParallax = composition.band.parallax;
		context.elapsedSeconds = std::max(timeSeconds - mLocationStart, 0.f);
		context.pipelineRotationSpeed = settings.planet.rotationSpeed;
		context.mirrored = composition.mirrored;
		context.sceneOrbit = sceneOrbit;
		for (std::size_t i = 1; i < deepSky->planets.size(); ++i)
		{
			DrawPlanetBody(assets, pipeline, BuildEntryBody(deepSky->planets[i], i, context), hostCenter, starHidden, timeSeconds, i);
		}
	}

	bool ProceduralBackdrop::DrawPlanetBody(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, const PlanetBodyDescription& body, sf::Vector2f hostCenter, bool starHidden, float timeSeconds, std::size_t index)
	{
		const visual::PlanetSettings& settings = assets.Settings().planet;
		visual::PlanetParams planet;
		// A distant background moves continuously against the camera, without the gameplay pixel grid.
		planet.center = body.center;
		planet.radius = body.radius;
		planet.saturation = settings.saturation;
		planet.opacity = settings.opacity;
		planet.dither = settings.dither;
		planet.pixelScale = 1.f;
		planet.surfacePixelScale = 1.f;
		visual::SetPlanetKind(planet, body.kind);
		sky::ResolvePlanetAtmosphere(planet, assets.Settings(), body.authored);
		planet.seed = body.seed;
		planet.ring = body.ring;
		planet.timeSeconds = timeSeconds;
		planet.rotation = body.rotation ? *body.rotation : timeSeconds * body.rotationSpeed * body.rotationScale;
		planet.spinAxis = body.spinAxis;
		planet.brightness = settings.brightness * body.brightnessScale;
		planet.light = { -0.7f, -0.45f, 0.55f };

		const visual::HostStarSettings& host = assets.Settings().hostStar;
		if (body.light)
		{
			// An orbit-scene entry has an exact scene light direction (already lifted toward its framing).
			planet.light = *body.light;
			SetStarLight(planet, mHostStar, !starHidden);
		}
		else if (host.enabled)
		{
			// Light points from the planet to the host star on screen, lifted toward the viewer.
			const sf::Vector2f toStar = hostCenter - planet.center;
			const float length = std::sqrt(toStar.x * toStar.x + toStar.y * toStar.y);
			if (length > 0.f)
			{
				planet.light = { toStar.x / length, toStar.y / length, PlanetLightLift };
			}
			SetStarLight(planet, mHostStar, !starHidden);
		}

		if (index == 0)
		{
			mPlanetCenter = planet.center;
			mPlanetRadius = planet.radius;
			const float lightLength = std::sqrt(planet.light.x * planet.light.x + planet.light.y * planet.light.y);
			if (lightLength > 0.f)
			{
				mStarDirection = sf::Vector2f(planet.light.x / lightLength, planet.light.y / lightLength);
			}
		}
		// No planet at this place: keep the light direction above, but draw neither the planet nor its moon.
		if (!body.visible)
		{
			return false;
		}
		// The orbiting moon is behind the planet in the upper half of its orbit, in front in the lower half.
		const auto observe = [&](PlanetBodyPass pass) { if (mPlanetBodyObserver) mPlanetBodyObserver(index, pass, planet, body.moonSeed); };
		const auto ramp = visual::PlanetRamp(body.kind);
		if (index == 0)
		{
			// Light reflected from the stop's visible owner planet (planetshine).
			SetPlanetshineFill(pipeline, ramp, planet.center, planet.radius);
		}
		if (body.ring)
		{
			observe(PlanetBodyPass::BackRing);
			visual::DrawPlanetRing(assets.Shaders(), pipeline, ramp, planet, false);
		}
		if (body.moon)
		{
			observe(PlanetBodyPass::BackMoon);
			visual::DrawPlanetMoon(assets.Shaders(), pipeline, planet, body.moonSeed, timeSeconds, false);
		}
		observe(PlanetBodyPass::Body);
		visual::DrawPlanet(assets.Shaders(), pipeline, ramp, planet);
		if (body.ring)
		{
			observe(PlanetBodyPass::FrontRing);
			visual::DrawPlanetRing(assets.Shaders(), pipeline, ramp, planet, true);
		}
		if (body.moon)
		{
			observe(PlanetBodyPass::FrontMoon);
			visual::DrawPlanetMoon(assets.Shaders(), pipeline, planet, body.moonSeed, timeSeconds, true);
		}
		return true;
	}
}
