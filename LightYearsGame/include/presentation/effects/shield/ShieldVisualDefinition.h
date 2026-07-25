#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>

namespace ly
{
	struct ShieldVisualDefinition
	{
		std::string texturePath;
		sf::Color color = sf::Color::White;
		sf::Vector2f localOffset{ 0.f, 0.f };
		float baseScale = 1.f;
		float rotationSpeed = 0.f;
		float pulseSpeed = 1.f;
		float pulseScaleAmount = 0.f;
		float minimumAlpha = 255.f;
		float maximumAlpha = 255.f;
		float lowIntegrityPulseMultiplier = 1.75f;
		float lowIntegrityAlphaMultiplier = 0.65f;
	};
}
