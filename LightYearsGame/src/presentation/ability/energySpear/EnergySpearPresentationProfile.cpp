#include "presentation/ability/energySpear/EnergySpearPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/energySpear/EnergySpearPresentationIds.h"

namespace ly
{
	namespace
	{
		EnergySpearPresentationProfile BuildBasicEnergySpearProfile()
		{
			EnergySpearPresentationProfile profile;
			profile.profileId = EnergySpearPresentationIds::ChargeBasic;
			profile.chargeTelegraph.glowColor = sf::Color{ 35, 210, 255, 100 };
			profile.chargeTelegraph.outerColor = sf::Color{ 75, 225, 255, 220 };
			profile.chargeTelegraph.coreColor = sf::Color{ 245, 255, 255, 255 };
			profile.chargeTelegraph.endpointColor = sf::Color{ 150, 245, 255, 235 };
			profile.chargeTelegraph.endpointRingColor = sf::Color{ 240, 255, 255, 255 };
			profile.chargeTelegraph.glowWidth = 38.f;
			profile.chargeTelegraph.outerWidth = 18.f;
			profile.chargeTelegraph.coreWidth = 5.f;
			profile.chargeTelegraph.endpointRadius = 12.f;
			profile.chargeTelegraph.pulseSpeed = 15.f;
			profile.chargeTelegraph.minimumAlpha = 0.55f;
			profile.chargeTelegraph.maximumAlpha = 1.f;
			// Energy Spear replaces the charge preview with its traversal visual
			// immediately on release; it must not leave a completion tail behind.
			profile.chargeTelegraph.completionFeedbackDuration = 0.f;

			profile.traversal.impactColor = sf::Color{ 235, 255, 255, 245 };
			profile.traversal.impactRingColor = sf::Color{ 65, 215, 255, 235 };
			return profile;
		}
	}

	bool RegisterEnergySpearPresentationProfiles()
	{
		static const bool registered = []
		{
			EnergySpearPresentationProfile profile = BuildBasicEnergySpearProfile();
			const bool chargeRegistered =
				PresentationProfileRegistry<EnergySpearPresentationProfile>::Register(profile);

			profile.profileId = EnergySpearPresentationIds::TraversalBasic;
			return chargeRegistered &&
				PresentationProfileRegistry<EnergySpearPresentationProfile>::Register(profile);
		}();
		return registered;
	}
}
