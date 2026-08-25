#pragma once

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	// A reusable line-shaped charge telegraph. The geometry is intentionally
	// separate from AreaTelegraphVisualDefinition because a path preview has no
	// circular fill, radius outline or area countdown semantics.
	struct DirectionalChargeTelegraphVisualDefinition
	{
		sf::Color glowColor{ 40, 210, 255, 105 };
		sf::Color outerColor{ 80, 220, 255, 220 };
		sf::Color coreColor{ 245, 255, 255, 255 };
		sf::Color endpointColor{ 170, 250, 255, 235 };
		sf::Color endpointRingColor{ 235, 255, 255, 255 };
		float glowWidth = 34.f;
		float outerWidth = 18.f;
		float coreWidth = 5.f;
		float endpointRadius = 12.f;
		float endpointRingThickness = 2.5f;
		float pulseSpeed = 12.f;
		float minimumAlpha = 0.60f;
		float maximumAlpha = 1.f;
		float completionFeedbackDuration = 0.18f;
		float completionStartScale = 1.f;
		float completionEndScale = 1.14f;
	};
}
