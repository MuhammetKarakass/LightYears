#include "presentation/ability/mineLayer/MineLayerPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/mineLayer/MineLayerPresentationIds.h"

namespace ly
{
	namespace
	{
		MineLayerPresentationProfile BuildBasicMineProfile()
		{
			MineLayerPresentationProfile profile;
			profile.profileId = MineLayerPresentationIds::MineBasic;
			profile.visual.bodyColor = sf::Color{ 45, 130, 255, 235 };
			profile.visual.coreColor = sf::Color{ 235, 252, 255, 255 };
			profile.visual.triggerRingColor = sf::Color{ 70, 175, 255, 110 };
			profile.visual.explosionColor = sf::Color{ 100, 205, 255, 220 };
			profile.visual.explosionRingColor = sf::Color{ 225, 250, 255, 245 };
			return profile;
		}
	}

	bool RegisterMineLayerPresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<MineLayerPresentationProfile>::Register(
				BuildBasicMineProfile()
			);
		return registered;
	}
}
