#pragma once

#include <SFML/Graphics.hpp>

namespace ly
{
	enum class ArenaBoundaryVisualType
	{
		DebugRectangle,
		EnergyWall,
		WarningField,
		None
	};

	struct ArenaBoundaryVisualDefinition
	{
		bool enabled = true;
		ArenaBoundaryVisualType visualType = ArenaBoundaryVisualType::DebugRectangle;
		sf::Color outlineColor{ 90, 230, 255, 180 };
		sf::Color warningColor{ 255, 80, 80, 220 };
		float outlineThickness = 4.f;
	};

	struct ArenaDefinition
	{
		sf::Vector2f size {3000.0f, 2000.0f};
		sf::FloatRect legalBounds{ sf::Vector2f{0.0f, 0.0f}, size };
		float outOfBoundsMargin = 20.0f;
		float outOfBoundsTime = 5.f;

		ArenaBoundaryVisualDefinition boundaryVisual;
	};
}
