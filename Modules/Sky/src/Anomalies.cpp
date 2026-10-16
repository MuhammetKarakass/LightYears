#include "sky/Anomalies.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

#include "sky/ColorRamp.h"
#include "sky/UniformWriter.h"

namespace vega::sky
{
	namespace
	{
		constexpr float kTwoPi = 6.28318530718f;
		// Angular speed at the orbit radius (rad/s); scaled by (r / orbit)^-1.5 per chunk.
		constexpr float kBaseOmega = 0.05f;
		// Debris starts in a 60-degree arc (radians).
		constexpr float kArcWidth = 1.0471976f;
		// Share of the albedo left on the dark side of the terminator.
		constexpr float kShadeDark = 0.35f;
		constexpr int kDustPoints = 220;
		constexpr std::size_t kMaxChunks = 400;

		sf::Color Scale(sf::Color color, float k)
		{
			auto channel = [k](std::uint8_t c) { return static_cast<std::uint8_t>(std::clamp(c * k, 0.f, 255.f)); };
			return sf::Color(channel(color.r), channel(color.g), channel(color.b), color.a);
		}

		// Deterministic hash in [0,1) per index; the dust is re-derived each frame, so it keeps no random state.
		float HashUnit(int index)
		{
			float x = std::sin(static_cast<float>(index) * 12.9898f) * 43758.5453f;
			return x - std::floor(x);
		}

		float Dot(sf::Vector2f a, sf::Vector2f b)
		{
			return a.x * b.x + a.y * b.y;
		}

		// Triangle (centre, a, b) split by the terminator, which is the line through the centre along the light. Because
		// the split line passes through the fan centre, the triangle yields at most two pieces.
		void AddSplitTriangle(std::vector<sf::Vertex>& out, sf::Vector2f c, sf::Vector2f a, sf::Vector2f b,
			sf::Vector2f light, sf::Color lit, sf::Color dark)
		{
			const float da = Dot(a - c, light);
			const float db = Dot(b - c, light);
			if (da >= 0.f && db >= 0.f)
			{
				out.push_back({c, lit});
				out.push_back({a, lit});
				out.push_back({b, lit});
			}
			else if (da < 0.f && db < 0.f)
			{
				out.push_back({c, dark});
				out.push_back({a, dark});
				out.push_back({b, dark});
			}
			else
			{
				// da and db have opposite signs here, so da - db is never zero.
				const sf::Vector2f q = a + (b - a) * (da / (da - db));
				const bool aLit = da >= 0.f;
				const sf::Color aColor = aLit ? lit : dark;
				const sf::Color bColor = aLit ? dark : lit;
				out.push_back({c, aColor});
				out.push_back({a, aColor});
				out.push_back({q, aColor});
				out.push_back({c, bColor});
				out.push_back({q, bColor});
				out.push_back({b, bColor});
			}
		}
	}

	void DrawPulsar(ShaderLibrary& shaders, sf::RenderTarget& target, const PulsarParams& params)
	{
		sf::Shader* shader = shaders.Find("pulsar");
		if (shader == nullptr)
		{
			return;
		}
		const float period = std::max(params.periodSeconds, 0.05f);
		// Only the fraction of a period matters: the clock is free-running, so wrap it into [0, 1).
		float cycle = params.timeSeconds / period;
		cycle -= std::floor(cycle);
		const float extent = std::max(params.beamLength, params.nebulaRadius) * 1.1f + 2.f;
		{
			UniformWriter uniforms(*shader);
			uniforms.Set("uCenter", params.center);
			uniforms.Set("uPhase", cycle * kTwoPi);
			uniforms.Set("uTilt", params.magneticTiltRad);
			uniforms.Set("uBeamLength", params.beamLength);
			uniforms.Set("uBeamHalfAngle", params.beamHalfAngleRad);
			uniforms.Set("uNebulaRadius", params.nebulaRadius);
			uniforms.Set("uMaxBrightness", params.maxBrightness);
		}
		const sf::Vector2f extentVec(extent, extent);
		DrawShaderQuad(target, *shader, sf::FloatRect(params.center - extentVec, extentVec * 2.f), sf::BlendAdd);
	}

	ShatteredMoon::ShatteredMoon(unsigned seed, int chunkCount)
	{
		std::mt19937 rng(seed);
		std::uniform_real_distribution<float> unit(0.f, 1.f);

		const int count = std::clamp(chunkCount, 1, static_cast<int>(kMaxChunks));
		// Breakup happened at one point of the orbit, so every piece starts near the same angle.
		const float arcCenter = unit(rng) * kTwoPi;
		mChunks.reserve(static_cast<std::size_t>(count));
		for (int i = 0; i < count; ++i)
		{
			Chunk chunk;
			chunk.radialFraction = (unit(rng) - 0.5f) * 0.24f;
			chunk.angle0 = arcCenter + (unit(rng) - 0.5f) * kArcWidth;
			// Small pieces are more common than large ones.
			const float u = unit(rng);
			chunk.size = 1.5f + 3.5f * u * u;
			chunk.spin = (unit(rng) - 0.5f) * 0.4f;
			chunk.shade = 0.75f + 0.25f * unit(rng);
			chunk.vertexCount = 5 + std::min(3, static_cast<int>(unit(rng) * 4.f));
			for (int j = 0; j < chunk.vertexCount; ++j)
			{
				const float step = kTwoPi / static_cast<float>(chunk.vertexCount);
				const float angle = (static_cast<float>(j) + (unit(rng) - 0.5f) * 0.5f) * step;
				const float radius = 0.7f + 0.6f * unit(rng);
				chunk.outline[static_cast<std::size_t>(j)] = {std::cos(angle) * radius, std::sin(angle) * radius};
			}
			mChunks.push_back(chunk);
		}
	}

	void ShatteredMoon::Draw(sf::RenderTarget& target, const Params& params) const
	{
		sf::Vector2f light = params.lightDirection;
		const float lightLength = std::sqrt(light.x * light.x + light.y * light.y);
		light = lightLength > 0.f ? light / lightLength : sf::Vector2f(0.f, -1.f);

		const sf::Color lit = LimitBrightness(params.rockColor, params.maxBrightness);
		const sf::Color dark = Scale(lit, kShadeDark);

		mVertices.clear();
		mVertices.reserve(mChunks.size() * 8 * 6);
		for (const Chunk& chunk : mChunks)
		{
			const float r = params.orbitRadius * (1.f + chunk.radialFraction);
			// Keplerian angular speed: inner pieces run ahead, which shears the arc into a ring.
			const float omega = kBaseOmega * std::pow(r / params.orbitRadius, -1.5f);
			const float angle = chunk.angle0 + omega * params.ageSeconds;
			// The ring is seen at an angle: the y axis is squashed by tilt. Occlusion by the planet disc is not modelled
			// (no planet radius is given), so pieces behind the planet are drawn too.
			const sf::Vector2f center = params.planetCenter
				+ sf::Vector2f(std::cos(angle) * r, std::sin(angle) * r * params.tilt);

			const float rot = chunk.spin * params.ageSeconds;
			const float cs = std::cos(rot);
			const float sn = std::sin(rot);
			const sf::Color litChunk = Scale(lit, chunk.shade);
			const sf::Color darkChunk = Scale(dark, chunk.shade);

			std::array<sf::Vector2f, 8> outline{};
			for (int j = 0; j < chunk.vertexCount; ++j)
			{
				const sf::Vector2f o = chunk.outline[static_cast<std::size_t>(j)];
				const sf::Vector2f turned(o.x * cs - o.y * sn, o.x * sn + o.y * cs);
				outline[static_cast<std::size_t>(j)] = center + turned * chunk.size;
			}
			for (int j = 0; j < chunk.vertexCount; ++j)
			{
				const sf::Vector2f a = outline[static_cast<std::size_t>(j)];
				const sf::Vector2f b = outline[static_cast<std::size_t>((j + 1) % chunk.vertexCount)];
				AddSplitTriangle(mVertices, center, a, b, light, litChunk, darkChunk);
			}
		}
		if (!mVertices.empty())
		{
			target.draw(mVertices.data(), mVertices.size(), sf::PrimitiveType::Triangles, sf::RenderStates(sf::BlendAlpha));
		}

		// Faint dust along the orbit: single pixels at low alpha, a fixed set of points around the ellipse.
		mDust.clear();
		const sf::Color dust = Scale(lit, 0.6f);
		for (int i = 0; i < kDustPoints; ++i)
		{
			const float theta = static_cast<float>(i) * kTwoPi / static_cast<float>(kDustPoints);
			const float r = params.orbitRadius * (1.f + 0.12f * (2.f * HashUnit(i) - 1.f));
			const sf::Vector2f p = params.planetCenter
				+ sf::Vector2f(std::cos(theta) * r, std::sin(theta) * r * params.tilt);
			const auto alpha = static_cast<std::uint8_t>(30.f + 50.f * HashUnit(i + 977));
			mDust.push_back({p, sf::Color(dust.r, dust.g, dust.b, alpha)});
		}
		target.draw(mDust.data(), mDust.size(), sf::PrimitiveType::Points, sf::RenderStates(sf::BlendAlpha));
	}
}
