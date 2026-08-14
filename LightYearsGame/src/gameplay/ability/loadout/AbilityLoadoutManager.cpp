#include "gameplay/ability/loadout/AbilityLoadoutManager.h"

#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"

namespace ly
{
	AbilityLoadoutManager::AbilityLoadoutManager(
		LightYearsAbilitySystemComponent& abilitySystem
	)
		: mAbilitySystem{ abilitySystem }
	{
	}

	bool AbilityLoadoutManager::IsEquippableSlot(sas::AbilitySlot slot)
	{
		return sas::IsLoadoutAbilitySlot(slot);
	}

	bool AbilityLoadoutManager::AcquireAbility(
		const std::string& abilityId,
		std::string* failureReason
	)
	{
		if (abilityId.empty())
		{
			if (failureReason) *failureReason = "Ability ID must not be empty.";
			return false;
		}
		if (mInventory.Contains(abilityId))
		{
			return true;
		}
		if (!AbilityData::FindShippedAbilityDefinition(abilityId))
		{
			if (failureReason)
			{
				*failureReason = "Ability is not present in the shipped content catalog.";
			}
			return false;
		}
		mInventory.Add(abilityId);
		return true;
	}

	bool AbilityLoadoutManager::EquipAbility(
		const std::string& abilityId,
		sas::AbilitySlot targetSlot,
		std::string* failureReason
	)
	{
		if (!IsEquippableSlot(targetSlot))
		{
			if (failureReason)
			{
				*failureReason = "Only Ability1 through Ability4 can be equipped by a player loadout.";
			}
			return false;
		}
		if (!AcquireAbility(abilityId, failureReason))
		{
			return false;
		}

		if (const std::string* currentId = mLoadout.FindAbility(targetSlot))
		{
			if (*currentId == abilityId)
			{
				return true;
			}
			if (!UnequipAbility(targetSlot, failureReason))
			{
				return false;
			}
		}

		if (GameAbility* existing = mAbilitySystem.GetAbilityById(abilityId))
		{
			const sas::AbilitySlot oldSlot = mLoadout.FindSlot(abilityId);
			if (oldSlot != sas::AbilitySlot::None)
			{
				mLoadout.Unbind(oldSlot);
			}
			if (!mAbilitySystem.RebindAbility(
				existing->GetHandle(),
				targetSlot,
				failureReason
			))
			{
				return false;
			}
		}
		else
		{
			const GameAbilityDefinition* definition =
				AbilityData::FindShippedAbilityDefinition(abilityId);
			if (!definition ||
				!mAbilitySystem.GrantAbility(*definition, targetSlot, failureReason).IsValid())
			{
				return false;
			}
		}

		return mLoadout.Bind(targetSlot, abilityId);
	}

	bool AbilityLoadoutManager::UnequipAbility(
		sas::AbilitySlot slot,
		std::string* failureReason
	)
	{
		if (!IsEquippableSlot(slot))
		{
			if (failureReason) *failureReason = "Only player ability slots can be unequipped.";
			return false;
		}
		GameAbility* ability = mAbilitySystem.GetAbility(slot);
		if (ability && !mAbilitySystem.RemoveAbility(ability->GetHandle()))
		{
			if (failureReason) *failureReason = "The equipped ability could not be removed.";
			return false;
		}
		mLoadout.Unbind(slot);
		return true;
	}
}
