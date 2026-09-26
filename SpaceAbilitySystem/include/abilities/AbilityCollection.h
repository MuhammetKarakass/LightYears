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
			std::unique_ptr<AbilityInstance> replacedInstance;
			return RegisterReplacing(
				handle,
				abilityId,
				slot,
				isPassive,
				AbilityHandle{},
				std::move(instance),
				replacedInstance
			);
		}

		bool CanRegisterReplacing(
			AbilityHandle handle,
			const std::string& abilityId,
			AbilitySlot slot,
			bool isPassive,
			AbilityHandle replacedHandle
		) const
		{
			if (!handle.IsValid() || abilityId.empty() ||
				mInstances.find(handle) != mInstances.end() ||
				mRegistrations.find(handle) != mRegistrations.end() ||
				mAbilityIds.find(abilityId) != mAbilityIds.end())
			{
				return false;
			}
			if (isPassive)
			{
				return !replacedHandle.IsValid();
			}
			if (slot == AbilitySlot::None)
			{
				return false;
			}

			const auto targetBinding = mSlotBindings.find(slot);
			if (!replacedHandle.IsValid())
			{
				return targetBinding == mSlotBindings.end();
			}
			if (replacedHandle == handle || targetBinding == mSlotBindings.end() ||
				!(targetBinding->second == replacedHandle))
			{
				return false;
			}

			const auto replacedRegistration = mRegistrations.find(replacedHandle);
			if (replacedRegistration == mRegistrations.end() ||
				replacedRegistration->second.isPassive ||
				replacedRegistration->second.slot != slot ||
				mInstances.find(replacedHandle) == mInstances.end())
			{
				return false;
			}
			const auto replacedId = mAbilityIds.find(replacedRegistration->second.abilityId);
			return replacedId != mAbilityIds.end() && replacedId->second == replacedHandle;
		}

		bool RegisterReplacing(
			AbilityHandle handle,
			const std::string& abilityId,
			AbilitySlot slot,
			bool isPassive,
			AbilityHandle replacedHandle,
			std::unique_ptr<AbilityInstance> instance,
			std::unique_ptr<AbilityInstance>& replacedInstance
		)
		{
			if (!instance || replacedInstance ||
				!CanRegisterReplacing(handle, abilityId, slot, isPassive, replacedHandle))
			{
				return false;
			}

			if (replacedHandle.IsValid())
			{
				auto replacedRegistration = mRegistrations.find(replacedHandle);
				mAbilityIds.erase(replacedRegistration->second.abilityId);
				auto replaced = mInstances.find(replacedHandle);
				replacedInstance = std::move(replaced->second);
				mInstances.erase(replaced);
				mRegistrations.erase(replacedRegistration);
				mSlotBindings.find(slot)->second = handle;
			}
			else if (!isPassive)
			{
				mSlotBindings.emplace(slot, handle);
			}

			mInstances.emplace(handle, std::move(instance));
			mRegistrations.emplace(handle, Registration{ abilityId, slot, isPassive });
			mAbilityIds.emplace(abilityId, handle);
			if (isPassive)
			{
				mPassiveAbilities.push_back(handle);
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
			std::unique_ptr<AbilityInstance> replacedInstance;
			return RebindReplacing(handle, newSlot, AbilityHandle{}, replacedInstance);
		}

		bool CanRebindReplacing(
			AbilityHandle handle,
			AbilitySlot newSlot,
			AbilityHandle replacedHandle
		) const
		{
			const auto registration = mRegistrations.find(handle);
			if (registration == mRegistrations.end() ||
				registration->second.isPassive ||
				newSlot == AbilitySlot::None ||
				mInstances.find(handle) == mInstances.end())
			{
				return false;
			}

			const auto oldBinding = mSlotBindings.find(registration->second.slot);
			if (oldBinding == mSlotBindings.end() || !(oldBinding->second == handle))
			{
				return false;
			}
			const auto targetBinding = mSlotBindings.find(newSlot);
			if (newSlot == registration->second.slot)
			{
				return !replacedHandle.IsValid() &&
					targetBinding != mSlotBindings.end() && targetBinding->second == handle;
			}
			if (!replacedHandle.IsValid())
			{
				return targetBinding == mSlotBindings.end();
			}
			if (replacedHandle == handle || targetBinding == mSlotBindings.end() ||
				!(targetBinding->second == replacedHandle))
			{
				return false;
			}

			const auto replacedRegistration = mRegistrations.find(replacedHandle);
			if (replacedRegistration == mRegistrations.end() ||
				replacedRegistration->second.isPassive ||
				replacedRegistration->second.slot != newSlot ||
				mInstances.find(replacedHandle) == mInstances.end())
			{
				return false;
			}
			const auto replacedId = mAbilityIds.find(replacedRegistration->second.abilityId);
			return replacedId != mAbilityIds.end() && replacedId->second == replacedHandle;
		}

		bool RebindReplacing(
			AbilityHandle handle,
			AbilitySlot newSlot,
			AbilityHandle replacedHandle,
			std::unique_ptr<AbilityInstance>& replacedInstance
		)
		{
			if (replacedInstance ||
				!CanRebindReplacing(handle, newSlot, replacedHandle))
			{
				return false;
			}

			auto registration = mRegistrations.find(handle);
			const AbilitySlot oldSlot = registration->second.slot;
			if (replacedHandle.IsValid())
			{
				auto replacedRegistration = mRegistrations.find(replacedHandle);
				mAbilityIds.erase(replacedRegistration->second.abilityId);
				auto replaced = mInstances.find(replacedHandle);
				replacedInstance = std::move(replaced->second);
				mInstances.erase(replaced);
				mRegistrations.erase(replacedRegistration);
				mSlotBindings.find(newSlot)->second = handle;
				auto oldBinding = mSlotBindings.find(oldSlot);
				if (oldBinding != mSlotBindings.end() && oldBinding->second == handle)
				{
					mSlotBindings.erase(oldBinding);
				}
			}
			else if (oldSlot != newSlot)
			{
				const auto oldBinding = mSlotBindings.find(oldSlot);
				mSlotBindings.erase(oldBinding);
				mSlotBindings.emplace(newSlot, handle);
			}
			registration->second.slot = newSlot;
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
