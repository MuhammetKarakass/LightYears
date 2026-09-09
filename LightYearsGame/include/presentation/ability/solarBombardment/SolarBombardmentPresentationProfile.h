#pragma once

#include "content/ContentId.h"
#include "presentation/ability/common/AreaTelegraphVisualDefinition.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct SolarBombardmentProjectileVisualDefinition
	{
		sf::Color outerColor{ 255, 115, 25, 220 };
		sf::Color coreColor{ 255, 238, 170, 255 };
		sf::Color trailColor{ 255, 90, 20, 155 };
		float radius = 14.f;
		float trailLength = 70.f;
		float pulseSpeed = 10.f;
	};

	struct SolarBombardmentExplosionVisualDefinition
	{
		sf::Color outerFillColor{ 255, 90, 15, 65 };
		sf::Color outerOutlineColor{ 255, 150, 35, 235 };
		sf::Color innerFillColor{ 255, 220, 95, 95 };
		sf::Color innerOutlineColor{ 255, 245, 190, 255 };
		sf::Color shockwaveColor{ 255, 245, 205, 220 };
		float outerRadius = 600.f;
		float innerRadius = 300.f;
		float duration = 0.32f;
		float outlineThickness = 4.f;
		float shockwaveThickness = 5.f;
	};

	struct SolarBombardmentPresentationProfile
	{
		sas::ContentId profileId;
		SolarBombardmentProjectileVisualDefinition projectile;
		SolarBombardmentExplosionVisualDefinition explosion;
		AreaTelegraphVisualDefinition outerTelegraph;
		AreaTelegraphVisualDefinition innerTelegraph;
	};

	bool RegisterSolarBombardmentPresentationProfiles();
}
