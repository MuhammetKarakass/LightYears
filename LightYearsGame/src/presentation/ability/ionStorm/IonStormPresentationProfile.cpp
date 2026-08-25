#include "presentation/ability/ionStorm/IonStormPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/ionStorm/IonStormPresentationIds.h"

namespace ly
{
	namespace
	{
		IonStormProjectilePresentationProfile BuildProjectileProfile()
		{
			IonStormProjectilePresentationProfile profile;
			profile.profileId = IonStormPresentationIds::ProjectileBasic;
			profile.visual.outerColor = sf::Color{ 70, 160, 255, 210 };
			profile.visual.coreColor = sf::Color{ 205, 245, 255, 245 };
			profile.visual.trailColor = sf::Color{ 75, 155, 255, 150 };
			return profile;
		}

		IonStormFieldPresentationProfile BuildFieldProfile()
		{
			IonStormFieldPresentationProfile profile;
			profile.profileId = IonStormPresentationIds::FieldBasic;
			profile.visual.fillColor = sf::Color{ 45, 120, 255, 85 };
			return profile;
		}
	}

	bool RegisterIonStormPresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<
				IonStormProjectilePresentationProfile
			>::Register(BuildProjectileProfile()) &&
			PresentationProfileRegistry<
				IonStormFieldPresentationProfile
			>::Register(BuildFieldProfile());
		return registered;
	}
}
