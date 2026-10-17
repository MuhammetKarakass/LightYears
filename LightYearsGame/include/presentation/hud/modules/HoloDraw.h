#pragma once

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

namespace vega::holo
{
	using Vec = sf::Vector2f;
	using Pts = std::vector<Vec>;

	inline float Clamp01(float v) { return std::clamp(v, 0.f, 1.f); }

	inline sf::Color Lerp(const sf::Color& a, const sf::Color& b, float t)
	{
		t = Clamp01(t);
		const auto mix = [t](std::uint8_t x, std::uint8_t y) { return static_cast<std::uint8_t>(static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t); };
		return { mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a) };
	}

	// Same color with an absolute alpha in 0..255.
	inline sf::Color Alpha(const sf::Color& c, float alpha) { return { c.r, c.g, c.b, static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 255.f)) }; }

	inline sf::Color Mul(const sf::Color& c, float f)
	{
		const auto ch = [f](std::uint8_t v) { return static_cast<std::uint8_t>(std::clamp(static_cast<float>(v) * f, 0.f, 255.f)); };
		return { ch(c.r), ch(c.g), ch(c.b), c.a };
	}

	inline void Tri(sf::VertexArray& va, const Vec& a, const Vec& b, const Vec& c, const sf::Color& ca, const sf::Color& cb, const sf::Color& cc)
	{
		va.append(sf::Vertex{ a, ca });
		va.append(sf::Vertex{ b, cb });
		va.append(sf::Vertex{ c, cc });
	}

	// Quad with corners given in order around the outline.
	inline void Quad(sf::VertexArray& va, const Vec& a, const Vec& b, const Vec& c, const Vec& d, const sf::Color& ca, const sf::Color& cb, const sf::Color& cc, const sf::Color& cd)
	{
		Tri(va, a, b, c, ca, cb, cc);
		Tri(va, a, c, d, ca, cc, cd);
	}

	inline void Rect(sf::VertexArray& va, const Vec& pos, const Vec& size, const sf::Color& top, const sf::Color& bottom)
	{
		Quad(va, pos, { pos.x + size.x, pos.y }, pos + size, { pos.x, pos.y + size.y }, top, top, bottom, bottom);
	}

	inline void RectH(sf::VertexArray& va, const Vec& pos, const Vec& size, const sf::Color& left, const sf::Color& right)
	{
		Quad(va, pos, { pos.x + size.x, pos.y }, pos + size, { pos.x, pos.y + size.y }, left, right, right, left);
	}

	inline void LineGrad(sf::VertexArray& va, const Vec& p0, const Vec& p1, float thickness, const sf::Color& c0, const sf::Color& c1)
	{
		const Vec d = p1 - p0;
		const float len = std::sqrt(d.x * d.x + d.y * d.y);
		if (len < 0.001f) return;
		const Vec n{ -d.y / len * thickness * 0.5f, d.x / len * thickness * 0.5f };
		Quad(va, p0 - n, p1 - n, p1 + n, p0 + n, c0, c1, c1, c0);
	}

	inline void Line(sf::VertexArray& va, const Vec& p0, const Vec& p1, float thickness, const sf::Color& color) { LineGrad(va, p0, p1, thickness, color, color); }

	inline void Diamond(sf::VertexArray& va, const Vec& center, float radius, const sf::Color& color)
	{
		Quad(va, { center.x, center.y - radius }, { center.x + radius, center.y }, { center.x, center.y + radius }, { center.x - radius, center.y }, color, color, color, color);
	}

	// Fan around the vertex average; correct for convex polygons.
	inline void FillConvex(sf::VertexArray& va, const Pts& pts, const sf::Color& color)
	{
		if (pts.size() < 3) return;
		Vec center{};
		for (const Vec& p : pts) center += p;
		center /= static_cast<float>(pts.size());
		for (std::size_t i = 0; i < pts.size(); ++i) Tri(va, center, pts[i], pts[(i + 1) % pts.size()], color, color, color);
	}

	inline Pts ChamferRect(const Vec& pos, const Vec& size, float topLeft, float topRight, float bottomRight, float bottomLeft)
	{
		Pts pts;
		const float r = pos.x + size.x;
		const float b = pos.y + size.y;
		const auto corner = [&pts](const Vec& a, const Vec& c, float cut) { if (cut > 0.f) { pts.push_back(a); pts.push_back(c); } else pts.push_back(a); };
		corner({ pos.x + topLeft, pos.y }, { pos.x, pos.y + topLeft }, topLeft);
		corner({ pos.x, b - bottomLeft }, { pos.x + bottomLeft, b }, bottomLeft);
		corner({ r - bottomRight, b }, { r, b - bottomRight }, bottomRight);
		corner({ r, pos.y + topRight }, { r - topRight, pos.y }, topRight);
		return pts;
	}

	inline float SignedArea(const Pts& pts)
	{
		float area = 0.f;
		for (std::size_t i = 0; i < pts.size(); ++i)
		{
			const Vec& a = pts[i];
			const Vec& b = pts[(i + 1) % pts.size()];
			area += a.x * b.y - b.x * a.y;
		}
		return area * 0.5f;
	}

	// Closed ribbon along a polygon: offsets are along the mitered outward normal (negative is inward).
	inline void ClosedStrip(sf::VertexArray& va, const Pts& pts, float offsetA, float offsetB, const sf::Color& colorA, const sf::Color& colorB)
	{
		const std::size_t n = pts.size();
		if (n < 3) return;
		const float sign = SignedArea(pts) > 0.f ? 1.f : -1.f;
		std::vector<Vec> edgeNormal(n), vertexNormal(n);
		for (std::size_t i = 0; i < n; ++i)
		{
			const Vec d = pts[(i + 1) % n] - pts[i];
			const float len = std::max(0.0001f, std::sqrt(d.x * d.x + d.y * d.y));
			edgeNormal[i] = Vec{ d.y / len * sign, -d.x / len * sign };
		}
		for (std::size_t i = 0; i < n; ++i)
		{
			const Vec& prev = edgeNormal[(i + n - 1) % n];
			const Vec& next = edgeNormal[i];
			Vec sum = prev + next;
			const float len = std::sqrt(sum.x * sum.x + sum.y * sum.y);
			if (len < 0.0001f) { vertexNormal[i] = next; continue; }
			sum /= len;
			const float cosHalf = std::max(0.45f, sum.x * next.x + sum.y * next.y);
			vertexNormal[i] = sum / cosHalf;
		}
		for (std::size_t i = 0; i < n; ++i)
		{
			const std::size_t j = (i + 1) % n;
			Quad(va, pts[i] + vertexNormal[i] * offsetA, pts[j] + vertexNormal[j] * offsetA, pts[j] + vertexNormal[j] * offsetB, pts[i] + vertexNormal[i] * offsetB, colorA, colorA, colorB, colorB);
		}
	}
}
