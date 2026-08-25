#include "presentation/ability/directionalBarrier/DirectionalBarrierPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/directionalBarrier/DirectionalBarrierPresentationIds.h"

namespace ly
{
	bool RegisterDirectionalBarrierPresentationProfiles()
	{
		static const bool registered = []
		{
			DirectionalBarrierPresentationProfile profile;
			profile.profileId = DirectionalBarrierPresentationIds::Basic;
			profile.radius = AbilityData::DirectionalBarrier::BarrierRadius;
			profile.halfAngleDegrees = 62.f;
			profile.edgeThickness = 7.f;
			profile.pulseSpeed = 5.f;
			profile.edgeColor = sf::Color{ 100, 210, 255, 170 };
			return PresentationProfileRegistry<DirectionalBarrierPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
