#pragma once

#include "abilities/AbilityPolicies.h"
#include "gameplay/ability/loadout/AbilityInventory.h"
#include "gameplay/ability/loadout/AbilityLoadout.h"

#include <string>

namespace ly
{
	class LightYearsAbilitySystemComponent;

	class AbilityLoadoutManager
	{
	public:
		explicit AbilityLoadoutManager(LightYearsAbilitySystemComponent& abilitySystem);

		bool AcquireAbility(
			const std::string& abilityId,
			std::string* failureReason = nullptr
		);
		bool EquipAbility(
			const std::string& abilityId,
			sas::AbilitySlot targetSlot,
			std::string* failureReason = nullptr
		);
		bool UnequipAbility(
			sas::AbilitySlot slot,
			std::string* failureReason = nullptr
		);

		const AbilityInventory& GetInventory() const { return mInventory; }
		const AbilityLoadout& GetLoadout() const { return mLoadout; }

	private:
		static bool IsEquippableSlot(sas::AbilitySlot slot);

		LightYearsAbilitySystemComponent& mAbilitySystem;
		AbilityInventory mInventory;
		AbilityLoadout mLoadout;
	};
}
