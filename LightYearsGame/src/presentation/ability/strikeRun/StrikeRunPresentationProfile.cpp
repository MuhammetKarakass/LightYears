#include "presentation/ability/strikeRun/StrikeRunPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/strikeRun/StrikeRunPresentationIds.h"

namespace ly
{
	namespace
	{
		StrikeRunPresentationProfile BuildBasicProfile()
		{
			StrikeRunPresentationProfile profile;
			profile.profileId = StrikeRunPresentationIds::BombardmentBasic;
			// Blue-white kinetic warning makes the long line readable without
			// implying a persistent elemental field.
			profile.telegraph.lineColor = sf::Color{ 80, 165, 255, 150 };
			profile.telegraph.linePulseColor = sf::Color{ 155, 225, 255, 230 };
			profile.telegraph.markerFillColor = sf::Color{ 35, 120, 255, 55 };
			profile.telegraph.markerOutlineColor = sf::Color{ 115, 210, 255, 220 };
			profile.explosion.outerFillColor = sf::Color{ 30, 120, 255, 95 };
			profile.explosion.outerOutlineColor = sf::Color{ 120, 215, 255, 240 };
			profile.explosion.shockwaveColor = sf::Color{ 225, 250, 255, 240 };
			profile.explosion.craftColor = sf::Color{ 190, 235, 255, 235 };
			return profile;
		}
	}

	bool RegisterStrikeRunPresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<StrikeRunPresentationProfile>::Register(
				BuildBasicProfile()
			);
		return registered;
	}
}
