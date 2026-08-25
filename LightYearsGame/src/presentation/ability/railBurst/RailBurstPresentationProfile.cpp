#include "presentation/ability/railBurst/RailBurstPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/railBurst/RailBurstPresentationIds.h"

namespace ly
{
	namespace
	{
		RailBurstPresentationProfile BuildBasicRailBurstProfile()
		{
			RailBurstPresentationProfile profile;
			profile.profileId = RailBurstPresentationIds::ProjectileBasic;
			profile.visual.outerColor = sf::Color{ 45, 195, 255, 185 };
			profile.visual.coreColor = sf::Color{ 245, 255, 255, 255 };
			profile.visual.glowColor = sf::Color{ 40, 215, 255, 125 };
			profile.visual.impactColor = sf::Color{ 235, 255, 255, 245 };
			profile.visual.impactRingColor = sf::Color{ 70, 210, 255, 235 };
			profile.visual.bodyLength = 190.f;
			profile.visual.bodyWidth = 7.f;
			profile.visual.coreWidth = 1.8f;
			profile.visual.trailLength = 360.f;
			profile.visual.trailWidth = 24.f;
			profile.visual.glowRadius = 7.f;
			profile.visual.pulseSpeed = 70.f;
			profile.visual.headLength = 30.f;
			profile.visual.railSeparation = 10.f;
			profile.visual.railThickness = 2.f;
			profile.visual.afterimageSpacing = 46.f;
			profile.visual.afterimageDecay = 0.42f;
			profile.visual.afterimageCount = 3;
			return profile;
		}
	}

	bool RegisterRailBurstPresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<RailBurstPresentationProfile>::Register(
				BuildBasicRailBurstProfile()
			);
		return registered;
	}
}
