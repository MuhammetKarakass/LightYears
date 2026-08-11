#pragma once

#include "VFX/Explosion.h"
#include "content/ContentId.h"
#include "presentation/ability/common/AreaTelegraphVisualDefinition.h"

#include <SFML/Graphics/Color.hpp>
#include <string>

namespace ly
{
	struct OverdriveCoreVisualDefinition
	{
		sf::Color bodyColor{ 255, 125, 35, 245 };
		sf::Color coreColor{ 255, 245, 170, 255 };
		sf::Color trailColor{ 255, 70, 20, 165 };
		sf::Color impactColor{ 255, 190, 75, 240 };
		float bodyRadius = 4.f;
		float trailLength = 24.f;
		float trailWidth = 8.f;
		float glowRadius = 9.f;
		float pulseSpeed = 32.f;
		float impactVisualDuration = 0.22f;
		float impactRingEndScale = 1.6f;
		float impactRingThickness = 3.f;
		float screenShakeAmplitude = 1.5f;
		float screenShakeDuration = 0.06f;
		float screenShakeFrequency = 44.f;
	};

	struct OverdriveCorePresentationProfile
	{
		sas::ContentId profileId;
		std::string texturePath;
		OverdriveCoreVisualDefinition visual;
		AreaTelegraphVisualDefinition telegraph;
		ExplosionType explosionType = ExplosionType::Small;
	};

	bool RegisterOverdriveCorePresentationProfiles();
}
