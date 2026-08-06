#pragma once

#include <SFML/Graphics/Color.hpp>
#include <string>

namespace ly
{
	struct InfernoSprayVisualDefinition
	{
		sf::Color outerFlameColor{ 255, 120, 20, 200 };
		sf::Color coreFlameColor{ 255, 230, 80, 240 };
		float flickerIntensity = 0.15f;
		float pulseSpeed = 12.f;
		float coreWidthRatio = 0.45f;
	};

	struct InfernoSprayPresentationProfile
	{
		std::string profileId;
		InfernoSprayVisualDefinition visual;
	};

	bool RegisterInfernoSprayPresentationProfiles();
}
