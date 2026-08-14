#pragma once

#include "abilities/AbilityPolicies.h"

#include <map>
#include <string>

namespace ly
{
	class AbilityLoadout
	{
	public:
		bool Bind(sas::AbilitySlot slot, const std::string& abilityId);
		bool Unbind(sas::AbilitySlot slot);
		const std::string* FindAbility(sas::AbilitySlot slot) const;
		sas::AbilitySlot FindSlot(const std::string& abilityId) const;

	private:
		std::map<sas::AbilitySlot, std::string> mBindings;
	};
}
