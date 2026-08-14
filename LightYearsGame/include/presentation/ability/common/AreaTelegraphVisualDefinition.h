#pragma once

#include <SFML/Graphics/Color.hpp>

#include <cstdint>

namespace ly
{
	enum class AreaTelegraphFillMode : std::uint8_t
	{
		Static,
		RadialProgress
	};

	struct AreaTelegraphVisualDefinition
	{
		// A radial fill expands from the actor's center as countdown progress grows.
		bool drawInteriorFill = true;
		AreaTelegraphFillMode fillMode = AreaTelegraphFillMode::Static;
		// Optional non-zero starting radius for radial fills. A value of 0 keeps
		// the original center-out behavior; 0.5 starts the fill at half radius.
		float minimumFillRadiusRatio = 0.f;
		bool drawCountdownRing = true;
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
		// Controls the optional mild logarithmic-like radial growth curve.
		// Zero keeps radial fills on the normal eased-progress curve.
		float radialGrowthLogStrength = 0.f;
		// Normalized countdown point where the primary radial-fill phase ends.
		// With the default values, the whole radial fill completes normally.
		float radialGrowthPrimaryPhaseEnd = 1.f;
		// Radial-fill progress reached at radialGrowthPrimaryPhaseEnd.
		float radialGrowthPrimaryPhaseFill = 1.f;
		// A positive duration enables a short completion feedback after filling.
		float completionFeedbackDuration = 0.f;
		sf::Color completionFillColor{ 210, 250, 255, 190 };
		sf::Color completionOutlineColor{ 245, 255, 255, 255 };
		float completionStartScale = 1.f;
		float completionEndScale = 1.16f;
		float countdownRingThickness = 3.f;
	};
}
