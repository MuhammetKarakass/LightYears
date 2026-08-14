#pragma once

#include "gameplay/ability/loadout/OwnedAbilityState.h"

#include <map>
#include <string>

namespace ly
{
	class AbilityInventory
	{
	public:
		bool Add(const std::string& abilityId);
		bool Contains(const std::string& abilityId) const;
		OwnedAbilityState* Find(const std::string& abilityId);
		const OwnedAbilityState* Find(const std::string& abilityId) const;

	private:
		std::map<std::string, OwnedAbilityState> mAbilities;
	};
}
