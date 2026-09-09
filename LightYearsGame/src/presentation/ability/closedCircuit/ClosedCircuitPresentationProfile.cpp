#include "presentation/ability/closedCircuit/ClosedCircuitPresentationProfile.h"
#include "presentation/ability/closedCircuit/ClosedCircuitPresentationIds.h"
#include "presentation/ability/PresentationProfileRegistry.h"
namespace ly
{
	bool RegisterClosedCircuitPresentationProfiles()
	{
		static const bool registered = [] { ClosedCircuitPresentationProfile profile; profile.profileId = ClosedCircuitPresentationIds::FieldBasic; return PresentationProfileRegistry<ClosedCircuitPresentationProfile>::Register(profile); }();
		return registered;
	}
}
