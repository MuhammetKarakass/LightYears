#pragma once

#include "content/ContentId.h"
#include "presentation/ability/common/AreaTelegraphVisualDefinition.h"

namespace ly
{
	struct ShieldHarvestPresentationProfile
	{
		sas::ContentId profileId;
		AreaTelegraphVisualDefinition focusTelegraph;
	};

	bool RegisterShieldHarvestPresentationProfiles();
}
