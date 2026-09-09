#include "presentation/ability/shieldGraft/ShieldGraftPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/shieldGraft/ShieldGraftPresentationIds.h"

namespace ly
{
	bool RegisterShieldGraftPresentationProfiles()
	{
		static const bool registered = []
		{
			ShieldGraftPresentationProfile profile;
			profile.profileId = ShieldGraftPresentationIds::VisualBasic;
			profile.outerRingColor = sf::Color{ 80, 200, 255, 220 };
			profile.innerFlashColor = sf::Color{ 95, 245, 160, 230 };
			profile.visualDuration = 0.35f;
			profile.startRadius = 80.f;
			profile.endRadius = 10.f;
			profile.ringThickness = 3.f;
			return PresentationProfileRegistry<ShieldGraftPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
