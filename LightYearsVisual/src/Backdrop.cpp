#include "visual/Backdrop.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

#include "visual/Relativity.h"
#include "visual/UniformWriter.h"

namespace ly::visual
{
	namespace
	{
		constexpr float kDegToRad = 0.01745329252f;
		constexpr float kCepheidAmplitude = 0.8f;
		constexpr float kEclipseDepth = 0.7f;
		constexpr float kMilkyWaySeed = 4.f;
		// below this beta the motion path is skipped, so a resting ship costs nothing extra per frame
		constexpr float kMotionThreshold = 0.001f;
		// streak length in output pixels per unit beta (stylised, see Draw)
		constexpr float kStreakPixelsPerBeta = 8.f;
		// chroma boost of the star colours (stylised, see BoostChroma)
		constexpr float kStarChromaBoost = 1.4f;
		// the brightest stars get a halo: this many segments around it
		constexpr int kHaloSegments = 8;
		// Colour adds to the background and destination alpha is left alone. sf::BlendAdd scales the colour by the source
		// alpha, and the host star writes alpha 0, which would make the star invisible.
		const sf::BlendMode LightAdd(sf::BlendMode::Factor::One, sf::BlendMode::Factor::One, sf::BlendMode::Equation::Add,
			sf::BlendMode::Factor::Zero, sf::BlendMode::Factor::One, sf::BlendMode::Equation::Add);
		// skypaint.frag writes premultiplied (colour * alpha, alpha), so the layer goes over what is there.
		const sf::BlendMode PremultipliedOver(sf::BlendMode::Factor::One, sf::BlendMode::Factor::OneMinusSrcAlpha, sf::BlendMode::Equation::Add,
			sf::BlendMode::Factor::One, sf::BlendMode::Factor::OneMinusSrcAlpha, sf::BlendMode::Equation::Add);

		float Dot(sf::Vector2f a, sf::Vector2f b)
		{
			return a.x * b.x + a.y * b.y;
		}

		float SmoothStep(float edge0, float edge1, float x)
		{
			const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
			return t * t * (3.f - 2.f * t);
		}

		// Display-space colour from 0xRRGGBB.
		sf::Glsl::Vec3 Hex(unsigned int rgb)
		{
			return sf::Glsl::Vec3(static_cast<float>((rgb >> 16) & 0xffu) / 255.f, static_cast<float>((rgb >> 8) & 0xffu) / 255.f,
				static_cast<float>(rgb & 0xffu) / 255.f);
		}

		SkyPalette Palette(unsigned int shadow, unsigned int mid, unsigned int lit, unsigned int accent)
		{
			return SkyPalette{Hex(shadow), Hex(mid), Hex(lit), Hex(accent)};
		}

		sf::Glsl::Vec3 MixColour(sf::Glsl::Vec3 a, sf::Glsl::Vec3 b, float t)
		{
			return sf::Glsl::Vec3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
		}

		// Unit length; the zero vector stays zero.
		sf::Vector3f UnitVector(sf::Vector3f v)
		{
			const float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
			return length > 0.f ? v * (1.f / length) : v;
		}

		// Display-space colour of a ramp at t (0..1), as the vec3 a shader reads.
		sf::Glsl::Vec3 RampColour(const ColorRamp& ramp, float t)
		{
			const sf::Color c = ramp.Sample(t);
			return sf::Glsl::Vec3(c.r / 255.f, c.g / 255.f, c.b / 255.f);
		}

		// Each channel multiplied by factor, then clamped to 1.
		sf::Glsl::Vec3 ScaleClamped(sf::Glsl::Vec3 c, float factor)
		{
			return sf::Glsl::Vec3(std::min(c.x * factor, 1.f), std::min(c.y * factor, 1.f), std::min(c.z * factor, 1.f));
		}

		// t = 0 gives a, t = 1 gives b.
		SkyPalette BlendPalette(const SkyPalette& a, const SkyPalette& b, float t)
		{
			return SkyPalette{MixColour(a.shadow, b.shadow, t), MixColour(a.mid, b.mid, t), MixColour(a.lit, b.lit, t), MixColour(a.accent, b.accent, t)};
		}

		astro::LinearRgb Mix(astro::LinearRgb a, astro::LinearRgb b, float t)
		{
			return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
		}

		float WrapPositive(float value, float period)
		{
			float r = std::fmod(value, period);
			return r < 0.f ? r + period : r;
		}

		// Display level of a star. Magnitudes are already logarithmic like perception, so a linear map from magnitude
		// to level is intended; minLevel keeps the faintest stars visible.
		float LevelForMagnitude(float magnitude, const StarFieldSettings& s)
		{
			// the span is floored so a validated but narrow magnitude range cannot divide by zero or flip sign
			float span = std::max(s.faintestMagnitude - (s.brightestMagnitude + 1.f), 0.25f);
			float k = std::clamp((s.faintestMagnitude - magnitude) / span, 0.f, 1.f);
			return s.minLevel + (1.f - s.minLevel) * std::pow(k, s.levelGamma);
		}

		// Brightness change in magnitudes (positive = fainter) for a variable at cycle in [0, 1).
		float VariableDelta(bool cepheid, float cycle)
		{
			if (cepheid)
			{
				// Fast rise over 20% of the period, slow decline over the rest; peak-to-peak amplitude kCepheidAmplitude.
				float b = cycle < 0.2f ? cycle / 0.2f : 1.f - (cycle - 0.2f) / 0.8f;
				return kCepheidAmplitude * (0.5f - b);
			}
			// Eclipsing binary: flat, with a dip lasting 10% of the period centred on half a cycle.
			return std::fabs(cycle - 0.5f) < 0.05f ? kEclipseDepth : 0.f;
		}

		void AddRect(std::vector<sf::Vertex>& vertices, sf::Vector2f origin, float width, float height, sf::Color color)
		{
			sf::Vector2f b = origin + sf::Vector2f(width, 0.f);
			sf::Vector2f c = origin + sf::Vector2f(0.f, height);
			sf::Vector2f d = origin + sf::Vector2f(width, height);
			for (sf::Vector2f corner : {origin, b, c, b, d, c})
			{
				vertices.push_back({corner, color});
			}
		}

		// One-pixel-wide segment as two triangles; normal must be unit length.
		void AddLine(std::vector<sf::Vertex>& vertices, sf::Vector2f from, sf::Vector2f to, sf::Vector2f normal, sf::Color color)
		{
			const sf::Vector2f half = normal * 0.5f;
			const sf::Vector2f a = from + half;
			const sf::Vector2f b = from - half;
			const sf::Vector2f c = to + half;
			const sf::Vector2f d = to - half;
			vertices.push_back({a, color});
			vertices.push_back({b, color});
			vertices.push_back({c, color});
			vertices.push_back({b, color});
			vertices.push_back({d, color});
			vertices.push_back({c, color});
		}

		// Raises the saturation of a linear colour about its luma (stylised: true blackbody colours look pale at the low
		// brightness of a star field). Channels that go past 1 pull the peak back to 1, which costs a little luma on hot stars.
		astro::LinearRgb BoostChroma(astro::LinearRgb c, float boost)
		{
			const float luma = 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
			astro::LinearRgb o{
				std::max(0.f, luma + (c.r - luma) * boost),
				std::max(0.f, luma + (c.g - luma) * boost),
				std::max(0.f, luma + (c.b - luma) * boost)};
			const float peak = std::max({o.r, o.g, o.b});
			if (peak > 1.f)
			{
				o.r /= peak;
				o.g /= peak;
				o.b /= peak;
			}
			return o;
		}

		sf::Glsl::Vec3 DisplayVec(astro::Rgb8 rgb)
		{
			return sf::Glsl::Vec3(rgb.r / 255.f, rgb.g / 255.f, rgb.b / 255.f);
		}

		sf::Color Dimmed(sf::Color color, float factor)
		{
			return sf::Color(static_cast<std::uint8_t>(color.r * factor), static_cast<std::uint8_t>(color.g * factor), static_cast<std::uint8_t>(color.b * factor), color.a);
		}

		// Unit directions of the halo segments, computed once.
		const std::array<sf::Vector2f, kHaloSegments>& HaloDirections()
		{
			static const std::array<sf::Vector2f, kHaloSegments> directions = []
			{
				std::array<sf::Vector2f, kHaloSegments> built{};
				for (int i = 0; i < kHaloSegments; ++i)
				{
					const float angle = 6.2831853f * static_cast<float>(i) / static_cast<float>(kHaloSegments);
					built[static_cast<std::size_t>(i)] = sf::Vector2f(std::cos(angle), std::sin(angle));
				}
				return built;
			}();
			return directions;
		}

		// Soft glow of a bright star, additive: full colour at the centre, 45% at 2.5 px, black at 7 px. The colour is
		// interpolated across the triangles, so the falloff is smooth.
		void AddHalo(std::vector<sf::Vertex>& vertices, sf::Vector2f centre, sf::Color peak)
		{
			constexpr float kInnerRadius = 2.5f;
			constexpr float kOuterRadius = 7.f;
			const sf::Color inner = Dimmed(peak, 0.45f);
			const sf::Color outer(0, 0, 0);
			const std::array<sf::Vector2f, kHaloSegments>& directions = HaloDirections();
			for (std::size_t i = 0; i < directions.size(); ++i)
			{
				const sf::Vector2f a = directions[i];
				const sf::Vector2f b = directions[(i + 1) % directions.size()];
				const sf::Vector2f a1 = centre + a * kInnerRadius;
				const sf::Vector2f b1 = centre + b * kInnerRadius;
				const sf::Vector2f a2 = centre + a * kOuterRadius;
				const sf::Vector2f b2 = centre + b * kOuterRadius;
				vertices.push_back({centre, peak});
				vertices.push_back({a1, inner});
				vertices.push_back({b1, inner});
				vertices.push_back({a1, inner});
				vertices.push_back({a2, outer});
				vertices.push_back({b1, inner});
				vertices.push_back({b1, inner});
				vertices.push_back({a2, outer});
				vertices.push_back({b2, outer});
			}
		}

		// Blackbody colour table, uniform in log T over the star temperature range. One Planck integration (81 samples)
		// per star per frame is too slow while moving, so the colours are computed once here.
		constexpr int kColorTableSize = 512;

		const std::array<sf::Color, kColorTableSize>& BlackbodyTable()
		{
			static const std::array<sf::Color, kColorTableSize> table = []
			{
				std::array<sf::Color, kColorTableSize> built{};
				const float logMin = std::log(astro::MinStarTemperatureK);
				const float logMax = std::log(astro::MaxStarTemperatureK);
				for (int i = 0; i < kColorTableSize; ++i)
				{
					const float u = static_cast<float>(i) / static_cast<float>(kColorTableSize - 1);
					const astro::Rgb8 rgb = astro::ToSrgb8(BoostChroma(astro::BlackbodyColor(std::exp(logMin + (logMax - logMin) * u)), kStarChromaBoost));
					built[static_cast<std::size_t>(i)] = sf::Color(rgb.r, rgb.g, rgb.b);
				}
				return built;
			}();
			return table;
		}

		// Blackbody display colour at temperatureK, linearly interpolated between table entries (8-bit channels, rounded).
		sf::Color BlackbodyColorFast(float temperatureK)
		{
			const std::array<sf::Color, kColorTableSize>& table = BlackbodyTable();
			const float logMin = std::log(astro::MinStarTemperatureK);
			const float logMax = std::log(astro::MaxStarTemperatureK);
			const float u = std::clamp((std::log(std::max(temperatureK, 1.f)) - logMin) / (logMax - logMin), 0.f, 1.f);
			const float position = u * static_cast<float>(kColorTableSize - 1);
			const int index = std::min(static_cast<int>(position), kColorTableSize - 2);
			const float fraction = position - static_cast<float>(index);
			const sf::Color& a = table[static_cast<std::size_t>(index)];
			const sf::Color& b = table[static_cast<std::size_t>(index) + 1];
			auto mix = [fraction](std::uint8_t x, std::uint8_t y) -> std::uint8_t
			{
				return static_cast<std::uint8_t>(std::lround(static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * fraction));
			};
			return sf::Color(mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b));
		}

		// Screen geometry for the ship-motion aberration (see StarField::SetMotion).
		struct MotionFrame
		{
			float beta = 0.f;
			sf::Vector2f screenCentre;
			// unit vector of the ship's motion on screen (y down)
			sf::Vector2f direction{1.f, 0.f};
			sf::Vector2f perpendicular{0.f, 1.f};
			// half of the screen's projected extent along direction, in output pixels
			float halfExtent = 1.f;
			// half of the field of view across the direction of motion, radians
			float halfFov = 0.6f;
		};

		// Moves a rest-frame screen point to where aberration puts it and returns the Doppler factor of the light there.
		float AberrateScreenPoint(const MotionFrame& m, sf::Vector2f& p)
		{
			const sf::Vector2f offset = p - m.screenCentre;
			// s in [-1, 1] along the motion; ahead of the centre is positive
			const float along = std::clamp(Dot(offset, m.direction) / m.halfExtent, -1.f, 1.f);
			// Simplification: the perpendicular coordinate is kept, so only the along-motion angle is aberrated.
			const float across = Dot(offset, m.perpendicular);
			// rest-frame angle from the motion: theta = pi/2 - s fov/2, so cos(theta) = sin(s fov/2)
			const float cosRest = std::sin(along * m.halfFov);
			const float cosObserved = relativity::AberratedCos(cosRest, m.beta);
			// inverse of the same map: s' = (pi/2 - theta') / (fov/2) = asin(cos theta') / (fov/2)
			const float alongObserved = std::asin(std::clamp(cosObserved, -1.f, 1.f)) / m.halfFov;
			p = m.screenCentre + m.direction * (alongObserved * m.halfExtent) + m.perpendicular * across;
			return relativity::DopplerFactor(cosObserved, m.beta);
		}
	}

	void DrawGradient(sf::RenderTarget& target, sf::Color top, sf::Color bottom, float maxBrightness)
	{
		sf::Vector2f size(target.getSize());
		top = LimitBrightness(top, maxBrightness);
		bottom = LimitBrightness(bottom, maxBrightness);
		sf::Vertex quad[4] = {
			{{0.f, 0.f}, top},
			{{size.x, 0.f}, top},
			{{0.f, size.y}, bottom},
			{{size.x, size.y}, bottom},
		};
		target.draw(quad, 4, sf::PrimitiveType::TriangleStrip, sf::RenderStates(sf::BlendNone));
	}

	void DrawNebula(ShaderLibrary& shaders, sf::RenderTarget& target, const ColorRamp& ramp, const NebulaParams& params)
	{
		sf::Shader* shader = shaders.Find("nebula");
		if (shader == nullptr)
		{
			return;
		}
		{
			UniformWriter uniforms(*shader);
			uniforms.Set("uScroll", params.scroll);
			uniforms.Set("uScale", params.scale);
			uniforms.Set("uIntensity", params.intensity);
			uniforms.Set("uSeed", params.seed);
			uniforms.Set("uMaxBrightness", params.maxBrightness);
		}
		SetRampUniform(*shader, "uRamp", ramp, 4);
		DrawShaderQuad(target, *shader, sf::FloatRect({0.f, 0.f}, target.getView().getSize()));
	}

	namespace
	{
		// Sets the orbit camera of planet.frag or planetring.frag. Without one uRayDistance is 0 and the shader keeps the orthographic path.
		void SetRayCamera(UniformWriter& uniforms, const std::optional<PlanetRayCamera>& camera)
		{
			const PlanetRayCamera ray = camera.value_or(PlanetRayCamera{});
			uniforms.Set("uRayCenter", sf::Glsl::Vec3(ray.centerDir));
			uniforms.Set("uRayDistance", camera ? ray.distance : 0.f);
			uniforms.Set("uViewCenter", ray.viewCenter);
			uniforms.Set("uFlow", ray.flow);
			uniforms.Set("uPixelsPerRadian", ray.pixelsPerRadian);
		}

		// Ray mode: the square around the sphere's screen centre that holds its halo. The centre's screen point inverts the sky's mapping
		// (OrbitSky::View::ScreenPoint). A wide sphere, or one whose centre is off the target, covers the whole target.
		sf::FloatRect RayPlanetArea(const sf::RenderTarget& target, const PlanetRayCamera& camera, float haloWidth)
		{
			const float reach = haloWidth * (0.14f / 0.03f);
			const float angle = std::asin(std::min((1.f + reach) / camera.distance, 1.f));
			const sf::Vector2f across{-camera.flow.y, camera.flow.x};
			const sf::Vector3f c = camera.centerDir;
			const float along = std::atan2(camera.flow.x * c.x + camera.flow.y * c.y, -c.z);
			const float aside = std::asin(std::clamp(across.x * c.x + across.y * c.y, -1.f, 1.f));
			const sf::Vector2f centre = camera.viewCenter + camera.flow * (along * camera.pixelsPerRadian) + across * (aside * camera.pixelsPerRadian);
			const sf::FloatRect view({0.f, 0.f}, target.getView().getSize());
			if (angle >= 1.2f || !view.contains(centre))
			{
				return view;
			}
			const float half = angle * camera.pixelsPerRadian + 2.f;
			const sf::Vector2f extent(half, half);
			return sf::FloatRect(centre - extent, extent * 2.f);
		}
	}

	void DrawPlanet(ShaderLibrary& shaders, sf::RenderTarget& target, const ColorRamp& ramp, const PlanetParams& params)
	{
		sf::Shader* shader = shaders.Find("planet");
		if (shader == nullptr)
		{
			return;
		}
		{
			UniformWriter uniforms(*shader);
			uniforms.Set("uCenter", params.center);
			uniforms.Set("uRadius", params.radius);
			uniforms.Set("uPixel", params.pixelScale);
			uniforms.Set("uSurfacePixel", params.surfacePixelScale);
			uniforms.Set("uLight", sf::Glsl::Vec3(params.light));
			uniforms.Set("uRotation", params.rotation);
			uniforms.Set("uSeed", params.seed);
			uniforms.Set("uDither", params.dither);
			uniforms.Set("uBrightness", params.brightness);
			uniforms.Set("uSaturation", params.saturation);
			uniforms.Set("uOpacity", params.opacity);
			uniforms.Set("uLightColor", params.lightColor);
			uniforms.Set("uLightIntensity", params.lightIntensity);
			uniforms.Set("uKind", static_cast<int>(params.kind));
			uniforms.Set("uTime", params.timeSeconds);
			uniforms.Set("uCloudAmount", params.cloudAmount);
			uniforms.Set("uAtmosphere", params.atmosphereColor);
			uniforms.Set("uSpinAxis", sf::Glsl::Vec3(params.spinAxis));
			uniforms.Set("uHalo", params.haloWidth);
			uniforms.Set("uSurfaceScale", params.surfaceScale);
			// No ring: the outer radius is 0, which switches the shadow off in planet.frag.
			uniforms.Set("uRingNormal", sf::Glsl::Vec3(params.ring ? params.ring->normal : sf::Vector3f(0.f, 0.f, 1.f)));
			uniforms.Set("uRingInner", params.ring ? params.ring->inner : 0.f);
			uniforms.Set("uRingOuter", params.ring ? params.ring->outer : 0.f);
			uniforms.Set("uRingOpacity", params.ring ? params.ring->opacity : 0.f);
			uniforms.Set("uOrbitAxis", sf::Glsl::Vec3(params.orbitAxis));
			uniforms.Set("uOrbitAngle", params.orbitAngle);
			SetRayCamera(uniforms, params.rayCamera);
		}
		SetRampUniform(*shader, "uRamp", ramp, 5);
		if (params.rayCamera)
		{
			DrawShaderQuad(target, *shader, RayPlanetArea(target, *params.rayCamera, params.haloWidth));
			return;
		}
		// The quad reaches past the limb by the halo's falloff (planet.frag kHaloExtent): 0.14 radii over a 0.03 falloff, so 1.14 at the default.
		const float reach = params.radius * (1.f + params.haloWidth * (0.14f / 0.03f));
		sf::Vector2f extent(reach, reach);
		DrawShaderQuad(target, *shader, sf::FloatRect(params.center - extent, extent * 2.f));
	}

	void DrawPlanetRing(ShaderLibrary& shaders, sf::RenderTarget& target, const ColorRamp& ramp, const PlanetParams& planet, bool front)
	{
		if (!planet.ring || (planet.rayCamera && !front))
		{
			return;
		}
		sf::Shader* shader = shaders.Find("planetring");
		if (shader == nullptr)
		{
			return;
		}
		const PlanetRing& ring = *planet.ring;
		{
			UniformWriter uniforms(*shader);
			uniforms.Set("uCenter", planet.center);
			uniforms.Set("uRadius", planet.radius);
			uniforms.Set("uNormal", sf::Glsl::Vec3(UnitVector(ring.normal)));
			uniforms.Set("uLight", sf::Glsl::Vec3(UnitVector(planet.light)));
			uniforms.Set("uInner", ring.inner);
			uniforms.Set("uOuter", ring.outer);
			uniforms.Set("uOpacity", ring.opacity * planet.opacity);
			uniforms.Set("uSeed", planet.seed);
			uniforms.Set("uFront", front ? 1.f : 0.f);
			uniforms.Set("uBrightness", planet.brightness);
			uniforms.Set("uLightColor", planet.lightColor);
			uniforms.Set("uLightIntensity", planet.lightIntensity);
			// The ring's two colours are the planet's ramp stops 2 and 4 (of 5).
			uniforms.Set("uColorA", RampColour(ramp, 0.5f));
			uniforms.Set("uColorB", RampColour(ramp, 1.f));
			SetRayCamera(uniforms, planet.rayCamera);
		}
		// Ray mode: one pass over the target, since the planet hides the far half of the ring per pixel.
		if (planet.rayCamera)
		{
			DrawShaderQuad(target, *shader, sf::FloatRect({0.f, 0.f}, target.getView().getSize()));
			return;
		}
		// The square quad spans the outer edge of the ring on both sides of the planet.
		const float reach = planet.radius * ring.outer;
		sf::Vector2f extent(reach, reach);
		DrawShaderQuad(target, *shader, sf::FloatRect(planet.center - extent, extent * 2.f));
	}

	namespace
	{
		constexpr float kTwoPi = 6.28318530718f;
		// moon orbit: radius and y scale in planet radii, one turn in seconds; spin in rad/s
		constexpr float kMoonOrbitRadius = 1.7f;
		constexpr float kMoonOrbitSquash = 0.35f;
		constexpr float kMoonRadius = 0.18f;
		constexpr float kMoonPeriodSeconds = 40.f;
		constexpr float kMoonSpin = 0.08f;

		// Well-mixed hash of a seed, in [0, 1).
		float SeedUnit(unsigned seed)
		{
			seed ^= seed >> 16;
			seed *= 0x7feb352dU;
			seed ^= seed >> 15;
			seed *= 0x846ca68bU;
			seed ^= seed >> 16;
			return static_cast<float>(seed & 0xffffffU) / 16777216.f;
		}

		// Five stops (shadow -> light), limb glow and cloud cover of one kind.
		struct PlanetLook
		{
			std::array<sf::Color, 5> stops;
			sf::Glsl::Vec3 atmosphere;
			float clouds = 0.f;
		};

		PlanetLook LookOf(PlanetKind kind)
		{
			switch (kind)
			{
			case PlanetKind::Ocean:
				return {{sf::Color(7, 27, 58), sf::Color(29, 79, 140), sf::Color(58, 160, 200), sf::Color(154, 216, 230), sf::Color(244, 251, 255)}, {0.3f, 0.6f, 1.f}, 0.6f};
			case PlanetKind::Desert:
				return {{sf::Color(74, 29, 16), sf::Color(140, 59, 30), sf::Color(192, 121, 47), sf::Color(224, 184, 116), sf::Color(246, 227, 184)}, {1.f, 0.55f, 0.25f}, 0.15f};
			case PlanetKind::Lava:
				return {{sf::Color(18, 4, 6), sf::Color(58, 10, 8), sf::Color(138, 29, 12), sf::Color(217, 83, 26), sf::Color(255, 210, 58)}, {1.f, 0.25f, 0.1f}, 0.f};
			case PlanetKind::Ice:
				return {{sf::Color(11, 45, 92), sf::Color(29, 95, 154), sf::Color(95, 184, 216), sf::Color(180, 228, 240), sf::Color(242, 251, 255)}, {0.45f, 0.9f, 1.f}, 0.4f};
			case PlanetKind::GasGiant:
				return {{sf::Color(58, 36, 24), sf::Color(138, 74, 44), sf::Color(176, 138, 92), sf::Color(220, 192, 142), sf::Color(244, 230, 196)}, {0.85f, 0.7f, 0.5f}, 0.5f};
			case PlanetKind::HotJupiter:
				return {{sf::Color(58, 10, 6), sf::Color(140, 31, 14), sf::Color(224, 86, 28), sf::Color(255, 178, 54), sf::Color(255, 233, 138)}, {1.f, 0.45f, 0.15f}, 0.35f};
			case PlanetKind::IceGiant:
				return {{sf::Color(10, 47, 63), sf::Color(19, 95, 114), sf::Color(42, 154, 168), sf::Color(127, 208, 214), sf::Color(216, 246, 245)}, {0.4f, 0.85f, 0.95f}, 0.5f};
			case PlanetKind::Terran:
			default:
				return {{sf::Color(9, 30, 62), sf::Color(24, 108, 116), sf::Color(63, 125, 74), sf::Color(185, 164, 106), sf::Color(242, 245, 240)}, {0.35f, 0.6f, 1.f}, 0.55f};
			}
		}
	}

	PlanetKind ClassifyPlanet(float radiusEarth, float eqTempK, unsigned seed)
	{
		const float roll = SeedUnit(seed);
		// Unknown radius: any kind, by seed.
		if (radiusEarth <= 0.f)
		{
			return static_cast<PlanetKind>(static_cast<int>(roll * 8.f));
		}
		if (radiusEarth >= 6.f)
		{
			if (eqTempK <= 0.f)
			{
				return roll < 0.5f ? PlanetKind::GasGiant : PlanetKind::HotJupiter;
			}
			return eqTempK > 1000.f ? PlanetKind::HotJupiter : PlanetKind::GasGiant;
		}
		if (radiusEarth >= 2.f)
		{
			if (eqTempK <= 0.f)
			{
				return roll < 0.5f ? PlanetKind::Ocean : PlanetKind::IceGiant;
			}
			return eqTempK < 250.f ? PlanetKind::IceGiant : PlanetKind::Ocean;
		}
		// Rocky: the temperature picks the surface; outside the Earth-like window a 200-400 K world is an ocean world in 30% of seeds.
		if (eqTempK <= 0.f)
		{
			constexpr PlanetKind rocky[] = {PlanetKind::Terran, PlanetKind::Ocean, PlanetKind::Desert, PlanetKind::Lava, PlanetKind::Ice};
			return rocky[static_cast<int>(roll * 5.f)];
		}
		// Earth-like window (0.8-1.25 Earth radii, 200-300 K): always Terran, no seeded Ocean roll, so the Solar System stop always
		// shows an Earth.
		if (radiusEarth >= 0.8f && radiusEarth <= 1.25f && eqTempK >= 200.f && eqTempK <= 300.f)
		{
			return PlanetKind::Terran;
		}
		if (eqTempK > 700.f)
		{
			return PlanetKind::Lava;
		}
		if (eqTempK >= 400.f)
		{
			return PlanetKind::Desert;
		}
		if (eqTempK >= 200.f)
		{
			return roll < 0.3f ? PlanetKind::Ocean : PlanetKind::Terran;
		}
		return PlanetKind::Ice;
	}

	std::optional<PlanetKind> ParsePlanetKind(const std::string& name)
	{
		constexpr std::pair<const char*, PlanetKind> kinds[] = {
			{"terran", PlanetKind::Terran}, {"ocean", PlanetKind::Ocean}, {"desert", PlanetKind::Desert}, {"lava", PlanetKind::Lava},
			{"ice", PlanetKind::Ice}, {"gasGiant", PlanetKind::GasGiant}, {"hotJupiter", PlanetKind::HotJupiter}, {"iceGiant", PlanetKind::IceGiant}};
		for (const auto& [key, kind] : kinds)
		{
			if (name == key)
			{
				return kind;
			}
		}
		return std::nullopt;
	}

	ColorRamp PlanetRamp(PlanetKind kind)
	{
		const std::array<sf::Color, 5> stops = LookOf(kind).stops;
		std::vector<ColorStop> ramp;
		for (std::size_t i = 0; i < stops.size(); ++i)
		{
			ramp.push_back({static_cast<float>(i) / 4.f, stops[i]});
		}
		return ColorRamp(std::move(ramp));
	}

	void SetPlanetKind(PlanetParams& params, PlanetKind kind)
	{
		const PlanetLook look = LookOf(kind);
		params.kind = kind;
		params.atmosphereColor = look.atmosphere;
		params.cloudAmount = look.clouds;
	}

	void DrawPlanetMoon(ShaderLibrary& shaders, sf::RenderTarget& target, const PlanetParams& planet, unsigned seed, float timeSeconds, bool front)
	{
		const float angle = SeedUnit(seed) * kTwoPi + kTwoPi * timeSeconds / kMoonPeriodSeconds;
		// The upper half of the orbit (sin < 0, y points down) is behind the planet.
		const bool behind = std::sin(angle) < 0.f;
		if (behind == front)
		{
			return;
		}
		PlanetParams moon;
		SetPlanetKind(moon, (seed & 1u) != 0u ? PlanetKind::Ice : PlanetKind::Desert);
		moon.cloudAmount = 0.f;
		moon.atmosphereColor = sf::Glsl::Vec3(0.f, 0.f, 0.f);
		const sf::Vector2f offset(std::cos(angle) * kMoonOrbitRadius, std::sin(angle) * kMoonOrbitRadius * kMoonOrbitSquash);
		moon.center = planet.center + offset * planet.radius;
		moon.radius = kMoonRadius * planet.radius;
		moon.seed = static_cast<float>(seed % 997u) + 1.f;
		moon.pixelScale = 1.f;
		moon.surfacePixelScale = 1.f;
		moon.brightness = planet.brightness;
		moon.saturation = planet.saturation;
		moon.opacity = planet.opacity;
		moon.dither = planet.dither;
		moon.light = planet.light;
		moon.lightColor = planet.lightColor;
		moon.lightIntensity = planet.lightIntensity;
		moon.rotation = timeSeconds * kMoonSpin;
		moon.timeSeconds = timeSeconds;
		DrawPlanet(shaders, target, PlanetRamp(moon.kind), moon);
	}

	float HostStarIrradiance(const HostStar& star)
	{
		float distance = std::max(star.distanceAU, 0.001f);
		return star.luminositySolar / (distance * distance);
	}

	float HostStarApparentRadius(const HostStar& star, float radiusAt1AUPixels)
	{
		return radiusAt1AUPixels * star.radiusSolar / std::max(star.distanceAU, 0.001f);
	}

	void DrawHostStar(ShaderLibrary& shaders, sf::RenderTarget& target, const HostStarParams& params)
	{
		sf::Shader* shader = shaders.Find("hoststar");
		if (shader == nullptr)
		{
			return;
		}
		// The halo must reach past the disc edge, or the fade would run backwards.
		const float glow = std::max(params.glowRadius, params.radius + 2.f);
		// Rays reach a little past the glow, never across the play area.
		const float rays = std::max(glow, std::min(glow * 1.25f, params.radius * 22.f));
		{
			UniformWriter uniforms(*shader);
			uniforms.Set("uCenter", params.center);
			uniforms.Set("uRadius", params.radius);
			uniforms.Set("uGlowRadius", glow);
			uniforms.Set("uColor", params.color);
			uniforms.Set("uIntensity", params.intensity);
			uniforms.Set("uMaxBrightness", params.maxBrightness);
			uniforms.Set("uRayLength", rays);
		}
		sf::Vector2f extent(rays, rays);
		DrawShaderQuad(target, *shader, sf::FloatRect(params.center - extent, extent * 2.f), LightAdd);
	}

	void DrawComet(ShaderLibrary& shaders, sf::RenderTarget& target, const CometParams& params)
	{
		sf::Shader* shader = shaders.Find("comet");
		if (shader == nullptr)
		{
			return;
		}
		sf::Vector2f away = params.head - params.starAt;
		const float distance = std::sqrt(away.x * away.x + away.y * away.y);
		away = distance > 1e-3f ? away / distance : sf::Vector2f(1.f, 0.f);
		{
			UniformWriter uniforms(*shader);
			uniforms.Set("uHead", params.head);
			uniforms.Set("uAway", away);
			uniforms.Set("uLength", params.length);
			uniforms.Set("uWidth", params.width);
			uniforms.Set("uBend", params.bend);
			uniforms.Set("uDustColor", sf::Glsl::Vec3(1.f, 0.82f, 0.9f));
			uniforms.Set("uIonColor", sf::Glsl::Vec3(0.35f, 0.75f, 1.f));
			uniforms.Set("uTime", params.timeSeconds);
			uniforms.Set("uMaxBrightness", params.maxBrightness);
		}
		// Bounding box of the head and both tails, padded by the widest part of the dust fan and its bend.
		const sf::Vector2f tail = params.head + away * params.length;
		const float pad = params.width * 2.2f + std::abs(params.bend) * params.length * 0.25f;
		const sf::Vector2f low(std::min(params.head.x, tail.x) - pad, std::min(params.head.y, tail.y) - pad);
		const sf::Vector2f high(std::max(params.head.x, tail.x) + pad, std::max(params.head.y, tail.y) + pad);
		DrawShaderQuad(target, *shader, sf::FloatRect(low, high - low), LightAdd);
	}

	void DrawMilkyWay(ShaderLibrary& shaders, sf::RenderTarget& target, const galaxy::SkyState& sky, const MilkyWaySettings& settings, sf::Vector2f scroll, float maxBrightness, const SkyPalette* tint)
	{
		if (!settings.enabled)
		{
			return;
		}
		sf::Shader* shader = shaders.Find("milkyway");
		if (shader == nullptr)
		{
			return;
		}
		sf::Vector2f size = target.getView().getSize();

		// Two colours of integrated starlight: old warm light (5200 K, K-type giants toward the bulge) for the clumps, and
		// young blue light (10 500 K) for the band edges and the arms. milkyway.frag mixes them by noise and position.
		const astro::LinearRgb warm = Mix(astro::BlackbodyColor(5200.f), astro::BlackbodyColor(4800.f), sky.bulgeFraction);
		const astro::LinearRgb blue = astro::BlackbodyColor(10500.f);
		// unresolved-star glow: warm, a little bluer where the arms are
		sf::Glsl::Vec3 diskColour = DisplayVec(astro::ToSrgb8(warm));
		sf::Glsl::Vec3 blueColour = DisplayVec(astro::ToSrgb8(blue));
		sf::Glsl::Vec3 glowColour = DisplayVec(astro::ToSrgb8(Mix(warm, blue, 0.3f * sky.armProximity)));
		if (tint != nullptr)
		{
			// Region colour script: the warm light moves 55% toward the region's lit colour, the blue light 55% toward its mid
			// colour scaled by 1.8, and the glow is the same warm-to-blue mix of those colours.
			constexpr float kTintMix = 0.55f;
			constexpr float kBlueTintGain = 1.8f;
			diskColour = MixColour(diskColour, tint->lit, kTintMix);
			blueColour = MixColour(blueColour, ScaleClamped(tint->mid, kBlueTintGain), kTintMix);
			glowColour = MixColour(diskColour, blueColour, 0.3f * sky.armProximity);
		}
		// H-alpha knots: H-alpha with H-beta at 1/2.86 of its strength (Case B), normalised to the brightest channel
		const astro::LinearRgb halpha = astro::WavelengthColor(astro::HAlphaNm);
		const astro::LinearRgb hbeta = astro::WavelengthColor(astro::HBetaNm);
		constexpr float kHBetaRatio = 1.f / 2.86f;
		const astro::LinearRgb pink{
			halpha.r + hbeta.r * kHBetaRatio,
			halpha.g + hbeta.g * kHBetaRatio,
			halpha.b + hbeta.b * kHBetaRatio};
		const float pinkPeak = std::max({pink.r, pink.g, pink.b});
		const astro::Rgb8 pinkRgb = astro::ToSrgb8(pink, 1.f / std::max(pinkPeak, 1e-6f));

		// Inside the bulge the light is better carried by resolved stars than by a uniform glow, which would hide them.
		// So the band widens less and dims a little with bulgeFraction, and the star-cloud modulation in
		// milkyway.frag carries the structure instead.
		const float bandGain = 1.f - 0.7f * sky.bulgeFraction;

		{
			UniformWriter uniforms(*shader);
			uniforms.Set("uViewSize", size);
			uniforms.Set("uScroll", scroll * settings.parallax);
			uniforms.Set("uAngle", settings.angleDeg * kDegToRad);
			uniforms.Set("uOffset", settings.offset * size.y);
			uniforms.Set("uWidth", settings.width * size.y * (1.f + 0.4f * sky.bulgeFraction));
			uniforms.Set("uSeed", kMilkyWaySeed);
			uniforms.Set("uCenterSide", std::min(settings.intensity * sky.bandCenterSide * bandGain, 1.5f));
			uniforms.Set("uAntiSide", std::min(settings.intensity * sky.bandAntiSide * bandGain, 1.5f));
			uniforms.Set("uDustLane", settings.dustLane * std::clamp(sky.dustDensity, 0.f, 3.f) / 3.f);
			uniforms.Set("uSkyGlow", settings.glowIntensity * sky.skyGlow);
			uniforms.Set("uArmProximity", sky.armProximity);
			uniforms.Set("uDiskColor", diskColour);
			uniforms.Set("uBlueColor", blueColour);
			uniforms.Set("uGlowColor", glowColour);
			uniforms.Set("uHAlphaColor", DisplayVec(pinkRgb));
			uniforms.Set("uMaxBrightness", maxBrightness);
		}
		DrawShaderQuad(target, *shader, sf::FloatRect({0.f, 0.f}, size));
	}

	SkyPalette SkyPaletteFor(const galaxy::SkyState& sky)
	{
		// Region palettes: near-black shadows with each region's own hue bias (no shared purple), plus the region's mid,
		// lit and accent colours. Orion spur navy with ice blue, the star-forming arms warm black with rose and teal,
		// the outer arms teal-black with cyan, the interarm plain deep blue-black, the bulge brown-black with amber, the
		// core warm black with cream and gold.
		const SkyPalette orion = Palette(0x040915, 0x1d3a6e, 0x9cc8ff, 0xffc070);
		const SkyPalette crimson = Palette(0x0d0609, 0x6a1e2a, 0xff6a6a, 0x3fe0d0);
		const SkyPalette cool = Palette(0x030d12, 0x14505a, 0x6fe0e0, 0xffb060);
		const SkyPalette interarm = Palette(0x02040b, 0x16244a, 0x8aa8e0, 0xe0c090);
		const SkyPalette bulge = Palette(0x0e0804, 0x6a3a14, 0xffb050, 0xff6a4a);
		const SkyPalette core = Palette(0x0b0806, 0x5a4020, 0xffe0b0, 0x7ad0ff);

		SkyPalette arm = interarm;
		switch (sky.nearestArm)
		{
		case galaxy::Arm::LocalOrion:
			arm = orion;
			break;
		case galaxy::Arm::SagittariusCarina:
		case galaxy::Arm::ScutumCentaurus:
			arm = crimson;
			break;
		case galaxy::Arm::Norma3Kpc:
		case galaxy::Arm::Perseus:
		case galaxy::Arm::Outer:
			arm = cool;
			break;
		default:
			break;
		}
		// Off the arm ridge the interarm colour takes over; that also hides the switch between two arms' colours.
		const SkyPalette disk = BlendPalette(interarm, arm, SmoothStep(0.2f, 0.6f, sky.armProximity));
		const SkyPalette withBulge = BlendPalette(disk, bulge, SmoothStep(0.2f, 0.6f, sky.bulgeFraction));
		return BlendPalette(withBulge, core, SmoothStep(0.75f, 1.f, sky.bulgeFraction));
	}

	void DrawSkyPaint(ShaderLibrary& shaders, sf::RenderTarget& nebulaTarget, const SkyPaintParams& params)
	{
		sf::Shader* shader = shaders.Find("skypaint");
		if (shader == nullptr)
		{
			return;
		}
		const sf::Vector2f size = nebulaTarget.getView().getSize();
		{
			UniformWriter uniforms(*shader);
			uniforms.Set("uViewSize", size);
			uniforms.Set("uLight", params.lightPos);
			uniforms.Set("uScroll0", params.scroll * params.farParallax);
			uniforms.Set("uScroll1", params.scroll * params.midParallax);
			uniforms.Set("uScroll2", params.scroll * params.nearParallax);
			uniforms.Set("uShadow", params.palette.shadow);
			uniforms.Set("uMid", params.palette.mid);
			uniforms.Set("uLit", params.palette.lit);
			uniforms.Set("uAccent", params.palette.accent);
			uniforms.Set("uCoverage", params.coverage);
			uniforms.Set("uIntensity", params.intensity);
			uniforms.Set("uCenterCalm", params.centerCalm);
			uniforms.Set("uTime", params.timeSeconds);
			uniforms.Set("uSeed", params.seed);
			uniforms.Set("uMaxBrightness", params.maxBrightness);
		}
		DrawShaderQuad(nebulaTarget, *shader, sf::FloatRect({0.f, 0.f}, size), PremultipliedOver);
	}

	StarField::StarField(unsigned seed)
		: mSeed(seed)
	{
	}

	void StarField::Build(const galaxy::SkyState& sky, const StarFieldSettings& stars, const MilkyWaySettings& band)
	{
		mStarSettings = stars;
		mBand = band;
		mStars.clear();

		std::mt19937 rng(mSeed);
		std::uniform_real_distribution<float> unit(0.f, 1.f);
		std::normal_distribution<float> gaussian(0.f, 1.f);

		// denser sky: more stars, and brighter on average
		const float logDensity = std::log10(std::max(1.f, sky.starDensity));
		const int count = std::min(stars.maxCount, static_cast<int>(std::lround(stars.baseCount * (1.f + 0.25f * logDensity))));
		const float magnitudeOffset = -0.6f * logDensity;
		// the bulge concentrates stars around the band as well
		const float bandShare = stars.bandFraction + (1.f - stars.bandFraction) * 0.5f * sky.bulgeFraction;
		const float bandWidth = std::max(band.width, 0.01f);
		const float bandSigma = bandWidth * (1.f + 3.f * sky.bulgeFraction);
		const float dustScale = 2.5f * band.dustLane * std::min(sky.dustDensity, 3.f) / 3.f;

		mStars.reserve(static_cast<std::size_t>(count));
		for (int i = 0; i < count; ++i)
		{
			Star star;
			star.magnitude = astro::SampleMagnitude(unit(rng), stars.brightestMagnitude, stars.faintestMagnitude, stars.magnitudeSlope) + magnitudeOffset;
			const float uClass = unit(rng);
			const float uTemp = unit(rng);
			float temperature = astro::SampleStarTemperature(sky.stellarMix, uClass, uTemp);

			const bool inBand = unit(rng) < bandShare;
			star.band = band.enabled && inBand;
			if (star.band)
			{
				star.unit.x = unit(rng);
				star.across = gaussian(rng) * bandSigma;
				// dust lane: extinction near the band centre, reddening approximated by a temperature drop
				// (real interstellar extinction scales roughly with 1/wavelength)
				float dustCoord = star.across / (0.35f * bandWidth);
				float extinction = dustScale * std::exp(-dustCoord * dustCoord);
				star.magnitude += extinction;
				temperature /= 1.f + 0.15f * extinction;
				star.parallax = band.parallax;
			}
			else
			{
				star.unit = {unit(rng), unit(rng)};
				star.parallax = unit(rng) < stars.nearFraction ? stars.nearParallax : stars.farParallax * (0.6f + 0.8f * unit(rng));
			}

			star.level = LevelForMagnitude(star.magnitude, stars);
			star.temperatureK = temperature;
			// sRGB is converted once here; Draw runs every frame
			astro::Rgb8 rgb = astro::ToSrgb8(BoostChroma(astro::BlackbodyColor(temperature), kStarChromaBoost));
			star.color = sf::Color(rgb.r, rgb.g, rgb.b);
			if (star.magnitude < stars.spikeMagnitude)
			{
				star.size = StarSize::Spiked;
			}
			else if (star.magnitude < stars.spikeMagnitude + 1.5f)
			{
				star.size = StarSize::Core;
			}

			if (unit(rng) < stars.variableFraction)
			{
				if (unit(rng) < 0.6f)
				{
					star.variability = Variability::Cepheid;
					star.period = 4.f + 8.f * unit(rng);
				}
				else
				{
					star.variability = Variability::Eclipsing;
					star.period = 6.f + 14.f * unit(rng);
				}
				star.phase = unit(rng);
			}
			mStars.push_back(star);
		}

		// The brightest ~2% of stars get a halo (see Draw). Magnitudes here are before variability, which is fixed per star.
		if (!mStars.empty())
		{
			std::vector<float> magnitudes;
			magnitudes.reserve(mStars.size());
			for (const Star& star : mStars)
			{
				magnitudes.push_back(star.magnitude);
			}
			const std::size_t haloCount = std::max<std::size_t>(1, mStars.size() / 50);
			std::nth_element(magnitudes.begin(), magnitudes.begin() + static_cast<std::ptrdiff_t>(haloCount - 1), magnitudes.end());
			const float threshold = magnitudes[haloCount - 1];
			for (Star& star : mStars)
			{
				star.halo = star.magnitude <= threshold;
			}
		}
	}

	void StarField::AddCluster(sf::Vector2f centre, float radius, bool open, float aspect, unsigned seed)
	{
		std::mt19937 rng(seed ^ 0x5bd1e995u);
		std::uniform_real_distribution<float> unit(0.f, 1.f);
		const int count = open ? 60 : 900;
		// Plummer scale radius: half the light within about 1.3 a; the crowd is cut at the cluster radius.
		const float scale = radius * (open ? 0.22f : 0.12f);
		const float parallax = mStarSettings.farParallax;
		for (int i = 0; i < count; ++i)
		{
			float r = 0.f;
			for (int tries = 0; tries < 6; ++tries)
			{
				const float u = std::clamp(unit(rng), 1e-4f, 0.999f);
				r = scale / std::sqrt(std::pow(u, -2.f / 3.f) - 1.f);
				if (r <= radius)
				{
					break;
				}
			}
			r = std::min(r, radius);
			const float angle = unit(rng) * 6.2831853f;
			Star star;
			star.unit = {centre.x + std::cos(angle) * r / std::max(aspect, 0.1f), centre.y + std::sin(angle) * r};
			star.parallax = parallax;
			// Steep luminosity function: a crowd of faint stars, a handful of bright ones.
			// Open clusters are known by a handful of bright blue-white stars (the Pleiades look), so the first eight are
			// bright; globulars by a dense crowd that resolves into grain.
			star.magnitude = open && i < 8 ? -2.5f + 3.f * unit(rng)
				: astro::SampleMagnitude(unit(rng), open ? 1.0f : -0.5f, open ? 5.0f : 5.0f, open ? 0.35f : 0.55f);
			const float roll = unit(rng);
			float temperature;
			if (open)
			{
				// young: the bright ones hot and blue-white, the rest white to yellow, a rare red supergiant
				temperature = star.magnitude < 2.5f ? 11000.f + 14000.f * unit(rng) : (roll < 0.05f ? 3700.f : 6000.f + 4000.f * unit(rng));
			}
			else
			{
				// old: red and orange giants on top, yellow-white turn-off stars below, a few blue horizontal-branch stars
				temperature = star.magnitude < 3.0f ? 3900.f + 800.f * unit(rng) : (roll < 0.06f ? 8500.f + 2000.f * unit(rng) : 5200.f + 1400.f * unit(rng));
			}
			star.temperatureK = temperature;
			star.level = LevelForMagnitude(star.magnitude, mStarSettings);
			const astro::Rgb8 rgb = astro::ToSrgb8(BoostChroma(astro::BlackbodyColor(temperature), kStarChromaBoost));
			star.color = sf::Color(rgb.r, rgb.g, rgb.b);
			star.size = star.magnitude < mStarSettings.spikeMagnitude + 1.5f ? StarSize::Core : StarSize::Point;
			star.halo = star.magnitude < (open ? 1.5f : 2.0f);
			mStars.push_back(star);
		}
	}

	void StarField::SetMotion(float beta, sf::Vector2f direction, float fieldOfViewRad)
	{
		mBeta = std::clamp(beta, 0.f, relativity::kMaxBeta);
		const float norm = std::sqrt(direction.x * direction.x + direction.y * direction.y);
		mDirection = norm > 1e-6f ? direction / norm : sf::Vector2f(1.f, 0.f);
		mFov = std::clamp(fieldOfViewRad, 0.05f, 3.f);
	}

	void StarField::Vary(const Star& star, float timeSeconds, float& magnitude, float& level) const
	{
		if (star.variability == Variability::None)
		{
			return;
		}
		float x = timeSeconds / star.period + star.phase;
		magnitude = star.magnitude + VariableDelta(star.variability == Variability::Cepheid, x - std::floor(x));
		level = LevelForMagnitude(magnitude, mStarSettings);
	}

	void StarField::Emit(const Star& star, sf::Vector2f p, float magnitude, float level, sf::Vector2f size, float maxBrightness) const
	{
		// Ship motion: the screen is a patch of sky seen side-on, so the coordinate along the motion is the one that
		// aberrates. Built per star, since a resting ship skips all of it.
		const bool moving = mBeta > kMotionThreshold;
		MotionFrame motion;
		if (moving)
		{
			motion.beta = mBeta;
			motion.screenCentre = size * 0.5f;
			motion.direction = mDirection;
			motion.perpendicular = sf::Vector2f(-mDirection.y, mDirection.x);
			motion.halfExtent = std::max(0.5f * (std::fabs(mDirection.x) * size.x + std::fabs(mDirection.y) * size.y), 1.f);
			motion.halfFov = 0.5f * mFov;
		}
		// aberration moves the star on screen; the same observed angle gives its Doppler factor
		float doppler = 1.f;
		if (moving)
		{
			doppler = AberrateScreenPoint(motion, p);
		}
		p.x = std::floor(p.x);
		p.y = std::floor(p.y);
		if (p.x < 0.f || p.y < 0.f || p.x >= size.x || p.y >= size.y)
		{
			return;
		}

		sf::Color color = star.color;
		if (moving)
		{
			// Beaming: observed flux scales with D^4 (magnitude -10 log10 D). Doppler shift of a blackbody is T -> D T,
			// so its colour follows from BlackbodyColor; ahead is bluer, behind redder.
			magnitude += relativity::BeamingMagnitudeDelta(doppler);
			level = LevelForMagnitude(magnitude, mStarSettings);
			color = BlackbodyColorFast(star.temperatureK * doppler);
		}

		sf::Color core = LimitBrightness(color, maxBrightness * level);
		AddRect(mVertices, p, star.size == StarSize::Point ? 1.f : 2.f, star.size == StarSize::Point ? 1.f : 2.f, core);
		if (star.halo)
		{
			// about 35% of the star level, tinted by the star colour; centred on the core rect
			AddHalo(mVertices, p + sf::Vector2f(1.f, 1.f), LimitBrightness(color, maxBrightness * level * 0.35f));
		}
		if (moving)
		{
			// Stylised streak, not a static relativistic effect: it stands in for the aberration changing while the
			// ship accelerates. It trails opposite to the motion at 40% level.
			const sf::Vector2f tail = p - motion.direction * (mBeta * kStreakPixelsPerBeta);
			AddLine(mVertices, p, tail, motion.perpendicular, LimitBrightness(color, maxBrightness * level * 0.4f));
		}
		if (star.size == StarSize::Spiked)
		{
			sf::Color spike = LimitBrightness(color, maxBrightness * level * 0.4f);
			AddRect(mVertices, p - sf::Vector2f(3.f, 0.f), 3.f, 1.f, spike);
			AddRect(mVertices, p + sf::Vector2f(2.f, 0.f), 3.f, 1.f, spike);
			AddRect(mVertices, p - sf::Vector2f(0.f, 3.f), 1.f, 3.f, spike);
			AddRect(mVertices, p + sf::Vector2f(0.f, 2.f), 1.f, 3.f, spike);
		}
	}

	void StarField::Draw(sf::RenderTarget& target, sf::Vector2f scroll, float timeSeconds, float maxBrightness) const
	{
		sf::Vector2f size(target.getSize());
		if (mStars.empty() || size.x <= 0.f || size.y <= 0.f)
		{
			return;
		}

		// the Milky Way band runs through the screen centre, offset perpendicular to its direction
		const float angle = mBand.angleDeg * kDegToRad;
		const sf::Vector2f along(std::cos(angle), std::sin(angle));
		const sf::Vector2f across(-along.y, along.x);
		const float length = std::sqrt(size.x * size.x + size.y * size.y);
		const sf::Vector2f centre = size * 0.5f + across * (mBand.offset * size.y);
		const sf::Vector2f bandShift = scroll * mBand.parallax;

		mVertices.clear();
		for (const Star& star : mStars)
		{
			float magnitude = star.magnitude;
			float level = star.level;
			Vary(star, timeSeconds, magnitude, level);

			sf::Vector2f p;
			if (star.band)
			{
				float s = WrapPositive(star.unit.x * length - Dot(bandShift, along), length);
				p = centre + along * (s - length * 0.5f) + across * (star.across * size.y - Dot(bandShift, across));
			}
			else
			{
				p.x = WrapPositive(star.unit.x * size.x - scroll.x * star.parallax, size.x);
				p.y = WrapPositive(star.unit.y * size.y - scroll.y * star.parallax, size.y);
			}
			Emit(star, p, magnitude, level, size, maxBrightness);
		}
		if (!mVertices.empty())
		{
			// starlight adds to what is behind it; alpha blending would let dim stars darken the band
			target.draw(mVertices.data(), mVertices.size(), sf::PrimitiveType::Triangles, sf::RenderStates(sf::BlendAdd));
		}
	}

	void StarField::DrawRigid(sf::RenderTarget& target, sf::Vector2f offset, float timeSeconds, float maxBrightness) const
	{
		sf::Vector2f size(target.getSize());
		if (mStars.empty() || size.x <= 0.f || size.y <= 0.f)
		{
			return;
		}

		mVertices.clear();
		for (const Star& star : mStars)
		{
			if (star.band)
			{
				continue;
			}
			float magnitude = star.magnitude;
			float level = star.level;
			Vary(star, timeSeconds, magnitude, level);

			// the whole field turns by one offset and wraps over the target
			sf::Vector2f p;
			p.x = WrapPositive(star.unit.x * size.x + offset.x, size.x);
			p.y = WrapPositive(star.unit.y * size.y + offset.y, size.y);
			Emit(star, p, magnitude, level, size, maxBrightness);
		}
		if (!mVertices.empty())
		{
			// starlight adds to what is behind it; alpha blending would let dim stars darken the band
			target.draw(mVertices.data(), mVertices.size(), sf::PrimitiveType::Triangles, sf::RenderStates(sf::BlendAdd));
		}
	}
}
