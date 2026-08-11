#pragma once

#include "VFX/Explosion.h"
#include "content/ContentId.h"
#include "presentation/ability/common/AreaTelegraphVisualDefinition.h"

#include <SFML/Graphics/Color.hpp>
#include <string>

namespace ly
{
	struct RocketVisualDefinition
	{
		sf::Color exhaustOuterColor{ 255, 95, 35, 175 };
		sf::Color exhaustCoreColor{ 255, 235, 155, 245 };
		sf::Color flightGlowColor{ 255, 125, 55, 125 };
		sf::Color impactFlashColor{ 255, 235, 175, 245 };
		sf::Color impactRingColor{ 255, 120, 45, 235 };
		float trailLength = 58.f;
		float trailWidth = 16.f;
		float coreTrailWidthScale = 0.38f;
		float flightGlowRadius = 18.f;
		float pulseSpeed = 24.f;
		float impactVisualDuration = 0.34f;
		float impactFlashEndScale = 1.35f;
		float impactRingEndScale = 1.9f;
		float impactRingThickness = 5.f;
		float screenShakeAmplitude = 3.5f;
		float screenShakeDuration = 0.1f;
		float screenShakeFrequency = 36.f;
	};

	struct RocketPresentationProfile
	{
		sas::ContentId profileId;
		std::string texturePath;
		RocketVisualDefinition visual;
		AreaTelegraphVisualDefinition telegraph;
		ExplosionType explosionType = ExplosionType::Small;
	};

	bool RegisterRocketPresentationProfiles();
}
