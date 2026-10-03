#pragma once

#include <vector>

#include <SFML/Graphics.hpp>

#include "visual/ShaderLibrary.h"

namespace ly::visual
{
	struct ShapeStyle
	{
		sf::Color fill = sf::Color::White;
		sf::Color outline = sf::Color::Transparent;
		float outlineWidth = 0.f;
		// true gives pixel-snapped edges (no anti-aliasing): use it in the world layer.
		bool hardEdges = false;
		// Adds the fill (scaled by its alpha) to the target instead of alpha blending: energy-layer shapes.
		bool additive = false;
	};

	// SDF shapes and polygons drawn into any layer target. Positions are output-pixel coordinates.
	class ShapeBrush
	{
	public:
		explicit ShapeBrush(ShaderLibrary& shaders);

		void Circle(sf::RenderTarget& target, sf::Vector2f center, float radius, const ShapeStyle& style) const;
		void Ring(sf::RenderTarget& target, sf::Vector2f center, float radius, float thickness, const ShapeStyle& style) const;
		// A line is a capsule with a small radius.
		void Capsule(sf::RenderTarget& target, sf::Vector2f a, sf::Vector2f b, float radius, const ShapeStyle& style) const;
		// Angles in degrees, clockwise on screen from +x. Sweep may be negative.
		void Arc(sf::RenderTarget& target, sf::Vector2f center, float radius, float thickness, float startDegrees, float sweepDegrees, const ShapeStyle& style) const;

		// Simple polygon, convex or concave, either winding. Always hard-edged.
		void Polygon(sf::RenderTarget& target, const std::vector<sf::Vector2f>& points, sf::Color fill) const;
		// Closed outline centered on the edges; draw it before the fill so only the outer half shows.
		void PolygonOutline(sf::RenderTarget& target, const std::vector<sf::Vector2f>& points, sf::Color color, float width) const;

	private:
		struct ShapeParams
		{
			int shapeId = 0;
			sf::Vector2f a;
			sf::Vector2f b;
			float radius = 0.f;
			float thickness = 0.f;
			sf::Vector2f arc;
		};

		void DrawShape(sf::RenderTarget& target, const sf::FloatRect& bounds, const ShapeParams& params, const ShapeStyle& style) const;

		ShaderLibrary& mShaders;
	};
}
