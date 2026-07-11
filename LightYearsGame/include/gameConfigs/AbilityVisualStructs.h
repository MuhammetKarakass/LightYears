#pragma once

#include <string>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

namespace ly
{
	struct ShieldVisualDefinition
	{
		std::string visualId;
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

	struct SunBeamVisualDefinition
	{
		std::string visualId;
		sf::Color outerColor{ 255, 165, 35, 150 };
		sf::Color coreColor{ 255, 250, 220, 250 };
		sf::Color groundGlowColor{ 255, 205, 90, 105 };
		sf::Color impactRingColor{ 255, 245, 205, 230 };
		float visibleLengthScale = 0.45f;
		float arrivalStartWidthScale = 0.04f;
		float arrivalEndWidthScale = 0.18f;
		float impactWidthScale = 0.26f;
		float groundGlowStartScale = 1.35f;
		float groundGlowEndScale = 0.75f;
		float impactRingEndScale = 1.9f;
		float impactRingThickness = 5.f;
		float pulseSpeed = 22.f;
		float overheadStartScale = 0.12f;
		float overheadEndScale = 1.f;
		float convergenceHaloStartScale = 1.8f;
		float convergenceHaloEndScale = 0.72f;
		float impactFlashScale = 1.65f;
		float impactGlareLengthScale = 2.4f;
		sf::Vector2f perspectiveSourceOffset{ -55.f, -520.f };
		float impactColumnWidthScale = 1.8f;
		float perspectiveCoreWidthScale = 0.32f;
		float perspectiveShaftIntensity = 0.78f;
	};

	// Presentation-only fixed-world warning for a circular delayed impact.
	struct AreaTelegraphVisualDefinition
	{
		std::string visualId;
		sf::Color fillColor = sf::Color::Transparent;
		sf::Color outlineColor = sf::Color::White;
		float outlineThickness = 2.f;
		float pulseSpeed = 0.f;
		float minimumPulse = 1.f;
		float maximumPulse = 1.f;
		float pulseScaleAmount = 0.f;
	};
}


