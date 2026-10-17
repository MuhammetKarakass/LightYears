#include "GalleryScene.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <random>
#include <utility>

using namespace vega::visual;

namespace
{
	// Same generator as the game's ProceduralBackdrop (splitmix64): one progress value gives the same features everywhere.
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

	constexpr float PlanetLightLift = 0.2f;
	constexpr float MoonOrbitPerPlanetRadius = 1.6f;
	// The backdrop planet turns 4x the pipeline's rotationSpeed, as in the game.
	constexpr float PlanetSpinScale = 4.f;
	constexpr float SunTemperatureK = 5772.f;
	// Sgr A* is the final stop of the route: the S2-like star from GalaxyRoute.
	constexpr float SgrATemperatureK = 22000.f;
	constexpr float SgrAMassSolar = 20.f;
	constexpr float PulsarMinProgress = 0.2f;
	constexpr float LensMinProgress = 0.999f;
	// The lens sits in the upper right, off the play-area centre, so the swirl stays out of the fight.
	constexpr float LensCenterX = 0.76f;
	constexpr float LensCenterY = 0.30f;
	// Same rules as ProceduralBackdrop: a distant HII region, only in the arms and in ~60% of them.
	constexpr float EmissionMinArmProximity = 0.45f;
	constexpr float EmissionChance = 0.6f;
	constexpr float DustChance = 0.5f;

	std::uint64_t HashProgress(float progress)
	{
		std::uint32_t bits = 0;
		std::memcpy(&bits, &progress, sizeof bits);
		return SeededRandom(0x9e3779b97f4a7c15ULL ^ bits).NextU64();
	}

	// Two draws in a fixed order: unsequenced arguments would make the seeded sequence depend on the compiler.
	sf::Vector2f RandomPoint(SeededRandom& random, float low, float high, float lowY, float highY)
	{
		const float x = random.Range(low, high);
		const float y = random.Range(lowY, highY);
		return sf::Vector2f(x, y);
	}

	// Space gradient colour: the region's shadow colour scaled by factor, so the sky stays near black.
	sf::Color ShadowColor(const SkyPalette& palette, float factor)
	{
		auto channel = [factor](float v) { return static_cast<std::uint8_t>(std::clamp(v * factor, 0.f, 1.f) * 255.f); };
		return sf::Color(channel(palette.shadow.x), channel(palette.shadow.y), channel(palette.shadow.z));
	}

	sf::Glsl::Vec3 BlackbodyDisplay(float temperatureK)
	{
		const astro::Rgb8 rgb = astro::ToSrgb8(astro::BlackbodyColor(temperatureK));
		return sf::Glsl::Vec3(rgb.r / 255.f, rgb.g / 255.f, rgb.b / 255.f);
	}

	// Host star of a place. Progress 0 is the Sun and Sgr A* its S2-like star; in between, a class is sampled from the
	// stellar mix and the radius and luminosity follow the main sequence, as GalaxyRoute does for procedural stops.
	HostStar HostStarAt(float progress, const galaxy::SkyState& sky, float uClass, float uTemp)
	{
		float temperatureK = SunTemperatureK;
		float massSolar = 1.f;
		if (progress >= LensMinProgress)
		{
			temperatureK = SgrATemperatureK;
			massSolar = SgrAMassSolar;
		}
		else if (progress > 0.f)
		{
			temperatureK = astro::SampleStarTemperature(sky.stellarMix, uClass, uTemp);
			massSolar = std::pow(temperatureK / SunTemperatureK, 1.f / 0.6f);
		}
		HostStar star;
		star.temperatureK = temperatureK;
		star.luminositySolar = massSolar >= 0.43f ? std::pow(massSolar, 4.f) : 0.23f * std::pow(massSolar, 2.3f);
		star.radiusSolar = std::pow(massSolar, 0.8f);
		star.distanceAU = std::sqrt(star.luminositySolar);
		return star;
	}
}

GalleryScene::GalleryScene(VisualAssets& assets, VisualPipeline& pipeline)
	: mAssets(assets)
	, mPipeline(pipeline)
	, mBrush(assets.Shaders())
	, mShips(assets)
	, mStars(1337)
{
	mPosition = galaxy::RoutePosition(mGalaxyProgress);
	mSky = galaxy::ComputeSky(mPosition);
}

void GalleryScene::SetGalaxyProgress(float progress)
{
	const float clamped = std::clamp(progress, 0.f, 1.f);
	if (clamped == mGalaxyProgress)
	{
		return;
	}
	mGalaxyProgress = clamped;
	mPosition = galaxy::RoutePosition(clamped);
	mSky = galaxy::ComputeSky(mPosition);
	mStarsDirty = true;
	mFeaturesDirty = true;
	mLocationStart = mTime;
}

void GalleryScene::Update(float deltaSeconds)
{
	mTime += deltaSeconds;
	mScroll += sf::Vector2f(7.f, 3.f) * deltaSeconds;

	// Warp eases like the game's backdrop: about 98% of the way after warp.rampSeconds.
	const WarpSettings& warp = mAssets.Settings().warp;
	const float target = std::min(mWarpTarget, warp.maxBeta);
	const float dt = std::clamp(deltaSeconds, 0.f, 0.1f);
	const float rate = 4.f / std::max(warp.rampSeconds, 0.05f);
	mBeta += (target - mBeta) * (1.f - std::exp(-rate * dt));
}

void GalleryScene::SetWarp(float targetBeta, sf::Vector2f direction)
{
	mWarpTarget = std::clamp(targetBeta, 0.f, 0.99f);
	const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
	if (length > 0.f)
	{
		mWarpDirection = direction * (1.f / length);
	}
}

std::string GalleryScene::FeatureFlags() const
{
	std::string flags;
	const auto add = [&flags](bool on, const char* name)
	{
		if (on)
		{
			if (!flags.empty())
			{
				flags += ' ';
			}
			flags += name;
		}
	};
	add(mLocation.emission, "emission");
	add(mLocation.dust, "dust");
	add(mLocation.pulsar, "pulsar");
	add(mLocation.moon.has_value(), "moon");
	add(mLocation.lens, "lens");
	return flags.empty() ? "none" : flags;
}

void GalleryScene::RebuildLocation(const PipelineSettings& settings)
{
	const NebulaSettings& nebula = settings.physicalNebula;
	const AnomalySettings& anomaly = settings.anomalies;
	const float armProximity = mSky.armProximity;
	SeededRandom random(HashProgress(mGalaxyProgress));

	Location next;
	const float uClass = random.Next01();
	const float uTemp = random.Next01();
	next.hostStar = HostStarAt(mGalaxyProgress, mSky, uClass, uTemp);
	// Same draw order as ProceduralBackdrop::RebuildFeatures, so a switched-off feature leaves the others unchanged.
	next.emissionAt = RandomPoint(random, 0.2f, 0.8f, 0.15f, 0.6f);
	next.emissionTemperatureK = random.Range(30000.f, 45000.f);
	next.emissionSeed = random.Range(0.f, 100.f);
	const bool dustRoll = random.Next01() < DustChance;
	next.dustAt = RandomPoint(random, 0.15f, 0.85f, 0.15f, 0.85f);
	next.dustAngleRad = random.Range(-1.5f, 1.5f);
	next.dustSeed = random.Range(0.f, 100.f);
	const sf::Vector2f strayAt = RandomPoint(random, 0.1f, 0.9f, 0.1f, 0.9f);
	const bool pulsarRoll = random.Next01() < anomaly.pulsarChance;
	next.pulsarAt = RandomPoint(random, 0.1f, 0.9f, 0.1f, 0.9f);
	next.pulsarPeriodSeconds = random.Range(0.7f, 3.f);
	const bool moonRoll = random.Next01() < anomaly.shatteredMoonChance;
	const auto moonSeed = static_cast<std::uint32_t>(random.NextU64());
	// Drawn last, so the draws above keep their values when this roll changes.
	const bool emissionRoll = random.Next01() < EmissionChance;

	next.emission = nebula.enabled && armProximity > EmissionMinArmProximity && emissionRoll;
	next.dust = nebula.enabled && (mSky.dustDensity > 0.8f || armProximity > 0.5f) && dustRoll;
	next.dustIlluminatorAt = next.emission ? next.emissionAt : strayAt;
	next.pulsar = pulsarRoll && mGalaxyProgress > PulsarMinProgress;
	// No route data here: the planet kind and its moon come from the progress seed alone (no draws from the random stream).
	const auto planetSeed = static_cast<unsigned>(HashProgress(mGalaxyProgress) >> 32);
	next.planetKind = ClassifyPlanet(0.f, 0.f, planetSeed);
	next.moonSeed = planetSeed ^ 0x6d2b79f5u;
	// The gallery has no planets to orbit, so the shattered moon is limited by its chance alone.
	if (moonRoll)
	{
		next.moon.emplace(moonSeed);
	}
	next.lens = mGalaxyProgress >= LensMinProgress;
	mLocation = std::move(next);
	// Near-field debris belongs to the place: the same seed as the progress gives the same rocks everywhere.
	mNearField.emplace(static_cast<unsigned>(HashProgress(mGalaxyProgress)));
}


void GalleryScene::Draw()
{
	// The showcase draws far more ships than a game frame does, so lift the energy budget it would otherwise hit.
	if (!mStress)
	{
		mAssets.Settings().maxEnergyDraws = std::max(mAssets.Settings().maxEnergyDraws, 400);
		mAssets.Settings().maxEnergyAreaRatio = std::max(mAssets.Settings().maxEnergyAreaRatio, 4.f);
	}
	DrawBackground();
	if (mBackdropOnly)
	{
		return;
	}
	DrawWorld();
	DrawShips();
	DrawEnergy();
}

void GalleryScene::DrawBackground()
{
	sf::RenderTarget& bg = mPipeline.Target(Layer::Background);
	sf::RenderTarget& nebulaTarget = mPipeline.Target(Layer::Nebula);
	const PipelineSettings& settings = mAssets.Settings();
	const ColorScript& script = mAssets.Script();
	const float cap = settings.tierMaxBrightness[TierIndex(LuminanceTier::Background)];

	if (mStarsDirty || settings.starField != mBuiltStars || settings.milkyWay != mBuiltMilkyWay)
	{
		mStars.Build(mSky, settings.starField, settings.milkyWay);
		mBuiltStars = settings.starField;
		mBuiltMilkyWay = settings.milkyWay;
		mStarsDirty = false;
	}
	if (mFeaturesDirty || settings.physicalNebula != mBuiltNebula || settings.anomalies != mBuiltAnomalies)
	{
		RebuildLocation(settings);
		mBuiltNebula = settings.physicalNebula;
		mBuiltAnomalies = settings.anomalies;
		mFeaturesDirty = false;
	}

	const sf::Vector2f output(mPipeline.OutputSize());
	// No camera here: the scroll is the drift in output pixels and each body applies its own parallax, as in the game.
	const sf::Vector2f scroll = mScroll;
	const float armProximity = mSky.armProximity;
	const NebulaSettings& nebula = settings.physicalNebula;
	auto placed = [&output, &scroll](float screenX, float screenY, float parallax)
	{
		return sf::Vector2f(output.x * screenX, output.y * screenY) - scroll * parallax;
	};

	// Region colour script: the gradient is the region's shadow colour, near black; the sky paint carries the colour.
	const SkyPalette palette = SkyPaletteFor(mSky);
	DrawGradient(bg, ShadowColor(palette, 0.6f), ShadowColor(palette, 1.f), cap);

	// The Milky Way, rendered once per location and added with parallax; resolved stars go over it.
	GalaxyLayer::Params galaxyParams;
	galaxyParams.sky = mSky;
	galaxyParams.tint = palette;
	galaxyParams.settings = settings.milkyWay;
	galaxyParams.seed = static_cast<unsigned>(HashProgress(mGalaxyProgress));
	galaxyParams.maxBrightness = cap;
	mGalaxy.Draw(mAssets.Shaders(), bg, galaxyParams, scroll);

	// Stars right after the gradient: the opaque planet hides the stars behind it, and the clouds sit over distant stars.
	mStars.SetMotion(mBeta, mWarpDirection, settings.warp.fieldOfViewRad);
	mStars.Draw(bg, scroll, mTime, cap);

	// The host star and the planet are drawn over the stars and under the clouds (art direction).
	const HostStarSettings& hostSettings = settings.hostStar;
	sf::Vector2f lightDirection(-0.84f, -0.54f);
	if (hostSettings.enabled)
	{
		const float irradiance = HostStarIrradiance(mLocation.hostStar);
		HostStarParams star;
		star.center = placed(hostSettings.screenX, hostSettings.screenY, hostSettings.parallax);
		star.radius = std::min(std::max(HostStarApparentRadius(mLocation.hostStar, hostSettings.radiusAt1AU * output.y), 1.5f), 0.018f * output.y);
		star.glowRadius = std::clamp(hostSettings.glowRadius * output.y * std::sqrt(irradiance), 0.03f * output.y, 0.3f * output.y);
		star.color = BlackbodyDisplay(mLocation.hostStar.temperatureK);
		star.intensity = std::max(0.15f, hostSettings.intensity * (1.f - std::exp(-1.5f * irradiance)));
		star.maxBrightness = cap;
		DrawHostStar(mAssets.Shaders(), bg, star);
	}

	const PlanetSettings& planetSettings = settings.planet;
	sf::Vector2f planetCenter;
	float planetRadius = 0.f;
	if (planetSettings.enabled)
	{
		PlanetParams planet;
		planet.center = placed(planetSettings.screenX, planetSettings.screenY, planetSettings.parallax);
		planet.radius = output.y * planetSettings.radius;
		planet.brightness = planetSettings.brightness;
		planet.saturation = planetSettings.saturation;
		planet.opacity = planetSettings.opacity;
		planet.dither = planetSettings.dither;
		planet.pixelScale = 1.f;
		planet.surfacePixelScale = 1.f;
		SetPlanetKind(planet, mLocation.planetKind);
		planet.timeSeconds = mTime;
		planet.rotation = mTime * planetSettings.rotationSpeed * PlanetSpinScale;
		planet.light = { -0.7f, -0.45f, 0.55f };
		if (hostSettings.enabled)
		{
			// Light points from the planet to the host star on screen, lifted toward the viewer.
			const sf::Vector2f toStar = placed(hostSettings.screenX, hostSettings.screenY, hostSettings.parallax) - planet.center;
			const float length = std::sqrt(toStar.x * toStar.x + toStar.y * toStar.y);
			if (length > 0.f)
			{
				lightDirection = sf::Vector2f(toStar.x / length, toStar.y / length);
				planet.light = { lightDirection.x, lightDirection.y, PlanetLightLift };
			}
			const float irradiance = HostStarIrradiance(mLocation.hostStar);
			planet.lightColor = BlackbodyDisplay(mLocation.hostStar.temperatureK);
			planet.lightIntensity = std::clamp(0.4f + 0.6f * (1.f - std::exp(-1.5f * irradiance)), 0.4f, 1.2f);
		}
		planetCenter = planet.center;
		planetRadius = planet.radius;
		// The orbiting moon is behind the planet in the upper half of its orbit, in front in the lower half.
		DrawPlanetMoon(mAssets.Shaders(), bg, planet, mLocation.moonSeed, mTime, false);
		DrawPlanet(mAssets.Shaders(), bg, PlanetRamp(planet.kind), planet);
		DrawPlanetMoon(mAssets.Shaders(), bg, planet, mLocation.moonSeed, mTime, true);
	}

	if (mLocation.moon && planetSettings.enabled)
	{
		ShatteredMoon::Params moon;
		moon.planetCenter = planetCenter;
		moon.orbitRadius = MoonOrbitPerPlanetRadius * planetRadius;
		moon.ageSeconds = mTime - mLocationStart;
		moon.lightDirection = lightDirection;
		moon.maxBrightness = cap;
		mLocation.moon->Draw(bg, moon);
	}

	// Lit cloud layers, lit by the host star where it is now (the same position the host star uses).
	const SkyPaintSettings& skyPaint = settings.skyPaint;
	if (skyPaint.enabled)
	{
		SkyPaintParams paint;
		paint.scroll = scroll;
		paint.lightPos = placed(hostSettings.screenX, hostSettings.screenY, hostSettings.parallax);
		paint.palette = palette;
		paint.coverage = skyPaint.coverage;
		paint.intensity = skyPaint.intensity;
		paint.centerCalm = skyPaint.centerCalm;
		paint.timeSeconds = mTime;
		paint.seed = static_cast<float>(HashProgress(mGalaxyProgress) % 97u);
		paint.maxBrightness = cap;
		paint.farParallax = skyPaint.farParallax;
		paint.midParallax = skyPaint.midParallax;
		paint.nearParallax = skyPaint.nearParallax;
		DrawSkyPaint(mAssets.Shaders(), nebulaTarget, paint);
	}


	if (nebula.enabled && mLocation.emission)
	{
		EmissionNebulaParams emission;
		emission.center = sf::Vector2f(output.x * mLocation.emissionAt.x, output.y * mLocation.emissionAt.y);
		emission.radius = nebula.emissionRadius * output.y * (0.6f + 0.8f * armProximity);
		emission.scroll = scroll;
		emission.parallax = nebula.parallax;
		emission.ionizingTemperatureK = mLocation.emissionTemperatureK;
		emission.density = nebula.emissionDensity * (0.5f + armProximity);
		emission.seed = mLocation.emissionSeed;
		emission.palette = nebula.palette;
		emission.saturation = nebula.saturation;
		emission.timeSeconds = mTime;
		emission.maxBrightness = cap;
		DrawEmissionNebula(mAssets.Shaders(), nebulaTarget, emission);
	}
	mPipeline.CompositeNebula();

	if (nebula.enabled && mLocation.dust)
	{
		DustCloudParams dust;
		dust.center = sf::Vector2f(output.x * mLocation.dustAt.x, output.y * mLocation.dustAt.y);
		dust.size = sf::Vector2f(nebula.dustSize * output.y * 1.2f, nebula.dustSize * output.y * 0.55f);
		dust.angleRad = mLocation.dustAngleRad;
		dust.scroll = scroll;
		dust.parallax = nebula.parallax;
		dust.opticalDepth = nebula.dustOpticalDepth * std::clamp(mSky.dustDensity, 0.5f, 2.f);
		dust.reflection = nebula.reflection * armProximity;
		dust.illuminatorTemperatureK = 15000.f;
		dust.illuminator = sf::Vector2f(output.x * mLocation.dustIlluminatorAt.x, output.y * mLocation.dustIlluminatorAt.y);
		dust.seed = mLocation.dustSeed;
		dust.maxBrightness = cap;
		dust.stylize = settings.backgroundStyle.enabled;
		DrawDustCloud(mAssets.Shaders(), bg, dust);
	}

	if (mLocation.pulsar)
	{
		PulsarParams pulsar;
		pulsar.center = placed(mLocation.pulsarAt.x, mLocation.pulsarAt.y, nebula.parallax);
		pulsar.periodSeconds = mLocation.pulsarPeriodSeconds;
		pulsar.beamLength = 0.4f * output.y;
		pulsar.nebulaRadius = 0.03f * output.y;
		pulsar.timeSeconds = mTime;
		pulsar.maxBrightness = cap;
		DrawPulsar(mAssets.Shaders(), bg, pulsar);
	}

	// Near field last: sparse rocks and motes with strong parallax, still under the background cap.
	if (mNearField && settings.nearField.enabled)
	{
		NearField::Params debris;
		debris.scroll = scroll;
		debris.timeSeconds = mTime;
		debris.pixelSize = mPipeline.WorldPixelScale();
		debris.lightDirection = lightDirection;
		debris.rockRamp = &script.Role("planet");
		debris.moteRamp = &script.Role("ambient");
		debris.maxBrightness = cap;
		debris.density = std::clamp(0.5f + 0.5f * mSky.dustDensity, 0.3f, 1.6f);
		mNearField->Draw(bg, debris);
	}

	LensParams lens;
	lens.enabled = mLocation.lens;
	lens.center = sf::Vector2f(output.x * LensCenterX, output.y * LensCenterY);
	lens.einsteinRadius = settings.anomalies.lensEinsteinRadius * output.y;
	lens.shadowRadius = settings.anomalies.lensShadowRadius * output.y;
	lens.ringBrightness = settings.anomalies.lensRingBrightness;
	mPipeline.SetLens(lens);
}

void GalleryScene::DrawWorld()
{
	sf::RenderTarget& world = mPipeline.Target(Layer::World);
	const ColorScript& script = mAssets.Script();
	sf::Vector2f size(mPipeline.OutputSize());
	float px = mPipeline.WorldPixelScale();

	// A hard-edged arc as scale reference, and the procedural planet, both far behind the ships.
	const ColorRamp& ambient = script.Role("ambient");
	ShapeStyle arc;
	arc.fill = ambient.Sample(0.5f);
	arc.outline = ambient.Sample(0.f);
	arc.outlineWidth = px;
	arc.hardEdges = true;
	sf::Vector2f arcCenter(size.x * 1.15f, size.y * 1.4f);
	mBrush.Arc(world, arcCenter, size.y * 1.25f, px * 5.f, 188.f, 74.f, arc);

	PlanetParams planet;
	planet.center = {size.x * 0.1f, size.y * 0.95f};
	planet.radius = size.y * 0.2f;
	planet.pixelScale = px;
	planet.rotation = mTime * 0.06f;
	// The world planet keeps its plain look: no clouds and no limb glow.
	planet.cloudAmount = 0.f;
	planet.atmosphereColor = sf::Glsl::Vec3(0.f, 0.f, 0.f);
	DrawPlanet(mAssets.Shaders(), world, script.Role("planet"), planet);
}

namespace
{
	// state: 0 unequipped, 1 cooldown, 2 ready, 3 active
	ShipPortState Port(AbilityCategory category, int state, float progress = 0.f)
	{
		ShipPortState port;
		port.equipped = state != 0;
		port.category = category;
		port.isReady = state == 2;
		port.isActive = state == 3;
		port.cooldownFraction = state == 1 ? progress : 1.f;
		return port;
	}
}

void GalleryScene::DrawShips()
{
	sf::Vector2f size(mPipeline.OutputSize());
	const float k = size.y / 720.f;
	// Roster order first, then anything else the ships folder holds.
	std::vector<std::string> ids;
	for (const char* wanted : {"fighter", "breacher", "interceptor", "conductor", "aegis", "cryo_controller"})
	{
		if (mAssets.Ships().Find(wanted) != nullptr)
		{
			ids.push_back(wanted);
		}
	}
	for (const std::string& id : mAssets.Ships().Ids())
	{
		if (std::find(ids.begin(), ids.end(), id) == ids.end())
		{
			ids.push_back(id);
		}
	}
	if (ids.empty())
	{
		return;
	}
	const float fullScale = 0.95f * k;
	// The game draws a ship at about 0.5 output pixels per unit on a 1080p screen; the bottom strip shows that size.
	const float gameScale = 0.34f * k;

	auto baseState = [&](int index, int count, float rowY, float scale)
	{
		ShipVisualState state;
		state.position = {size.x * (static_cast<float>(index) + 0.5f) / static_cast<float>(count), size.y * rowY};
		state.scale = scale;
		state.time = mTime;
		state.seed = 1u + static_cast<unsigned>(index) * 7919u;
		state.shieldFraction = 1.f;
		return state;
	};
	auto draw = [&](const std::string& id, const ShipVisualState& state)
	{
		if (const ShipVisualDefinition* definition = mAssets.Ships().Find(id))
		{
			mShips.Draw(mPipeline, *definition, state);
		}
	};
	// A short hit now and then on one ship of the first row, so the flash and the shield ripple show up in captures.
	const float hitPhase = std::fmod(mTime, 1.6f);
	const float hit = hitPhase < 0.3f ? 1.f - hitPhase / 0.3f : 0.f;

	// Row 1: every ship definition, thrusting, shields up, one in afterburner.
	const int count = static_cast<int>(ids.size());
	for (int i = 0; i < count; ++i)
	{
		ShipVisualState state = baseState(i, count, 0.17f, fullScale);
		state.thrust = 0.35f + 0.3f * std::sin(mTime * 0.8f + static_cast<float>(i));
		state.afterburner = i == 1;
		state.shieldFraction = i == 3 ? 0.35f : 1.f;
		if (i == static_cast<int>(mTime / 1.6f) % count)
		{
			state.hitFlash = hit;
			state.shieldHit = hit;
			state.shieldHitAngle = 0.8f * static_cast<float>(i);
		}
		for (int p = 0; p < ShipPortCount; ++p)
		{
			state.ports[p] = Port(static_cast<AbilityCategory>((i + p) % 5), 2);
		}
		draw(ids[static_cast<size_t>(i)], state);
	}

	// Row 2: the first ship at each damage stage (no shield), then mixed port states on two more ships.
	const float hull[4] = {1.f, 0.65f, 0.4f, 0.15f};
	for (int i = 0; i < 4; ++i)
	{
		ShipVisualState state = baseState(i, 6, 0.45f, fullScale);
		state.hullFraction = hull[i];
		state.shieldFraction = 0.f;
		state.thrust = 0.4f;
		draw(ids[0], state);
	}
	if (ids.size() > 4)
	{
		ShipVisualState state = baseState(4, 6, 0.45f, fullScale);
		state.shieldFraction = 0.6f;
		state.ports[0] = Port(AbilityCategory::Offense, 0);
		state.ports[1] = Port(AbilityCategory::Defense, 1, std::fmod(mTime * 0.25f, 1.f));
		state.ports[2] = Port(AbilityCategory::Movement, 2);
		state.ports[3] = Port(AbilityCategory::Utility, 3);
		draw(ids[3], state);

		state = baseState(5, 6, 0.45f, fullScale);
		const float phase = std::fmod(mTime, 2.f);
		const float burst = phase < 0.4f ? 1.f - phase / 0.4f : 0.f;
		state.ports[0] = Port(AbilityCategory::Primary, 2);
		state.ports[1] = Port(AbilityCategory::Offense, 2);
		state.ports[2] = Port(AbilityCategory::Offense, 1, 0.8f);
		state.ports[3] = Port(AbilityCategory::Defense, 2);
		state.ports[1].justActivated = burst;
		state.ports[1].isActive = burst > 0.f;
		draw(ids[4], state);
	}

	// Row 3: game-size strip, slightly rotated, alternating damage stages.
	for (int i = 0; i < count; ++i)
	{
		ShipVisualState state = baseState(i, count, 0.78f, gameScale);
		state.rotationDegrees = 25.f * std::sin(mTime * 0.5f + static_cast<float>(i));
		state.thrust = 0.5f;
		state.hullFraction = i % 2 == 0 ? 1.f : 0.45f;
		for (int p = 0; p < ShipPortCount; ++p)
		{
			state.ports[p] = Port(static_cast<AbilityCategory>((i + p) % 5), p == 1 ? 1 : 2, 0.5f);
		}
		draw(ids[static_cast<size_t>(i)], state);
	}
}

void GalleryScene::DrawEnergy()
{
	const ColorScript& script = mAssets.Script();
	sf::Vector2f size(mPipeline.OutputSize());
	float k = size.y / 720.f;

	const ColorRamp& warm = script.Role("combatWarm");
	const ColorRamp& enemy = script.Role("enemy");
	const ColorRamp& ambient = script.Role("ambient");
	const ColorRamp& ambient2 = script.Role("ambient2");

	// Enemy projectiles on the edges: warm, bright but never brighter than the player.
	for (int i = 0; i < 4; ++i)
	{
		float y = std::fmod(mTime * 150.f + static_cast<float>(i) * 137.f, size.y + 240.f) - 120.f;
		float x = size.x * (0.015f + 0.012f * static_cast<float>(i)) + 6.f * std::sin(mTime * 2.f + i);
		sf::Vector2f head(x, y);
		TrailParams tail;
		for (int p = 0; p < 7; ++p)
		{
			tail.points.push_back(head - sf::Vector2f(0.f, 11.f * k * static_cast<float>(p)));
		}
		tail.headWidth = 12.f * k;
		tail.tailWidth = 1.f * k;
		tail.color = (i % 2 == 0 ? warm : enemy).Sample(0.8f);
		tail.tier = LuminanceTier::EnemyProjectile;
		mPipeline.SubmitTrail(tail);

		GlowParams shot;
		shot.position = head;
		shot.coreRadius = 5.f * k;
		shot.haloRadius = 32.f * k;
		shot.color = tail.color;
		shot.tier = LuminanceTier::EnemyProjectile;
		mPipeline.SubmitGlow(shot);
	}

	// Decorative drifting motes, cool and low in the hierarchy.
	for (int i = 0; i < 10; ++i)
	{
		float a = mTime * (0.25f + 0.03f * static_cast<float>(i)) + static_cast<float>(i) * 1.7f;
		GlowParams mote;
		mote.position = {size.x * (0.5f + 0.45f * std::sin(a * 0.7f + i)), size.y * (0.5f + 0.45f * std::cos(a * 0.5f + i * 2.f))};
		mote.coreRadius = 2.5f * k;
		mote.haloRadius = 34.f * k;
		mote.color = (i % 2 == 0 ? ambient : ambient2).Sample(0.9f);
		mote.intensity = 0.9f;
		mote.tier = LuminanceTier::Decorative;
		mPipeline.SubmitGlow(mote);
	}

	if (mStress)
	{
		// Budget demo: far more decorative glows than the energy budget allows. They are culled first.
		std::mt19937 rng(7);
		std::uniform_real_distribution<float> unit(0.f, 1.f);
		for (int i = 0; i < 160; ++i)
		{
			GlowParams spam;
			spam.position = {unit(rng) * size.x, unit(rng) * size.y};
			spam.coreRadius = 3.f * k;
			spam.haloRadius = 44.f * k;
			spam.color = ambient2.Sample(0.9f);
			spam.tier = LuminanceTier::Decorative;
			mPipeline.SubmitGlow(spam);
		}
	}
}
