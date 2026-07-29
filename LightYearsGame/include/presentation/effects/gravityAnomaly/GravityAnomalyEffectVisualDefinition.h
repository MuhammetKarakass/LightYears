#pragma once

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct GravityAnomalyEffectVisualDefinition
	{
		sf::Color ringColor{ 145, 100, 255, 170 };
		sf::Color coreColor{ 90, 165, 255, 130 };
		float radius = 24.f;
		float ringThickness = 2.f;
		float rotationSpeed = 135.f;
	};
}
