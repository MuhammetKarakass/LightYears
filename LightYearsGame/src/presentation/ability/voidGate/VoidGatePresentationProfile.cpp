#include "presentation/ability/voidGate/VoidGatePresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/voidGate/VoidGatePresentationIds.h"

namespace ly
{
	bool RegisterVoidGatePresentationProfiles()
	{
		static const bool registered = []
		{
			VoidGatePresentationProfile profile;
			profile.profileId = VoidGatePresentationIds::PortalBasic;
			profile.portal.outerColor = sf::Color{ 100, 40, 230, 235 };
			profile.portal.innerColor = sf::Color{ 30, 10, 100, 215 };
			profile.portal.glowColor = sf::Color{ 180, 100, 255, 150 };
			profile.portal.outerThickness = 5.f;
			profile.portal.innerThickness = 2.f;
			profile.portal.pulseSpeed = 5.f;
			profile.portal.pulseAmount = 0.06f;
			return PresentationProfileRegistry<VoidGatePresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
