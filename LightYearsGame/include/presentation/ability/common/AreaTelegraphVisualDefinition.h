#pragma once

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct AreaTelegraphVisualDefinition
	{
		sf::Color fillColor = sf::Color::Transparent;
		sf::Color outlineColor = sf::Color::White;
		float outlineThickness = 2.f;
		float pulseSpeed = 0.f;
		float minimumPulse = 1.f;
		float maximumPulse = 1.f;
		float pulseScaleAmount = 0.f;
		sf::Color dangerFillColor{ 255, 45, 20, 135 };
		sf::Color dangerOutlineColor{ 255, 220, 110, 255 };
		float countdownStartScale = 1.45f;
		float countdownEndScale = 1.f;
		float countdownEaseExponent = 2.6f;
		float countdownRingThickness = 3.f;
	};
}
