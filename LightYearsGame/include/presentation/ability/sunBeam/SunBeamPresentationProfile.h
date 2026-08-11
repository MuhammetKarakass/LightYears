#pragma once

#include "presentation/ability/common/AreaTelegraphVisualDefinition.h"
#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>

namespace ly
{
	struct SunBeamVisualDefinition
	{
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
		float impactFlashDuration = 0.07f;
		float screenShakeAmplitude = 5.f;
		float screenShakeDuration = 0.11f;
		float screenShakeFrequency = 40.f;
	};

	struct SunBeamPresentationProfile
	{
		sas::ContentId profileId;
		SunBeamVisualDefinition visual;
		AreaTelegraphVisualDefinition telegraph;
	};

	bool RegisterSunBeamPresentationProfiles();
}
