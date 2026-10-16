#include "sky/Nebula.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "sky/Astro.h"
#include "sky/UniformWriter.h"

namespace vega::sky
{
	namespace
	{
		constexpr float kTwoPi = 6.2831853f;
		// Case B recombination: H-alpha / H-beta = 2.86 at ~10 000 K.
		constexpr float kHBetaWeight = 1.f / 2.86f;
		constexpr float kNIIWeight = 0.3f;
		constexpr float kSIIWeight = 0.25f;
		// [O III] weight at full ionization. [O III] 500.7 nm is forbidden-line emission from O++; it needs ~35 eV.
		// In high-excitation HII regions [O III] 500.7 / H-beta reaches ~3-10, so at full excitation the line is about
		// twice H-alpha in the core: the core reads teal-green, the outer shell (H-alpha, [N II], [S II]) red.
		constexpr float kOIIIWeight = 2.1f;
		constexpr float kOIIILowK = 30000.f;
		constexpr float kOIIIHighK = 45000.f;
		// Quad half-extent in radii: the warped envelope reaches ~1.15 along its long axis, and the shader's square window
		// makes the emission exactly zero by 1.5 radii, so the quad edge never shows.
		constexpr float kEmissionExtent = 1.6f;
		// Young cluster: stars sit inside the [O III] core, within this fraction of the Stromgren radius.
		constexpr float kClusterRadius = 0.12f;
		// Quad half-extent in cloud half-diagonals (any rotation fits in a square of this size).
		constexpr float kDustExtent = 1.05f;
		// Small grains scatter roughly as lambda^-1.5. Per-channel gains relative to red, approximating that for
		// R, G, B (650, 540, 450 nm) and normalized to the brightest channel below.
		constexpr float kReflectionGain[3] = {1.f, 1.25f, 1.6f};

		// The shader writes premultiplied (colour, max channel). Emission adds light, so colour and alpha both add.
		// sf::BlendAdd would multiply the colour by its alpha a second time (colour^2), so the factors are spelled out.
		const sf::BlendMode EmissionBlend(sf::BlendMode::Factor::One, sf::BlendMode::Factor::One, sf::BlendMode::Equation::Add,
			sf::BlendMode::Factor::One, sf::BlendMode::Factor::One, sf::BlendMode::Equation::Add);

		float SmoothStep(float edge0, float edge1, float x)
		{
			const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
			return t * t * (3.f - 2.f * t);
		}

		// Display-space (sRGB, 0..1) colour of a spectral line; the line colour is already normalized to its brightest
		// channel by WavelengthColor.
		sf::Glsl::Vec3 LineColor(float nm)
		{
			const astro::Rgb8 rgb = astro::ToSrgb8(astro::WavelengthColor(nm));
			return sf::Glsl::Vec3(rgb.r / 255.f, rgb.g / 255.f, rgb.b / 255.f);
		}

		// Line colours for one palette, display space.
		struct LineSet
		{
			sf::Glsl::Vec3 halpha;
			sf::Glsl::Vec3 hbeta;
			sf::Glsl::Vec3 oiii;
			sf::Glsl::Vec3 nii;
			sf::Glsl::Vec3 sii;
		};

		LineSet ResolveLines(NebulaPalette palette)
		{
			if (palette == NebulaPalette::Narrowband)
			{
				// Hubble palette (SHO), the HST false-colour convention: [S II] -> red, H-alpha -> green, [O III] -> blue.
				// [N II] falls inside the H-alpha filter band, so SHO images carry it in the green channel; it is set to
				// zero here so that the ionization front stays the gold-red of [S II] (the green is the H-alpha zone).
				// H-beta is not in the SHO set, so it is zero too.
				LineSet set;
				set.halpha = sf::Glsl::Vec3(0.2f, 1.f, 0.4f);
				set.hbeta = sf::Glsl::Vec3(0.f, 0.f, 0.f);
				set.oiii = sf::Glsl::Vec3(0.1f, 0.55f, 1.f);
				set.nii = sf::Glsl::Vec3(0.f, 0.f, 0.f);
				set.sii = sf::Glsl::Vec3(1.f, 0.35f, 0.1f);
				return set;
			}
			LineSet set;
			set.halpha = LineColor(astro::HAlphaNm);
			set.hbeta = LineColor(astro::HBetaNm);
			set.oiii = LineColor(astro::OIIINm);
			set.nii = LineColor(astro::NIINm);
			set.sii = LineColor(astro::SIINm);
			return set;
		}

		// Deterministic value in [0, 1) from the seed: the same cluster and pillars every frame for the same seed.
		float Unit(float seed, int index, int channel)
		{
			const double x = std::sin(static_cast<double>(seed) * 12.9898 + index * 78.233 + channel * 37.719) * 43758.5453;
			return static_cast<float>(x - std::floor(x));
		}

		// Up to four elephant-trunk pillars (ray angles from the star), two to four of them, spread around it with jitter.
		struct PillarSet
		{
			float count = 2.f;
			sf::Glsl::Vec4 angles;
		};

		PillarSet MakePillars(float seed)
		{
			const int count = 2 + static_cast<int>(Unit(seed, 0, 2) * 3.f);
			const float base = Unit(seed, 0, 3) * kTwoPi;
			const float spacing = kTwoPi / static_cast<float>(count);
			float angles[4] = {0.f, 0.f, 0.f, 0.f};
			for (int i = 0; i < count; ++i)
			{
				angles[i] = base + static_cast<float>(i) * spacing + (Unit(seed, i + 1, 4) - 0.5f) * 0.5f * spacing;
			}
			PillarSet set;
			set.count = static_cast<float>(count);
			set.angles = sf::Glsl::Vec4(angles[0], angles[1], angles[2], angles[3]);
			return set;
		}

		// Scattered-light colour: the illuminator's blackbody, weighted toward blue, normalized to its brightest channel.
		sf::Glsl::Vec3 ReflectionColor(float temperatureK)
		{
			astro::LinearRgb lit = astro::BlackbodyColor(temperatureK);
			lit.r *= kReflectionGain[0];
			lit.g *= kReflectionGain[1];
			lit.b *= kReflectionGain[2];
			const float peak = std::max({lit.r, lit.g, lit.b});
			if (peak > 0.f)
			{
				lit.r /= peak;
				lit.g /= peak;
				lit.b /= peak;
			}
			const astro::Rgb8 rgb = astro::ToSrgb8(lit);
			return sf::Glsl::Vec3(rgb.r / 255.f, rgb.g / 255.f, rgb.b / 255.f);
		}

		// Premultiplied vertex colour for the additive nebula target: alpha is the max channel, as the shader writes.
		sf::Color Premultiplied(float r, float g, float b)
		{
			const auto to8 = [](float v) { return static_cast<std::uint8_t>(std::clamp(v, 0.f, 1.f) * 255.f + 0.5f); };
			const std::uint8_t r8 = to8(r);
			const std::uint8_t g8 = to8(g);
			const std::uint8_t b8 = to8(b);
			return sf::Color(r8, g8, b8, std::max({r8, g8, b8}));
		}

		void AddQuad(std::vector<sf::Vertex>& vertices, sf::Vector2f a, sf::Vector2f b, sf::Color color)
		{
			const sf::Vector2f c0(a.x, a.y);
			const sf::Vector2f c1(b.x, a.y);
			const sf::Vector2f c2(b.x, b.y);
			const sf::Vector2f c3(a.x, b.y);
			vertices.push_back(sf::Vertex{c0, color});
			vertices.push_back(sf::Vertex{c1, color});
			vertices.push_back(sf::Vertex{c3, color});
			vertices.push_back(sf::Vertex{c1, color});
			vertices.push_back(sf::Vertex{c2, color});
			vertices.push_back(sf::Vertex{c3, color});
		}

		// Young cluster at the heart of the region: 6-14 hot O/B stars (20 000-40 000 K blackbody colour), each a one
		// pixel point, a 1 px four-pointed sparkle and a soft 6-10 px glow. They ionize the gas, so they are drawn into
		// the same target after it. One vertex batch.
		void DrawClusterStars(sf::RenderTarget& target, const EmissionNebulaParams& params)
		{
			const int count = 6 + static_cast<int>(Unit(params.seed, 0, 0) * 9.f);
			constexpr int kGlowSegments = 12;
			std::vector<sf::Vertex> vertices;
			vertices.reserve(static_cast<std::size_t>(count) * (6 + 12 + kGlowSegments * 3));

			for (int i = 0; i < count; ++i)
			{
				const float angle = Unit(params.seed, i, 1) * kTwoPi;
				const float dist = params.radius * kClusterRadius * std::sqrt(Unit(params.seed, i, 2));
				const sf::Vector2f at(std::floor(params.center.x + std::cos(angle) * dist),
					std::floor(params.center.y + std::sin(angle) * dist));

				// Blackbody, hotter than the ionizing star's class range: bluish-white. Display-space like the gas.
				const float temperatureK = 20000.f + 20000.f * Unit(params.seed, i, 3);
				const astro::Rgb8 rgb = astro::ToSrgb8(astro::BlackbodyColor(temperatureK));
				const float level = 0.6f + 0.4f * Unit(params.seed, i, 4);
				const float bright = params.maxBrightness * level;
				const float cr = rgb.r / 255.f;
				const float cg = rgb.g / 255.f;
				const float cb = rgb.b / 255.f;

				// Point: one pixel at full level.
				AddQuad(vertices, at, at + sf::Vector2f(1.f, 1.f), Premultiplied(cr * bright, cg * bright, cb * bright));

				// Sparkle: a 1 px cross with 2-4 px arms, at 40% of the point.
				const float arm = 2.f + 2.f * Unit(params.seed, i, 5);
				const sf::Color spike = Premultiplied(cr * bright * 0.4f, cg * bright * 0.4f, cb * bright * 0.4f);
				AddQuad(vertices, at - sf::Vector2f(arm, 0.f), at + sf::Vector2f(arm + 1.f, 1.f), spike);
				AddQuad(vertices, at - sf::Vector2f(0.f, arm), at + sf::Vector2f(1.f, arm + 1.f), spike);

				// Glow: a triangle fan from the centre (35%) to a transparent rim, so the falloff is linear and soft.
				const float glowRadius = 6.f + 4.f * Unit(params.seed, i, 6);
				const sf::Color glowCore = Premultiplied(cr * bright * 0.35f, cg * bright * 0.35f, cb * bright * 0.35f);
				const sf::Color glowEdge(0, 0, 0, 0);
				const sf::Vector2f centre = at + sf::Vector2f(0.5f, 0.5f);
				for (int s = 0; s < kGlowSegments; ++s)
				{
					const float a0 = kTwoPi * static_cast<float>(s) / static_cast<float>(kGlowSegments);
					const float a1 = kTwoPi * static_cast<float>(s + 1) / static_cast<float>(kGlowSegments);
					const sf::Vector2f p0 = centre + sf::Vector2f(std::cos(a0), std::sin(a0)) * glowRadius;
					const sf::Vector2f p1 = centre + sf::Vector2f(std::cos(a1), std::sin(a1)) * glowRadius;
					vertices.push_back(sf::Vertex{centre, glowCore});
					vertices.push_back(sf::Vertex{p0, glowEdge});
					vertices.push_back(sf::Vertex{p1, glowEdge});
				}
			}
			if (!vertices.empty())
			{
				target.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Triangles, sf::RenderStates(EmissionBlend));
			}
		}
	}

	void DrawEmissionNebula(ShaderLibrary& shaders, sf::RenderTarget& nebulaTarget, const EmissionNebulaParams& params)
	{
		sf::Shader* shader = shaders.Find("emission");
		if (shader == nullptr || !(params.radius > 0.f))
		{
			return;
		}
		const float oiiiShare = SmoothStep(kOIIILowK, kOIIIHighK, params.ionizingTemperatureK);
		const sf::Vector2f shift = params.scroll * params.parallax;
		const LineSet lines = ResolveLines(params.palette);
		const PillarSet pillars = MakePillars(params.seed);
		{
			UniformWriter uniforms(*shader);
			uniforms.Set("uScroll", shift);
			uniforms.Set("uCenter", params.center);
			uniforms.Set("uRadius", params.radius);
			uniforms.Set("uSeed", params.seed);
			uniforms.Set("uTime", params.timeSeconds);
			uniforms.Set("uDensity", params.density);
			uniforms.Set("uSaturation", params.saturation);
			uniforms.Set("uMaxBrightness", params.maxBrightness);
			uniforms.Set("uLineWeights", sf::Glsl::Vec4(1.f, kHBetaWeight, kNIIWeight, kSIIWeight));
			uniforms.Set("uOIIIWeight", kOIIIWeight * oiiiShare);
			uniforms.Set("uPillarCount", pillars.count);
			uniforms.Set("uPillarAngles", pillars.angles);
			uniforms.Set("uHalpha", lines.halpha);
			uniforms.Set("uHbeta", lines.hbeta);
			uniforms.Set("uOIII", lines.oiii);
			uniforms.Set("uNII", lines.nii);
			uniforms.Set("uSII", lines.sii);
		}
		// The shader samples at position + shift, so the quad is moved back by shift to cover the nebula.
		const sf::Vector2f extent(params.radius * kEmissionExtent, params.radius * kEmissionExtent);
		DrawShaderQuad(nebulaTarget, *shader, sf::FloatRect(params.center - shift - extent, extent * 2.f), EmissionBlend);
		DrawClusterStars(nebulaTarget, params);
	}

	void DrawDustCloud(ShaderLibrary& shaders, sf::RenderTarget& backgroundTarget, const DustCloudParams& params)
	{
		sf::Shader* shader = shaders.Find("dust");
		if (shader == nullptr || !(params.size.x > 0.f) || !(params.size.y > 0.f))
		{
			return;
		}
		const sf::Vector2f shift = params.scroll * params.parallax;
		const sf::Vector2f halfSize = params.size * 0.5f;
		const sf::Vector2f axis(std::cos(params.angleRad), std::sin(params.angleRad));
		const sf::Glsl::Vec3 reflectColor = ReflectionColor(params.illuminatorTemperatureK);

		// Writes every uniform for one pass; the draw happens after the writer is gone, as in Backdrop.
		auto setPass = [&](float pass)
		{
			UniformWriter uniforms(*shader);
			uniforms.Set("uPass", pass);
			uniforms.Set("uScroll", shift);
			uniforms.Set("uCenter", params.center);
			uniforms.Set("uHalfSize", halfSize);
			uniforms.Set("uAxis", axis);
			uniforms.Set("uOpticalDepth", params.opticalDepth);
			uniforms.Set("uReflection", params.reflection);
			uniforms.Set("uIlluminator", params.illuminator);
			uniforms.Set("uReflectColor", reflectColor);
			uniforms.Set("uSeed", params.seed);
			uniforms.Set("uMaxBrightness", params.maxBrightness);
			uniforms.Set("uPixelSize", params.pixelSize);
			uniforms.Set("uStylize", params.stylize ? 1.f : 0.f);
		};

		const float extentValue = std::sqrt(halfSize.x * halfSize.x + halfSize.y * halfSize.y) * kDustExtent;
		const sf::Vector2f extent(extentValue, extentValue);
		const sf::FloatRect rect(params.center - shift - extent, extent * 2.f);

		// Pass 0: extinction multiplies what is behind the cloud.
		setPass(0.f);
		DrawShaderQuad(backgroundTarget, *shader, rect, sf::BlendMultiply);
		// Pass 1: scattered starlight is added on top.
		setPass(1.f);
		DrawShaderQuad(backgroundTarget, *shader, rect, sf::BlendAdd);
	}
}
