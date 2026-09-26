#pragma once

#include "abilities/AbilityPolicies.h"
#include "gameplay/ability/loadout/AbilityInventory.h"
#include "gameplay/ability/loadout/AbilityLoadout.h"

#include <string>

namespace ly
{
	class LightYearsAbilitySystemComponent;

	// Live read-only projection. Runtime is the sole owner of equipped bindings.
	class AbilityLoadoutView
	{
	public:
		explicit AbilityLoadoutView(const LightYearsAbilitySystemComponent& system) : mSystem(system) {}
		// Pointer lifetime follows the runtime definition; copy the id across mutations.
		const std::string* FindAbility(sas::AbilitySlot slot) const;
		sas::AbilitySlot FindSlot(const std::string& abilityId) const;
	private:
		const LightYearsAbilitySystemComponent& mSystem;
	};

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
		const AbilityLoadoutView& GetLoadout() const { return mLoadout; }

	private:
		static bool IsEquippableSlot(sas::AbilitySlot slot);
		void SynchronizeInventory();

		LightYearsAbilitySystemComponent& mAbilitySystem;
		AbilityInventory mInventory;
		AbilityLoadoutView mLoadout;
		bool mMutationInProgress = false;
	};
}
