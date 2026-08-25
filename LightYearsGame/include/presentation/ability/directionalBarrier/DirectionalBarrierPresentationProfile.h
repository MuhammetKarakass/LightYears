#pragma once

#include "content/ContentId.h"
#include "gameplay/ability/directionalBarrier/DirectionalBarrierContracts.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct DirectionalBarrierPresentationProfile
	{
		sas::ContentId profileId;
		float radius = AbilityData::DirectionalBarrier::BarrierRadius;
		float halfAngleDegrees = 62.f;
		float edgeThickness = 7.f;
		float pulseSpeed = 5.f;
		sf::Color edgeColor{ 100, 210, 255, 170 };

		// This profile is intentionally family-local. Directional Barrier is a
		// directional panel, not an AreaTelegraph circle or a generic shield
		// sprite, so its future visual variants should not grow a global visual
		// configuration type.
	};

	bool RegisterDirectionalBarrierPresentationProfiles();
}
