#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct BlastbackPresentationProfile
	{
		sas::ContentId profileId;
		float range = 400.f;
		float innerRange = 200.f;
		float halfAngleDegrees = 32.f;
		// The visual starts on this fixed inner arc around the ship rather than at
		// a pointed apex. Gameplay targeting still uses the authored cone angle.
		float emitterInnerRadius = 48.f;
		// Focus remains active for the ability's full duration, but the area reaches
		// its authored range early and is deliberately held there before discharge.
		float focusExpansionDuration = 0.4f;
		sf::Color innerColor{ 255, 115, 36, 150 };
		sf::Color outerColor{ 255, 54, 22, 104 };
		sf::Color burstColor{ 255, 225, 145, 220 };
		float focusPulseSpeed = 7.f;
		float burstLifetime = 0.22f;
	};

	bool RegisterBlastbackPresentationProfiles();
}
