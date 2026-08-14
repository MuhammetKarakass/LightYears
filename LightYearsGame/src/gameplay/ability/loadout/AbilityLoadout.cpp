#include "gameplay/ability/loadout/AbilityLoadout.h"

namespace ly
{
	bool AbilityLoadout::Bind(
		sas::AbilitySlot slot,
		const std::string& abilityId
	)
	{
		if (slot == sas::AbilitySlot::None ||
			slot == sas::AbilitySlot::PrimaryFire ||
			abilityId.empty())
		{
			return false;
		}
		mBindings[slot] = abilityId;
		return true;
	}

	bool AbilityLoadout::Unbind(sas::AbilitySlot slot)
	{
		return mBindings.erase(slot) > 0;
	}

	const std::string* AbilityLoadout::FindAbility(sas::AbilitySlot slot) const
	{
		const auto found = mBindings.find(slot);
		return found != mBindings.end() ? &found->second : nullptr;
	}

	sas::AbilitySlot AbilityLoadout::FindSlot(const std::string& abilityId) const
	{
		for (const auto& [slot, boundAbilityId] : mBindings)
		{
			if (boundAbilityId == abilityId)
			{
				return slot;
			}
		}
		return sas::AbilitySlot::None;
	}
}
