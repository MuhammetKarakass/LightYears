#include "presentation/ability/statusReaction/StatusReactionPresentationProfile.h"
#include "presentation/ability/PresentationProfileRegistry.h"

namespace vega
{
	bool RegisterStatusReactionPresentationProfiles()
	{
		using Registry = PresentationProfileRegistry<StatusReactionPresentationProfile>;
		static const bool registered =
			Registry::Register({ StatusReactionPresentationIds::Steam, { 225, 235, 245, 210 }, { 255, 255, 255, 170 }, 0.45f, 60.f, false }) &&
			Registry::Register({ StatusReactionPresentationIds::Magnet, { 175, 120, 255, 220 }, { 220, 190, 255, 190 }, 0.5f, 250.f, true }) &&
			Registry::Register({ StatusReactionPresentationIds::Plasma, { 255, 120, 220, 230 }, { 255, 190, 120, 210 }, 0.3f, 250.f, false }) &&
			Registry::Register({ StatusReactionPresentationIds::Fracture, { 150, 225, 255, 230 }, { 210, 245, 255, 200 }, 0.35f, 55.f, false }) &&
			Registry::Register({ StatusReactionPresentationIds::Superconduct, { 120, 200, 255, 235 }, { 255, 255, 140, 210 }, 0.3f, 50.f, false }) &&
			Registry::Register({ StatusReactionPresentationIds::Shrapnel, { 255, 185, 110, 230 }, { 255, 230, 190, 200 }, 0.25f, 120.f, false });
		return registered;
	}
}
