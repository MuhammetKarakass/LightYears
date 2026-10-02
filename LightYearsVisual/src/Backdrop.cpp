#include "visual/Backdrop.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace ly::visual
{
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
		shader->setUniform("uScroll", sf::Glsl::Vec2(params.scroll));
		shader->setUniform("uScale", params.scale);
		shader->setUniform("uIntensity", params.intensity);
		shader->setUniform("uSeed", params.seed);
		shader->setUniform("uMaxBrightness", params.maxBrightness);
		SetRampUniform(*shader, "uRamp", ramp, 4);
		DrawShaderQuad(target, *shader, sf::FloatRect({0.f, 0.f}, sf::Vector2f(target.getSize())));
	}

	StarField::StarField(unsigned seed, int count)
	{
		std::mt19937 rng(seed);
		std::uniform_real_distribution<float> unit(0.f, 1.f);
		for (int i = 0; i < count; ++i)
		{
			Star star;
			star.unit = {unit(rng), unit(rng)};
			// most stars are far and dim; few are near
			float depth = unit(rng);
			star.depth = depth * depth;
			star.tint = unit(rng);
			star.phase = unit(rng) * 6.2831853f;
			mStars.push_back(star);
		}
	}

	void StarField::Draw(sf::RenderTarget& target, sf::Vector2f scroll, float timeSeconds, const ColorRamp& tint, float maxBrightness) const
	{
		sf::Vector2f size(target.getSize());
		std::vector<sf::Vertex> vertices;
		vertices.reserve(mStars.size() * 6);
		for (const Star& star : mStars)
		{
			float parallax = 0.08f + 0.5f * star.depth;
			float x = std::fmod(star.unit.x * size.x - scroll.x * parallax, size.x);
			float y = std::fmod(star.unit.y * size.y - scroll.y * parallax, size.y);
			x = std::floor(x < 0.f ? x + size.x : x);
			y = std::floor(y < 0.f ? y + size.y : y);
			float px = star.depth > 0.8f ? 2.f : 1.f;
			float twinkle = 0.88f + 0.12f * std::sin(timeSeconds * 1.7f + star.phase);
			sf::Color color = tint.Sample(0.65f + 0.35f * star.tint);
			color = LimitBrightness(color, maxBrightness * (0.45f + 0.55f * star.depth) * twinkle);
			sf::Vector2f p(x, y);
			for (sf::Vector2f corner : {p, p + sf::Vector2f(px, 0.f), p + sf::Vector2f(0.f, px), p + sf::Vector2f(px, 0.f), p + sf::Vector2f(px, px), p + sf::Vector2f(0.f, px)})
			{
				vertices.push_back({corner, color});
			}
		}
		target.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Triangles);
	}
}
