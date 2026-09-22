#pragma once

#include "content/ContentId.h"
#include "presentation/ability/common/AreaTelegraphVisualDefinition.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	// Storm Mark uses the shared ElectricArcVisualActor for its lightning. The
	// family-local profile owns only presentation tuning and remains type-safe.
	struct StormMarkPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color lightningColor{ 65, 190, 255, 235 };
		sf::Color impactColor{ 235, 255, 255, 255 };
		float strikeHeight = 130.f;
		// Ring drawn around the caster while the mark resolves, so the searched area is
		// readable before the strikes land.
		AreaTelegraphVisualDefinition searchArea;
	};

	bool RegisterStormMarkPresentationProfiles();
}
