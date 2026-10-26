#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace vega
{
	// One readable flash per status reaction: an expanding ring plus a short core flash at the reacting target.
	namespace StatusReactionPresentationIds
	{
		inline const sas::ContentId Steam{ "Presentation.Ability.StatusReaction.Steam.Basic" };
		inline const sas::ContentId Magnet{ "Presentation.Ability.StatusReaction.Magnet.Basic" };
		inline const sas::ContentId Plasma{ "Presentation.Ability.StatusReaction.Plasma.Basic" };
		inline const sas::ContentId Fracture{ "Presentation.Ability.StatusReaction.Fracture.Basic" };
		inline const sas::ContentId Superconduct{ "Presentation.Ability.StatusReaction.Superconduct.Basic" };
		inline const sas::ContentId Shrapnel{ "Presentation.Ability.StatusReaction.Shrapnel.Basic" };
	}

	struct StatusReactionPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color ringColor{ 255, 255, 255, 220 };
		sf::Color coreColor{ 255, 255, 255, 200 };
		float duration = 0.35f;
		// Ring radius when the reaction has no area of its own.
		float defaultRadius = 70.f;
		// The ring contracts toward the target instead of expanding (pulls).
		bool contracting = false;
	};

	bool RegisterStatusReactionPresentationProfiles();
}
