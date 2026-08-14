#pragma once

#include "abilities/AbilityCollection.h"
#include "abilities/AbilityGrantRules.h"
#include "abilities/AbilityRuntimeBinding.h"
#include "abilities/AbilityRuntimeSnapshot.h"

#include <cstddef>
#include <functional>
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sas
{
	template <typename Definition, typename Instance>
	struct AbilityRuntimeCallbacks
	{
		std::function<bool(const Definition&, std::string*)> validate;
		std::function<std::unique_ptr<Instance>(
			AbilityHandle,
			const Definition&,
			std::string*
		)> create;
		std::function<void(Instance&, AbilityEndReason)> cancel;
		std::function<void(Instance&, float)> tick;
		std::function<void(Instance&, bool)> setInput;
		std::function<AbilityRuntimeSnapshot(const Instance&)> snapshot;
		std::function<void(AbilityHandle)> granted;
		std::function<void(AbilityHandle)> removed;
		std::function<void(AbilityHandle)> changed;
		std::function<void()> cleared;
	};

	template <typename Definition, typename Instance>
	class AbilityRuntimeSystem
	{
	public:
		using Callbacks = AbilityRuntimeCallbacks<Definition, Instance>;
		using Collection = AbilityCollection<Instance>;

		explicit AbilityRuntimeSystem(
			std::size_t maxPassiveAbilities,
			Callbacks callbacks = {}
		)
			: mMaxPassiveAbilities{ maxPassiveAbilities },
			mCallbacks{ std::move(callbacks) }
		{
		}

		void SetCallbacks(Callbacks callbacks)
		{
			mCallbacks = std::move(callbacks);
		}

		AbilityHandle GrantAbility(
			const Definition& definition,
			std::string* failureReason = nullptr
		)
		{
			// Compatibility path for existing callers. New runtime loadout code
			// must pass an explicit binding instead of relying on content defaults.
			return GrantAbility(
				definition,
				AbilityRuntimeBinding{ definition.slot },
				failureReason
			);
		}

		AbilityHandle GrantAbility(
			const Definition& definition,
			AbilityRuntimeBinding binding,
			std::string* failureReason = nullptr
		)
		{
			if (!IsValidRuntimeBinding(definition, binding, failureReason))
			{
				return {};
			}
			// The runtime instance receives a bound copy. The catalog definition
			// remains immutable, while existing behavior code can still read the
			// effective slot from its runtime definition during this migration.
			Definition boundDefinition = definition;
			boundDefinition.slot = binding.slot;
			return GrantBoundAbility(boundDefinition, failureReason);
		}

		bool RebindAbility(
			AbilityHandle handle,
			AbilityRuntimeBinding binding,
			std::string* failureReason = nullptr
		)
		{
			Instance* ability = mAbilities.Find(handle);
			if (!ability)
			{
				if (failureReason)
				{
					*failureReason = "Cannot rebind an unknown ability handle.";
				}
				return false;
			}

			const AbilitySlot currentSlot = ability->GetDefinition().slot;
			if (currentSlot == binding.slot)
			{
				return true;
			}
			if (!IsLoadoutAbilitySlot(currentSlot) ||
				!IsLoadoutAbilitySlot(binding.slot))
			{
				if (failureReason)
				{
					*failureReason =
						"Only abilities already bound to Ability1 through Ability4 can be rebound.";
				}
				return false;
			}

			// Validation receives the same effective definition that the instance
			// will use after the rebind. This keeps rebinds subject to the exact
			// same content and behavior constraints as fresh grants, before an
			// occupied target slot is modified.
			Definition reboundDefinition = ability->GetDefinition();
			reboundDefinition.slot = binding.slot;
			if (mCallbacks.validate &&
				!mCallbacks.validate(reboundDefinition, failureReason))
			{
				return false;
			}

			const AbilityHandle occupyingHandle = mAbilities.FindHandle(binding.slot);
			if (occupyingHandle.IsValid() && !(occupyingHandle == handle))
			{
				RemoveAbility(occupyingHandle, AbilityEndReason::Cancelled);
			}

			ability = mAbilities.Find(handle);
			if (!ability || !mAbilities.Rebind(handle, binding.slot))
			{
				if (failureReason)
				{
					*failureReason = "Ability collection rejected the new runtime binding.";
				}
				return false;
			}
			ability->SetRuntimeSlot(binding.slot);
			if (mCallbacks.changed) mCallbacks.changed(handle);
			return true;
		}

	private:
		static bool IsValidRuntimeBinding(
			const Definition& definition,
			AbilityRuntimeBinding binding,
			std::string* failureReason
		)
		{
			if (definition.slot == AbilitySlot::PrimaryFire ||
				definition.slot == AbilitySlot::None)
			{
				if (definition.slot == binding.slot)
				{
					return true;
				}
				if (failureReason)
				{
					*failureReason = definition.slot == AbilitySlot::PrimaryFire
						? "PrimaryFire is reserved and cannot be assigned to a loadout slot."
						: "Passive abilities cannot be assigned to a loadout slot.";
				}
				return false;
			}

			if (IsLoadoutAbilitySlot(definition.slot) &&
				IsLoadoutAbilitySlot(binding.slot))
			{
				return true;
			}
			if (failureReason)
			{
				*failureReason =
					"Loadout abilities must be assigned to Ability1 through Ability4.";
			}
			return false;
		}

		AbilityHandle GrantBoundAbility(
			const Definition& definition,
			std::string* failureReason
		)
		{
			if (mCallbacks.validate &&
				!mCallbacks.validate(definition, failureReason))
			{
				return {};
			}
			if (const AbilityHandle existing =
					mAbilities.FindHandleById(definition.abilityId);
				existing.IsValid())
			{
				return existing;
			}
			if (IsGrantMutationBlocked(definition))
			{
				if (failureReason)
				{
					*failureReason =
						"Ability grant conflicts with an active replacement transaction.";
				}
				return {};
			}

			const bool passive = IsPassiveAbility(definition);
			if (!ValidateAbilityGrant(
				definition,
				mAbilities.GetPassiveAbilities().size(),
				mMaxPassiveAbilities,
				failureReason
			))
			{
				return {};
			}
			const bool reserveSlot = !passive;
			ReserveGrantMutation(definition, reserveSlot);
			if (!mCallbacks.create)
			{
				ReleaseGrantMutation(definition, reserveSlot);
				if (failureReason)
				{
					*failureReason = "Ability runtime has no instance factory.";
				}
				return {};
			}

			const AbilityHandle handle = mAbilities.AllocateHandle();
			std::unique_ptr<Instance> instance =
				mCallbacks.create(handle, definition, failureReason);
			if (!instance)
			{
				ReleaseGrantMutation(definition, reserveSlot);
				if (failureReason && failureReason->empty())
				{
					*failureReason = "Ability collection rejected the registration.";
				}
				return {};
			}

			if (!passive)
			{
				const AbilityHandle bound = mAbilities.FindHandle(definition.slot);
				if (bound.IsValid())
				{
					RemoveAbility(bound, AbilityEndReason::Cancelled);
				}
			}

			if (!mAbilities.Register(
				handle,
				definition.abilityId,
				definition.slot,
				passive,
				std::move(instance)
			))
			{
				ReleaseGrantMutation(definition, reserveSlot);
				if (failureReason && failureReason->empty())
				{
					*failureReason = "Ability collection rejected the registration.";
				}
				return {};
			}
			ReleaseGrantMutation(definition, reserveSlot);

			if (mCallbacks.granted) mCallbacks.granted(handle);
			if (mCallbacks.changed) mCallbacks.changed(handle);
			return handle;
		}

	public:

		bool RemoveAbility(
			AbilityHandle handle,
			AbilityEndReason reason = AbilityEndReason::Cancelled
		)
		{
			Instance* ability = mAbilities.Find(handle);
			if (!ability ||
				std::find(
					mRemovalInProgress.begin(),
					mRemovalInProgress.end(),
					handle
				) != mRemovalInProgress.end())
			{
				return false;
			}
			mRemovalInProgress.push_back(handle);
			if (mCallbacks.cancel)
			{
				mCallbacks.cancel(*ability, reason);
			}
			const bool removed = mAbilities.Remove(handle);
			mRemovalInProgress.pop_back();
			if (!removed)
			{
				return false;
			}
			if (mCallbacks.removed) mCallbacks.removed(handle);
			if (mCallbacks.changed) mCallbacks.changed(handle);
			return true;
		}

		void ClearSlot(AbilitySlot slot)
		{
			const AbilityHandle bound = mAbilities.FindHandle(slot);
			if (bound.IsValid())
			{
				RemoveAbility(bound, AbilityEndReason::Cancelled);
			}
		}

		void SetSlotInput(AbilitySlot slot, bool inputHeld)
		{
			Instance* ability = Find(slot);
			if (ability && mCallbacks.setInput)
			{
				mCallbacks.setInput(*ability, inputHeld);
			}
		}

		void Tick(float deltaTime)
		{
			if (!mCallbacks.tick)
			{
				return;
			}
			for (const AbilityHandle handle : GetHandles())
			{
				Instance* ability = mAbilities.Find(handle);
				if (ability)
				{
					mCallbacks.tick(*ability, deltaTime);
				}
			}
		}

		void Clear()
		{
			if (mCallbacks.cancel)
			{
				for (const AbilityHandle handle : GetHandles())
				{
					if (Instance* ability = mAbilities.Find(handle))
					{
						mCallbacks.cancel(
							*ability,
							AbilityEndReason::OwnerDestroyed
						);
					}
				}
			}
			mAbilities.Clear();
			if (mCallbacks.cleared) mCallbacks.cleared();
		}

		Instance* Find(AbilitySlot slot) { return mAbilities.Find(slot); }
		const Instance* Find(AbilitySlot slot) const { return mAbilities.Find(slot); }
		Instance* Find(AbilityHandle handle) { return mAbilities.Find(handle); }
		const Instance* Find(AbilityHandle handle) const { return mAbilities.Find(handle); }
		Instance* FindById(const std::string& abilityId)
		{
			return mAbilities.FindById(abilityId);
		}
		const Instance* FindById(const std::string& abilityId) const
		{
			return mAbilities.FindById(abilityId);
		}

		const std::vector<AbilityHandle>& GetPassiveAbilities() const
		{
			return mAbilities.GetPassiveAbilities();
		}

		typename Collection::InstanceMap& GetAll()
		{
			return mAbilities.GetAll();
		}

		const typename Collection::InstanceMap& GetAll() const
		{
			return mAbilities.GetAll();
		}

		std::vector<AbilityHandle> GetHandles() const
		{
			std::vector<AbilityHandle> handles;
			handles.reserve(mAbilities.GetAll().size());
			for (const auto& entry : mAbilities.GetAll())
			{
				handles.push_back(entry.first);
			}
			return handles;
		}

		std::vector<AbilityRuntimeSnapshot> BuildSnapshots() const
		{
			std::vector<AbilityRuntimeSnapshot> snapshots;
			if (!mCallbacks.snapshot)
			{
				return snapshots;
			}
			snapshots.reserve(mAbilities.GetAll().size());
			for (const AbilityHandle handle : GetHandles())
			{
				if (const Instance* ability = mAbilities.Find(handle))
				{
					snapshots.push_back(mCallbacks.snapshot(*ability));
				}
			}
			return snapshots;
		}

		bool IsGrantMutationBlocked(const Definition& definition) const
		{
			const bool slotBlocked =
				!IsPassiveAbility(definition) &&
				std::find(
					mGrantMutationSlots.begin(),
					mGrantMutationSlots.end(),
					definition.slot
				) != mGrantMutationSlots.end();
			const bool abilityIdBlocked = std::find(
				mGrantMutationAbilityIds.begin(),
				mGrantMutationAbilityIds.end(),
				definition.abilityId
			) != mGrantMutationAbilityIds.end();
			return slotBlocked || abilityIdBlocked;
		}

		void ReserveGrantMutation(
			const Definition& definition,
			bool reserveSlot
		)
		{
			if (reserveSlot)
			{
				mGrantMutationSlots.push_back(definition.slot);
			}
			mGrantMutationAbilityIds.push_back(definition.abilityId);
		}

		void ReleaseGrantMutation(
			const Definition& definition,
			bool releaseSlot
		)
		{
			if (releaseSlot)
			{
				const auto slot = std::find(
					mGrantMutationSlots.begin(),
					mGrantMutationSlots.end(),
					definition.slot
				);
				if (slot != mGrantMutationSlots.end())
				{
					mGrantMutationSlots.erase(slot);
				}
			}
			const auto abilityId = std::find(
				mGrantMutationAbilityIds.begin(),
				mGrantMutationAbilityIds.end(),
				definition.abilityId
			);
			if (abilityId != mGrantMutationAbilityIds.end())
			{
				mGrantMutationAbilityIds.erase(abilityId);
			}
		}

		std::size_t mMaxPassiveAbilities = 0;
		Callbacks mCallbacks;
		Collection mAbilities;
		std::vector<AbilityHandle> mRemovalInProgress;
		std::vector<AbilitySlot> mGrantMutationSlots;
		std::vector<std::string> mGrantMutationAbilityIds;
	};
}
