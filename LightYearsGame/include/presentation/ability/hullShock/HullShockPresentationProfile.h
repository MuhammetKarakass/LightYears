#pragma once

#include "content/ContentId.h"
#include "presentation/ability/common/AreaTelegraphVisualDefinition.h"

namespace vega
{
	struct HullShockPresentationProfile
	{
		sas::ContentId profileId;
		AreaTelegraphVisualDefinition focusTelegraph;
	};

	bool RegisterHullShockPresentationProfiles();
}
