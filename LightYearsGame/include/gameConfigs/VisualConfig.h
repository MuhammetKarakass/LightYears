#pragma once

#include "gameConfigs/AbilityVisualStructs.h"

namespace VisualData
{
	inline const ly::ShieldVisualDefinition Shield_Basic{
		"Visual.Shield.Basic",
		"SpaceShooterRedux/PNG/Effects/shield3.png",
		sf::Color{ 110, 205, 255, 145 },
		{ 0.f, 0.f },
		1.15f,
		35.f,
		5.5f,
		0.08f,
		120.f,
		190.f,
		1.75f,
		0.65f
	};

	inline const ly::SunBeamVisualDefinition SunBeam_Basic{
		"Visual.SunBeam.Basic",
		sf::Color{ 255, 165, 35, 150 },
		sf::Color{ 255, 250, 220, 250 },
		sf::Color{ 255, 205, 90, 105 },
		sf::Color{ 255, 245, 205, 230 },
		0.45f,
		0.04f,
		0.18f,
		0.26f,
		1.35f,
		0.75f,
		1.9f,
		5.f,
		22.f
	};

	inline const ly::AreaTelegraphVisualDefinition SunBeam_Strike_Telegraph{
		"Visual.Telegraph.SunBeam.Strike",
		sf::Color{ 235, 30, 30, 85 },
		sf::Color{ 255, 245, 245, 255 },
		4.f,
		8.f,
		0.80f,
		1.f,
		0.035f
	};

	inline const ly::ShieldVisualDefinition* FindShieldVisualDefinition(const std::string& visualId)
	{
		return visualId == Shield_Basic.visualId ? &Shield_Basic : nullptr;
	}

	inline const ly::SunBeamVisualDefinition* FindSunBeamVisualDefinition(const std::string& visualId)
	{
		return visualId == SunBeam_Basic.visualId ? &SunBeam_Basic : nullptr;
	}

	inline const ly::AreaTelegraphVisualDefinition* FindAreaTelegraphVisualDefinition(const std::string& visualId)
	{
		return visualId == SunBeam_Strike_Telegraph.visualId ? &SunBeam_Strike_Telegraph : nullptr;
	}
}
