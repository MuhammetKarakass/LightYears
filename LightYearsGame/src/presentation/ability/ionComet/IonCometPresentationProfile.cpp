#include "presentation/ability/ionComet/IonCometPresentationProfile.h"
#include "presentation/ability/PresentationProfileRegistry.h"

namespace vega
{
	bool RegisterIonCometPresentationProfiles()
	{
		static const bool registered = PresentationProfileRegistry<IonCometPresentationProfile>::Register({ IonCometPresentationIds::Basic });
		return registered;
	}
}
