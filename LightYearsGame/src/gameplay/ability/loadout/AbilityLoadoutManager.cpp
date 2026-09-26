#include "gameplay/ability/loadout/AbilityLoadoutManager.h"

#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"

namespace ly
{
	namespace
	{
		struct LoadoutMutationScope
		{
			bool& active;
			explicit LoadoutMutationScope(bool& flag) : active(flag) { active = true; }
			~LoadoutMutationScope() { active = false; }
		};
	}

	const std::string* AbilityLoadoutView::FindAbility(sas::AbilitySlot slot) const
	{
		if (!sas::IsLoadoutAbilitySlot(slot) || mSystem.IsClearPending()) return nullptr;
		const GameAbility* ability = mSystem.GetAbility(slot);
		return ability ? &ability->GetDefinition().abilityId : nullptr;
	}

	sas::AbilitySlot AbilityLoadoutView::FindSlot(const std::string& abilityId) const
	{
		for (const auto slot : { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2,
			sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 })
		{
			const auto* id = FindAbility(slot);
			if (id && *id == abilityId) return slot;
		}
		return sas::AbilitySlot::None;
	}

	void AbilityLoadoutManager::SynchronizeInventory()
	{
		if (mAbilitySystem.IsClearPending()) return;
		for (const auto slot : { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2,
			sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 })
		{
			if (const GameAbility* ability = mAbilitySystem.GetAbility(slot))
			{
				const auto& id = ability->GetDefinition().abilityId;
				if (!mInventory.Contains(id)) mInventory.Add(id);
			}
		}
	}
	AbilityLoadoutManager::AbilityLoadoutManager(
		LightYearsAbilitySystemComponent& abilitySystem
	)
		: mAbilitySystem{ abilitySystem }, mLoadout{ abilitySystem }
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
		if (failureReason) failureReason->clear();
		auto fail = [failureReason](const char* message)
		{
			if (failureReason && failureReason->empty()) *failureReason = message;
			return false;
		};
		if (!IsEquippableSlot(targetSlot))
		{
			return fail("Only Ability1 through Ability4 can be equipped by a player loadout.");
		}
		if (mMutationInProgress) return fail("A loadout mutation is already in progress.");
		LoadoutMutationScope mutation{ mMutationInProgress };
		if (abilityId.empty())
		{
			return fail("Ability ID must not be empty.");
		}
		const GameAbilityDefinition* definition =
			AbilityData::FindShippedAbilityDefinition(abilityId);
		if (!definition)
		{
			return fail("Ability is not present in the shipped content catalog.");
		}

		GameAbility* existing = mAbilitySystem.GetAbilityById(abilityId);
		try
		{
			if (existing)
			{
				if (!mAbilitySystem.RebindAbility(existing->GetHandle(), targetSlot, failureReason))
				{
					SynchronizeInventory();
					return fail("The ability runtime rejected the requested loadout binding.");
				}
			}
			else if (!mAbilitySystem.GrantAbility(*definition, targetSlot, failureReason).IsValid())
			{
				SynchronizeInventory();
				return fail("The ability runtime rejected the ability grant.");
			}
		}
		catch (...)
		{
			// Callback failure may happen after runtime commit. Reconcile the
			// canonical state before propagating the error; never claim rollback.
			SynchronizeInventory();
			throw;
		}
		SynchronizeInventory();
		return !mAbilitySystem.IsClearPending();
	}

	bool AbilityLoadoutManager::UnequipAbility(
		sas::AbilitySlot slot,
		std::string* failureReason
	)
	{
		if (mMutationInProgress)
		{
			if (failureReason) *failureReason = "A loadout mutation is already in progress.";
			return false;
		}
		LoadoutMutationScope mutation{ mMutationInProgress };
		if (!IsEquippableSlot(slot))
		{
			if (failureReason) *failureReason = "Only player ability slots can be unequipped.";
			return false;
		}
		GameAbility* ability = mAbilitySystem.GetAbility(slot);
		try
		{
			if (ability && !mAbilitySystem.RemoveAbility(ability->GetHandle()))
			{
				SynchronizeInventory();
				if (failureReason) *failureReason = "The equipped ability could not be removed.";
				return false;
			}
		}
		catch (...)
		{
			SynchronizeInventory();
			throw;
		}
		SynchronizeInventory();
		return true;
	}
}
