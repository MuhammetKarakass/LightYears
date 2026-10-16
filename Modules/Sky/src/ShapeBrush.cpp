#include "sky/ShapeBrush.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "sky/UniformWriter.h"

namespace vega::sky
{
	namespace
	{
		constexpr int CircleShape = 0;
		constexpr int RingShape = 1;
		constexpr int CapsuleShape = 2;
		constexpr int ArcShape = 3;
		constexpr float Pi = 3.14159265358979f;
		constexpr float SoftEdgePixels = 1.f;

		float Cross(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c)
		{
			return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
		}

		float SignedArea(const std::vector<sf::Vector2f>& p)
		{
			float area = 0.f;
			for (size_t i = 0; i < p.size(); ++i)
			{
				const sf::Vector2f& a = p[i];
				const sf::Vector2f& b = p[(i + 1) % p.size()];
				area += a.x * b.y - b.x * a.y;
			}
			return area * 0.5f;
		}

		bool InsideTriangle(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c)
		{
			return Cross(a, b, p) >= 0.f && Cross(b, c, p) >= 0.f && Cross(c, a, p) >= 0.f;
		}

		// Ear clipping for a simple polygon. Returns index triples.
		std::vector<std::array<size_t, 3>> Triangulate(const std::vector<sf::Vector2f>& points)
		{
			std::vector<std::array<size_t, 3>> triangles;
			std::vector<size_t> ring(points.size());
			for (size_t i = 0; i < ring.size(); ++i)
			{
				ring[i] = i;
			}
			if (SignedArea(points) < 0.f)
			{
				std::reverse(ring.begin(), ring.end());
			}
			size_t guard = ring.size() * ring.size();
			while (ring.size() > 3 && guard-- > 0)
			{
				bool clipped = false;
				for (size_t i = 0; i < ring.size(); ++i)
				{
					size_t prev = ring[(i + ring.size() - 1) % ring.size()];
					size_t cur = ring[i];
					size_t next = ring[(i + 1) % ring.size()];
					if (Cross(points[prev], points[cur], points[next]) <= 0.f)
					{
						continue;
					}
					bool containsOther = false;
					for (size_t other : ring)
					{
						if (other != prev && other != cur && other != next && InsideTriangle(points[other], points[prev], points[cur], points[next]))
						{
							containsOther = true;
							break;
						}
					}
					if (containsOther)
					{
						continue;
					}
					triangles.push_back({prev, cur, next});
					ring.erase(ring.begin() + static_cast<std::ptrdiff_t>(i));
					clipped = true;
					break;
				}
				if (!clipped)
				{
					break;
				}
			}
			if (ring.size() >= 3)
			{
				// Remaining triangle, or a fan fallback when the polygon was not simple.
				for (size_t i = 1; i + 1 < ring.size(); ++i)
				{
					triangles.push_back({ring[0], ring[i], ring[i + 1]});
				}
			}
			return triangles;
		}

		sf::FloatRect Bounds(sf::Vector2f min, sf::Vector2f max, float margin)
		{
			return sf::FloatRect({min.x - margin, min.y - margin}, {max.x - min.x + margin * 2.f, max.y - min.y + margin * 2.f});
		}
	}

	ShapeBrush::ShapeBrush(ShaderLibrary& shaders)
		: mShaders(shaders)
	{
	}

	void ShapeBrush::Circle(sf::RenderTarget& target, sf::Vector2f center, float radius, const ShapeStyle& style) const
	{
		ShapeParams params;
		params.shapeId = CircleShape;
		params.a = center;
		params.radius = radius;
		DrawShape(target, Bounds(center, center, radius), params, style);
	}

	void ShapeBrush::Ring(sf::RenderTarget& target, sf::Vector2f center, float radius, float thickness, const ShapeStyle& style) const
	{
		ShapeParams params;
		params.shapeId = RingShape;
		params.a = center;
		params.radius = radius;
		params.thickness = thickness;
		DrawShape(target, Bounds(center, center, radius + thickness * 0.5f), params, style);
	}

	void ShapeBrush::Capsule(sf::RenderTarget& target, sf::Vector2f a, sf::Vector2f b, float radius, const ShapeStyle& style) const
	{
		ShapeParams params;
		params.shapeId = CapsuleShape;
		params.a = a;
		params.b = b;
		params.radius = radius;
		sf::Vector2f lo(std::min(a.x, b.x), std::min(a.y, b.y));
		sf::Vector2f hi(std::max(a.x, b.x), std::max(a.y, b.y));
		DrawShape(target, Bounds(lo, hi, radius), params, style);
	}

	void ShapeBrush::Arc(sf::RenderTarget& target, sf::Vector2f center, float radius, float thickness, float startDegrees, float sweepDegrees, const ShapeStyle& style) const
	{
		float mid = (startDegrees + sweepDegrees * 0.5f) * Pi / 180.f;
		float half = std::min(std::abs(sweepDegrees) * 0.5f, 180.f) * Pi / 180.f;
		ShapeParams params;
		params.shapeId = ArcShape;
		params.a = center;
		params.b = {std::cos(mid), std::sin(mid)};
		params.radius = radius;
		params.thickness = thickness;
		params.arc = {std::sin(half), std::cos(half)};
		DrawShape(target, Bounds(center, center, radius + thickness * 0.5f), params, style);
	}

	void ShapeBrush::DrawShape(sf::RenderTarget& target, const sf::FloatRect& bounds, const ShapeParams& params, const ShapeStyle& style) const
	{
		sf::Shader* shader = mShaders.Find("sdf_shape");
		if (shader == nullptr)
		{
			return;
		}
		float soft = style.hardEdges ? 0.f : SoftEdgePixels;
		float margin = style.outlineWidth + soft + 1.f;
		sf::FloatRect padded({bounds.position.x - margin, bounds.position.y - margin}, {bounds.size.x + margin * 2.f, bounds.size.y + margin * 2.f});
		{
			UniformWriter uniforms(*shader);
			uniforms.Set("uShape", params.shapeId);
			uniforms.Set("uA", params.a);
			uniforms.Set("uB", params.b);
			uniforms.Set("uRadius", params.radius);
			uniforms.Set("uThickness", params.thickness);
			uniforms.Set("uArc", params.arc);
			uniforms.Set("uFill", sf::Glsl::Vec4(style.fill));
			uniforms.Set("uOutlineColor", sf::Glsl::Vec4(style.outline));
			uniforms.Set("uOutline", style.outlineWidth);
			uniforms.Set("uSoft", soft);
		}
		const sf::BlendMode additive(sf::BlendMode::Factor::SrcAlpha, sf::BlendMode::Factor::One);
		DrawShaderQuad(target, *shader, padded, style.additive ? additive : sf::BlendAlpha);
	}

	void ShapeBrush::Polygon(sf::RenderTarget& target, const std::vector<sf::Vector2f>& points, sf::Color fill) const
	{
		if (points.size() < 3)
		{
			return;
		}
		std::vector<sf::Vertex> vertices;
		for (const auto& tri : Triangulate(points))
		{
			for (size_t index : tri)
			{
				vertices.push_back({points[index], fill});
			}
		}
		if (!vertices.empty())
		{
			target.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Triangles);
		}
	}

	void ShapeBrush::PolygonOutline(sf::RenderTarget& target, const std::vector<sf::Vector2f>& points, sf::Color color, float width) const
	{
		if (points.size() < 2 || width <= 0.f)
		{
			return;
		}
		constexpr int JoinSegments = 8;
		float half = width * 0.5f;
		std::vector<sf::Vertex> vertices;
		for (size_t i = 0; i < points.size(); ++i)
		{
			sf::Vector2f a = points[i];
			sf::Vector2f b = points[(i + 1) % points.size()];
			sf::Vector2f dir = b - a;
			float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
			if (length > 0.f)
			{
				sf::Vector2f n(-dir.y / length * half, dir.x / length * half);
				for (sf::Vector2f p : {a + n, a - n, b - n, a + n, b - n, b + n})
				{
					vertices.push_back({p, color});
				}
			}
			// round join at every corner so concave and sharp angles stay closed
			for (int s = 0; s < JoinSegments; ++s)
			{
				float a0 = 2.f * Pi * static_cast<float>(s) / JoinSegments;
				float a1 = 2.f * Pi * static_cast<float>(s + 1) / JoinSegments;
				vertices.push_back({a, color});
				vertices.push_back({a + sf::Vector2f(std::cos(a0), std::sin(a0)) * half, color});
				vertices.push_back({a + sf::Vector2f(std::cos(a1), std::sin(a1)) * half, color});
			}
		}
		target.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Triangles);
	}
}
