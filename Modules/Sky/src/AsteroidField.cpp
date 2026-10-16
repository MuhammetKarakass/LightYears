#include "sky/AsteroidField.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "sky/UniformWriter.h"

namespace vega::sky
{
	namespace
	{
		constexpr float kDegToRad = 3.14159265f / 180.f;
		constexpr float kTwoPi = 6.2831853f;

		// Deterministic per-stop numbers (splitmix64), the same on every platform.
		class Random
		{
		public:
			explicit Random(std::uint64_t seed) : mState(seed) {}

			float Next01()
			{
				mState += 0x9e3779b97f4a7c15ULL;
				std::uint64_t z = mState;
				z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
				z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
				z ^= z >> 31;
				return static_cast<float>(z >> 40) / 16777216.f;
			}

		private:
			std::uint64_t mState;
		};

		bool Same(const sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b)
		{
			return a.x == b.x && a.y == b.y && a.z == b.z;
		}

		std::uint8_t Byte(float value01)
		{
			return static_cast<std::uint8_t>(std::clamp(value01, 0.f, 1.f) * 255.f + 0.5f);
		}

		float Wrap(float value, float period)
		{
			const float r = std::fmod(value, period);
			return r < 0.f ? r + period : r;
		}
	}

	bool AsteroidBeltSettings::operator==(const AsteroidBeltSettings& o) const
	{
		return count == o.count && angleDeg == o.angleDeg && center == o.center && width == o.width && speed == o.speed
			&& nearest == o.nearest && Same(dark, o.dark) && Same(mid, o.mid) && Same(light, o.light) && Same(haze, o.haze);
	}

	void AsteroidField::Build(unsigned seed, const AsteroidBeltSettings& settings)
	{
		mSettings = settings;
		mRocks.clear();
		Random random(0xa5a5f00dULL ^ seed);
		const int count = std::clamp(settings.count, 0, 400);
		mRocks.reserve(static_cast<std::size_t>(count));
		for (int i = 0; i < count; ++i)
		{
			Rock rock;
			rock.along = random.Next01();
			// Most rocks are far and small; a few are near and big (depth skewed toward 0).
			rock.depth = std::pow(random.Next01(), 1.4f) * std::clamp(settings.nearest, 0.05f, 1.f);
			// Near rocks spread a little wider across the belt (it surrounds the camera).
			const float u = random.Next01() * 2.f - 1.f;
			rock.across = u * std::abs(u) * (0.8f + 0.6f * rock.depth);
			const float sizeRoll = random.Next01();
			rock.size = (0.006f + 0.1f * rock.depth * rock.depth) * (0.45f + 1.1f * sizeRoll * sizeRoll);
			rock.spin = (random.Next01() * 2.f - 1.f) * (0.05f + 0.25f * (1.f - sizeRoll));
			rock.phase = random.Next01() * kTwoPi;
			rock.seed = random.Next01();
			mRocks.push_back(rock);
		}
		std::sort(mRocks.begin(), mRocks.end(), [](const Rock& a, const Rock& b) { return a.depth < b.depth; });
	}

	void AsteroidField::Draw(ShaderLibrary& shaders, sf::RenderTarget& target, const Params& params) const
	{
		if (mRocks.empty())
		{
			return;
		}
		sf::Shader* shader = shaders.Find("asteroid");
		if (shader == nullptr)
		{
			return;
		}
		const sf::Vector2f size(target.getSize());
		const float height = size.y;
		const float angle = (params.mirrored ? -mSettings.angleDeg : mSettings.angleDeg) * kDegToRad;
		const sf::Vector2f along(std::cos(angle), std::sin(angle));
		const sf::Vector2f across(-along.y, along.x);
		const float length = std::sqrt(size.x * size.x + size.y * size.y) * 1.3f;
		const sf::Vector2f centre(size.x * 0.5f, height * mSettings.center);
		const float lightAngle = std::atan2(params.lightDirection.y, params.lightDirection.x);

		mVertices.clear();
		mVertices.reserve(mRocks.size() * 6);
		for (const Rock& rock : mRocks)
		{
			// Parallax grows with depth (far rocks barely move, near ones sweep past); orbital drift too.
			const float parallax = 0.04f + 0.9f * rock.depth;
			const float drift = params.timeSeconds * mSettings.speed * (4.f + 26.f * rock.depth);
			const float s = Wrap(rock.along * length + drift - (params.scroll.x * along.x + params.scroll.y * along.y) * parallax, length) - 0.5f * length;
			const float t = rock.across * mSettings.width * height - (params.scroll.x * across.x + params.scroll.y * across.y) * parallax;
			const sf::Vector2f at = centre + along * s + across * t;
			const float radius = rock.size * height;
			if (at.x < -radius || at.y < -radius || at.x > size.x + radius || at.y > size.y + radius)
			{
				continue;
			}
			const float rotation = rock.phase + rock.spin * params.timeSeconds;
			const float c = std::cos(rotation);
			const float sn = std::sin(rotation);
			// The rock's texture turns with the quad, so the light direction is given in the rock's own frame.
			const float localLight = Wrap(lightAngle - rotation, kTwoPi) / kTwoPi;
			// g carries the radius in pixels (/510) so the shader can draw a one-pixel outline at any size.
			const sf::Color packed(Byte(rock.seed), Byte(radius / 510.f), Byte(localLight), Byte(rock.depth));
			const sf::Vector2f corners[4] = {{-1.f, -1.f}, {1.f, -1.f}, {1.f, 1.f}, {-1.f, 1.f}};
			sf::Vertex quad[4];
			for (int k = 0; k < 4; ++k)
			{
				const sf::Vector2f local = corners[k] * radius;
				quad[k].position = at + sf::Vector2f(local.x * c - local.y * sn, local.x * sn + local.y * c);
				quad[k].color = packed;
				quad[k].texCoords = corners[k];
			}
			const int order[6] = {0, 1, 2, 0, 2, 3};
			for (int k : order)
			{
				mVertices.push_back(quad[k]);
			}
		}
		if (mVertices.empty())
		{
			return;
		}
		{
			UniformWriter uniforms(*shader);
			// The 5-stop tone ramp of the planets' style, from the belt's dark, mid and light colours.
			const auto mix = [](const sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b) { return sf::Glsl::Vec3((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f, (a.z + b.z) * 0.5f); };
			const sf::Glsl::Vec3 ramp[5] = {mSettings.dark, mix(mSettings.dark, mSettings.mid), mSettings.mid, mix(mSettings.mid, mSettings.light), mSettings.light};
			uniforms.SetArray("uRamp", ramp, 5);
			uniforms.Set("uHaze", mSettings.haze);
			uniforms.Set("uMaxBrightness", params.maxBrightness);
		}
		sf::RenderStates states;
		states.shader = shader;
		states.blendMode = sf::BlendAlpha;
		target.draw(mVertices.data(), mVertices.size(), sf::PrimitiveType::Triangles, states);
	}
}
