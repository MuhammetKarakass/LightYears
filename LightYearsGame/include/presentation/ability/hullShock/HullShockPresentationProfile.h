#pragma once

#include "content/ContentId.h"
#include "presentation/ability/common/AreaTelegraphVisualDefinition.h"

namespace ly
{
	struct HullShockPresentationProfile
	{
		sas::ContentId profileId;
		AreaTelegraphVisualDefinition focusTelegraph;
	};

	bool RegisterHullShockPresentationProfiles();
}
