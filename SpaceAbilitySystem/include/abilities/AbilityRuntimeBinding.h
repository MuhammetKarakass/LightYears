#pragma once

#include "abilities/AbilityPolicies.h"

namespace sas
{
	// A definition describes what an ability is. This value describes where the
	// already-owned ability is currently equipped. Keeping the two concepts
	// separate lets a loadout move an ability without changing its content.
	struct AbilityRuntimeBinding
	{
		AbilitySlot slot = AbilitySlot::None;

		bool IsPassive() const
		{
			return slot == AbilitySlot::None;
		}
	};
}
