#include "gameplay/ability/loadout/AbilityInventory.h"

namespace ly
{
	bool AbilityInventory::Add(const std::string& abilityId)
	{
		if (abilityId.empty() || Contains(abilityId))
		{
			return false;
		}
		mAbilities.emplace(
			abilityId,
			OwnedAbilityState{ abilityId, 1 }
		);
		return true;
	}

	bool AbilityInventory::Contains(const std::string& abilityId) const
	{
		return mAbilities.find(abilityId) != mAbilities.end();
	}

	OwnedAbilityState* AbilityInventory::Find(const std::string& abilityId)
	{
		const auto found = mAbilities.find(abilityId);
		return found != mAbilities.end() ? &found->second : nullptr;
	}

	const OwnedAbilityState* AbilityInventory::Find(const std::string& abilityId) const
	{
		const auto found = mAbilities.find(abilityId);
		return found != mAbilities.end() ? &found->second : nullptr;
	}
}
