#pragma once

#include "abilities/AbilityHandle.h"
#include "abilities/AbilityPolicies.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace sas
{
	template <typename AbilityInstance>
	class AbilityCollection
	{
	public:
		using InstanceMap = std::map<AbilityHandle, std::unique_ptr<AbilityInstance>>;

		AbilityHandle AllocateHandle()
		{
			AbilityHandle handle;
			do
			{
				handle = AbilityHandle{ mNextHandleId++ };
			}
			while (mInstances.find(handle) != mInstances.end());
			return handle;
		}

		bool Register(
			AbilityHandle handle,
			const std::string& abilityId,
			AbilitySlot slot,
			bool isPassive,
			std::unique_ptr<AbilityInstance> instance
		)
		{
			if (!handle.IsValid() || abilityId.empty() || !instance ||
				mInstances.find(handle) != mInstances.end() ||
				mAbilityIds.find(abilityId) != mAbilityIds.end())
			{
				return false;
			}
			if (!isPassive && mSlotBindings.find(slot) != mSlotBindings.end())
			{
				return false;
			}

			mInstances.emplace(handle, std::move(instance));
			mRegistrations.emplace(handle, Registration{ abilityId, slot, isPassive });
			mAbilityIds.emplace(abilityId, handle);
			if (isPassive)
			{
				mPassiveAbilities.push_back(handle);
			}
			else
			{
				mSlotBindings.emplace(slot, handle);
			}
			return true;
		}

		bool Remove(AbilityHandle handle)
		{
			const auto registration = mRegistrations.find(handle);
			if (registration == mRegistrations.end())
			{
				return false;
			}

			mAbilityIds.erase(registration->second.abilityId);
			if (registration->second.isPassive)
			{
				mPassiveAbilities.erase(
					std::remove(mPassiveAbilities.begin(), mPassiveAbilities.end(), handle),
					mPassiveAbilities.end()
				);
			}
			else
			{
				const auto bound = mSlotBindings.find(registration->second.slot);
				if (bound != mSlotBindings.end() && bound->second == handle)
				{
					mSlotBindings.erase(bound);
				}
			}
			mRegistrations.erase(registration);
			return mInstances.erase(handle) > 0;
		}

		bool Rebind(AbilityHandle handle, AbilitySlot newSlot)
		{
			const auto registration = mRegistrations.find(handle);
			if (registration == mRegistrations.end() ||
				registration->second.isPassive ||
				newSlot == AbilitySlot::None ||
				(mSlotBindings.find(newSlot) != mSlotBindings.end() &&
					!(mSlotBindings.find(newSlot)->second == handle)))
			{
				return false;
			}

			const auto oldBinding = mSlotBindings.find(registration->second.slot);
			if (oldBinding != mSlotBindings.end() && oldBinding->second == handle)
			{
				mSlotBindings.erase(oldBinding);
			}
			registration->second.slot = newSlot;
			mSlotBindings[newSlot] = handle;
			return true;
		}

		AbilityInstance* Find(AbilityHandle handle)
		{
			const auto found = mInstances.find(handle);
			return found != mInstances.end() ? found->second.get() : nullptr;
		}

		const AbilityInstance* Find(AbilityHandle handle) const
		{
			const auto found = mInstances.find(handle);
			return found != mInstances.end() ? found->second.get() : nullptr;
		}

		AbilityInstance* Find(AbilitySlot slot)
		{
			const AbilityHandle handle = FindHandle(slot);
			return handle.IsValid() ? Find(handle) : nullptr;
		}

		const AbilityInstance* Find(AbilitySlot slot) const
		{
			const AbilityHandle handle = FindHandle(slot);
			return handle.IsValid() ? Find(handle) : nullptr;
		}

		AbilityInstance* FindById(const std::string& abilityId)
		{
			const AbilityHandle handle = FindHandleById(abilityId);
			return handle.IsValid() ? Find(handle) : nullptr;
		}

		const AbilityInstance* FindById(const std::string& abilityId) const
		{
			const AbilityHandle handle = FindHandleById(abilityId);
			return handle.IsValid() ? Find(handle) : nullptr;
		}

		AbilityHandle FindHandle(AbilitySlot slot) const
		{
			const auto found = mSlotBindings.find(slot);
			return found != mSlotBindings.end() ? found->second : AbilityHandle{};
		}

		AbilityHandle FindHandleById(const std::string& abilityId) const
		{
			const auto found = mAbilityIds.find(abilityId);
			return found != mAbilityIds.end() ? found->second : AbilityHandle{};
		}

		const std::vector<AbilityHandle>& GetPassiveAbilities() const
		{
			return mPassiveAbilities;
		}

		const InstanceMap& GetAll() const { return mInstances; }
		InstanceMap& GetAll() { return mInstances; }

		void Clear()
		{
			mInstances.clear();
			mRegistrations.clear();
			mSlotBindings.clear();
			mAbilityIds.clear();
			mPassiveAbilities.clear();
			mNextHandleId = 1;
		}

	private:
		struct Registration
		{
			std::string abilityId;
			AbilitySlot slot = AbilitySlot::None;
			bool isPassive = false;
		};

		InstanceMap mInstances;
		std::map<AbilityHandle, Registration> mRegistrations;
		std::map<AbilitySlot, AbilityHandle> mSlotBindings;
		std::map<std::string, AbilityHandle> mAbilityIds;
		std::vector<AbilityHandle> mPassiveAbilities;
		unsigned int mNextHandleId = 1;
	};
}
