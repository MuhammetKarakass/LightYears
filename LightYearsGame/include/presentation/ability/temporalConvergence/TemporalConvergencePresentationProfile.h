#pragma once

#include "content/ContentId.h"
#include "presentation/ability/common/AreaTelegraphVisualDefinition.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	// Visual-only values. Gameplay radius, duration, travel and shielding are
	// content attributes; this typed profile only establishes the family look.
	struct TemporalConvergencePresentationProfile
	{
		sas::ContentId profileId;
		sf::Color dormantCoreColor{ 124, 104, 255, 235 };
		sf::Color dormantGlowColor{ 90, 225, 255, 120 };
		sf::Color travelTrailColor{ 125, 210, 255, 180 };
		float dormantCoreRadius = 13.f;
		float dormantGlowRadius = 27.f;
		// The destination warning is a reusable AreaTelegraphActor. Keeping its
		// definition here lets this family choose its own countdown/impact look
		// without adding generic visual fields to every ability actor.
		AreaTelegraphVisualDefinition targetTelegraph;
	};

	bool RegisterTemporalConvergencePresentationProfiles();
}
