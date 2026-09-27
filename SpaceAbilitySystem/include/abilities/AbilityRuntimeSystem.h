#pragma once

#include "abilities/AbilityCollection.h"
#include "abilities/AbilityGrantRules.h"
#include "abilities/AbilityRuntimeBinding.h"
#include "abilities/AbilityRuntimeSnapshot.h"

#include <cstddef>
#include <exception>
#include <optional>
#include <functional>
#include <algorithm>
#include <memory>
#include <stdexcept>
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
		std::function<bool()> canMutate;
		std::function<bool(const Instance&)> hasPendingCleanup;
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
			if (failureReason) failureReason->clear();
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
			if (failureReason) failureReason->clear();
			if (mClearing || (mCallbacks.canMutate && !mCallbacks.canMutate())) return {};
			if (!IsValidRuntimeBinding(definition, binding, failureReason))
			{
				return {};
			}
			// The runtime instance receives a bound copy. The catalog definition
			// remains immutable, while existing behavior code can still read the
			// effective slot from its runtime definition during this migration.
			Definition boundDefinition = definition;
			boundDefinition.slot = binding.slot;
			const AbilityHandle handle = ExecuteMutation([&] { return GrantBoundAbility(boundDefinition, failureReason); });
			if (handle.IsValid() && !mAbilities.Find(handle))
			{
				if (failureReason) *failureReason = "Ability grant was cleared by a callback.";
				return {};
			}
			return handle;
		}

		bool RebindAbility(
			AbilityHandle handle,
			AbilityRuntimeBinding binding,
			std::string* failureReason = nullptr
		)
		{
			if (mClearing || (mCallbacks.canMutate && !mCallbacks.canMutate())) return false;
			const bool rebound = ExecuteMutation([&] { return RebindAbilityImpl(handle, binding, failureReason); });
			const Instance* ability = mAbilities.Find(handle);
			if (rebound && (!ability || ability->GetDefinition().slot != binding.slot))
			{
				if (failureReason) *failureReason = "Ability rebind was cleared by a callback.";
				return false;
			}
			return rebound;
		}

	private:
		bool RebindAbilityImpl(AbilityHandle handle, AbilityRuntimeBinding binding, std::string* failureReason)
		{
			if (failureReason) failureReason->clear();
			auto fail = [failureReason](const char* message)
			{
				if (failureReason && failureReason->empty()) *failureReason = message;
				return false;
			};
			Instance* ability = mAbilities.Find(handle);
			if (!ability)
			{
				return fail("Cannot rebind an unknown ability handle.");
			}

			const AbilitySlot currentSlot = ability->GetDefinition().slot;
			if (currentSlot == binding.slot)
			{
				return true;
			}
			if (!IsLoadoutAbilitySlot(currentSlot) ||
				!IsLoadoutAbilitySlot(binding.slot))
			{
				return fail(
					"Only abilities already bound to Ability1 through Ability4 can be rebound."
				);
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
				return fail("Ability definition validation rejected the rebind.");
			}

			ability = mAbilities.Find(handle);
			if (!ability || ability->GetDefinition().slot != currentSlot)
			{
				return fail("The ability changed while its rebind was being validated.");
			}
			if (IsSlotMutationBlocked(currentSlot) ||
				IsSlotMutationBlocked(binding.slot) ||
				IsGrantMutationBlocked(reboundDefinition) ||
				IsReplacementMutationHandleBlocked(handle))
			{
				return fail("Ability rebind conflicts with an active replacement transaction.");
			}

			const AbilityHandle occupyingHandle = mAbilities.FindHandle(binding.slot);
			if (!mAbilities.CanRebindReplacing(handle, binding.slot, occupyingHandle))
			{
				return fail("Ability collection rejected the new runtime binding.");
			}

			ReserveGrantMutation(reboundDefinition, true);
			mGrantMutationSlots.push_back(currentSlot);
			ReserveReplacementMutationHandle(handle);
			std::unique_ptr<Instance> replacedInstance;
			if (!mAbilities.RebindReplacing(
				handle,
				binding.slot,
				occupyingHandle,
				replacedInstance
			))
			{
				ReleaseReplacementMutationHandle(handle);
				ReleaseGrantMutationSlot(currentSlot);
				ReleaseGrantMutation(reboundDefinition, true);
				return fail("Ability collection rejected the new runtime binding.");
			}
			ability->SetRuntimeSlot(binding.slot);
			NotifyReplacedAbility(occupyingHandle, replacedInstance);
			if (mCallbacks.changed) mCallbacks.changed(handle);
			ReleaseReplacementMutationHandle(handle);
			ReleaseGrantMutationSlot(currentSlot);
			ReleaseGrantMutation(reboundDefinition, true);
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
				if (failureReason && failureReason->empty())
				{
					*failureReason = "Ability definition validation rejected the grant.";
				}
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

			const AbilityHandle replacedHandle = passive
				? AbilityHandle{}
				: mAbilities.FindHandle(definition.slot);
			if (!mAbilities.CanRegisterReplacing(
				handle,
				definition.abilityId,
				definition.slot,
				passive,
				replacedHandle
			))
			{
				ReleaseGrantMutation(definition, reserveSlot);
				if (failureReason && failureReason->empty())
				{
					*failureReason = "Ability collection rejected the replacement preflight.";
				}
				return {};
			}

			std::unique_ptr<Instance> replacedInstance;
			if (!mAbilities.RegisterReplacing(
				handle,
				definition.abilityId,
				definition.slot,
				passive,
				replacedHandle,
				std::move(instance),
				replacedInstance
			))
			{
				ReleaseGrantMutation(definition, reserveSlot);
				if (failureReason && failureReason->empty())
				{
					*failureReason = "Ability collection rejected the registration.";
				}
				return {};
			}

			ReserveReplacementMutationHandle(handle);
			// Collection state commits before callbacks run. The replaced instance
			// stays alive for Cancelled cleanup, then notifications preserve the
			// order: cancel, removed, changed(old), granted, changed(new).
			NotifyReplacedAbility(replacedHandle, replacedInstance);
			if (mCallbacks.granted) mCallbacks.granted(handle);
			if (mCallbacks.changed) mCallbacks.changed(handle);
			ReleaseReplacementMutationHandle(handle);
			ReleaseGrantMutation(definition, reserveSlot);
			return handle;
		}

	public:

		bool RemoveAbility(
			AbilityHandle handle,
			AbilityEndReason reason = AbilityEndReason::Cancelled
		)
		{
			if (mClearing || (mCallbacks.canMutate && !mCallbacks.canMutate())) return false;
			return ExecuteMutation([&] { return RemoveAbilityImpl(handle, reason); });
		}

	private:
		bool RemoveAbilityImpl(AbilityHandle handle, AbilityEndReason reason)
		{
			Instance* ability = mAbilities.Find(handle);
			if (!ability || IsReplacementMutationHandleBlocked(handle) ||
				std::find(
					mRemovalInProgress.begin(),
					mRemovalInProgress.end(),
					handle
				) != mRemovalInProgress.end())
			{
				return false;
			}
			mRemovalInProgress.push_back(handle);
			struct RemovalScope
			{
				std::vector<AbilityHandle>& removals;
				~RemovalScope() { removals.pop_back(); }
			} removalScope{ mRemovalInProgress };
			if (mCallbacks.cancel)
			{
				mCallbacks.cancel(*ability, reason);
			}
			const bool removed = mAbilities.Remove(handle);
			if (!removed)
			{
				return false;
			}
			if (mCallbacks.removed) mCallbacks.removed(handle);
			if (mCallbacks.changed) mCallbacks.changed(handle);
			return true;
		}

	public:
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
			VisitAbility(mAbilities.FindHandle(slot), [&](Instance& ability)
			{
				if (mCallbacks.setInput) mCallbacks.setInput(ability, inputHeld);
			});
		}

		template<typename Visitor>
		bool VisitAbility(AbilityHandle handle, Visitor&& visitor)
		{
			if (mClearing || mClearRequested) return false;
			return ExecuteMutation([&]
			{
				Instance* ability = mAbilities.Find(handle);
				if (!ability) return false;
				ReserveGrantMutation(ability->GetDefinition(), !IsPassiveAbility(ability->GetDefinition()));
				ReserveReplacementMutationHandle(handle);
				std::invoke(visitor, *ability);
				// MutationScope releases both reservations even if the visitor throws.
				return true;
			});
		}

		void Tick(float deltaTime)
		{
			if (mTicking || !mCallbacks.tick)
			{
				return;
			}
			mTicking = true;
			struct TickScope { bool& ticking; ~TickScope() { ticking = false; } } scope{ mTicking };
			for (const AbilityHandle handle : GetHandles())
			{
				VisitAbility(handle, [&](Instance& ability) { mCallbacks.tick(ability, deltaTime); });
			}
		}

		void Clear()
		{
			if (mClearing) return;
			if (mMutationDepth != 0)
			{
				mClearRequested = true;
				return;
			}
			mClearRequested = false;
			mClearing = true;
			struct ClearingScope { bool& flag; ~ClearingScope() { flag = false; } } scope{ mClearing };
			std::exception_ptr error;
			const std::vector<AbilityHandle> handles = GetHandles();
			if (mCallbacks.cancel)
			{
				for (const AbilityHandle handle : handles)
				{
					if (Instance* ability = mAbilities.Find(handle))
					{
						try { mCallbacks.cancel(*ability, AbilityEndReason::OwnerDestroyed); }
						catch (...) { if (!error) error = std::current_exception(); }
					}
				}
			}
			for (const AbilityHandle handle : handles)
			{
				Instance* ability = mAbilities.Find(handle);
				if (!ability)
				{
					continue;
				}

				bool pendingCleanup = static_cast<bool>(error);
				if (mCallbacks.hasPendingCleanup)
				{
					try { pendingCleanup = mCallbacks.hasPendingCleanup(*ability); }
					catch (...) { pendingCleanup = true; if (!error) error = std::current_exception(); }
				}
				if (!pendingCleanup)
				{
					mAbilities.Remove(handle);
				}
			}
			if (!mAbilities.GetAll().empty())
			{
				if (!error)
				{
					error = std::make_exception_ptr(std::runtime_error(
						"Ability cleanup is incomplete; retry Clear after retained cleanup succeeds."
					));
				}
			}
			else
			{
				mAbilities.Clear();
				try { if (mCallbacks.cleared) mCallbacks.cleared(); }
				catch (...) { if (!error) error = std::current_exception(); }
			}
			if (error) std::rethrow_exception(error);
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

	private:
		// Basic exception guarantee: a committed collection remains committed.
		// Reservations always unwind; teardown requested by callbacks runs only
		// after the outermost mutation releases its live instance references.
		struct MutationScope
		{
			AbilityRuntimeSystem& runtime;
			std::size_t slots, ids, handles, removals;
			explicit MutationScope(AbilityRuntimeSystem& owner)
				: runtime(owner), slots(owner.mGrantMutationSlots.size()), ids(owner.mGrantMutationAbilityIds.size()),
				handles(owner.mReplacementMutationHandles.size()), removals(owner.mRemovalInProgress.size())
			{
				++runtime.mMutationDepth;
			}
			~MutationScope()
			{
				if (runtime.mGrantMutationSlots.size() > slots) runtime.mGrantMutationSlots.resize(slots);
				if (runtime.mGrantMutationAbilityIds.size() > ids) runtime.mGrantMutationAbilityIds.resize(ids);
				if (runtime.mReplacementMutationHandles.size() > handles) runtime.mReplacementMutationHandles.resize(handles);
				if (runtime.mRemovalInProgress.size() > removals) runtime.mRemovalInProgress.resize(removals);
				--runtime.mMutationDepth;
			}
		};

		template<typename Operation>
		auto ExecuteMutation(Operation&& operation) -> decltype(operation())
		{
			std::optional<decltype(operation())> result;
			std::exception_ptr error;
			{
				MutationScope scope{ *this };
				try { result.emplace(operation()); }
				catch (...) { error = std::current_exception(); }
			}
			if (mMutationDepth == 0 && mClearRequested)
			{
				try { Clear(); }
				catch (...) { if (!error) error = std::current_exception(); }
			}
			if (error) std::rethrow_exception(error);
			return *result;
		}

		bool IsSlotMutationBlocked(AbilitySlot slot) const
		{
			return std::find(
				mGrantMutationSlots.begin(),
				mGrantMutationSlots.end(),
				slot
			) != mGrantMutationSlots.end();
		}

		bool IsReplacementMutationHandleBlocked(AbilityHandle handle) const
		{
			return std::find(
				mReplacementMutationHandles.begin(),
				mReplacementMutationHandles.end(),
				handle
			) != mReplacementMutationHandles.end();
		}

		void ReserveReplacementMutationHandle(AbilityHandle handle)
		{
			if (handle.IsValid()) mReplacementMutationHandles.push_back(handle);
		}

		void ReleaseReplacementMutationHandle(AbilityHandle handle)
		{
			const auto found = std::find(
				mReplacementMutationHandles.begin(),
				mReplacementMutationHandles.end(),
				handle
			);
			if (found != mReplacementMutationHandles.end())
			{
				mReplacementMutationHandles.erase(found);
			}
		}

		void ReleaseGrantMutationSlot(AbilitySlot slot)
		{
			const auto found = std::find(
				mGrantMutationSlots.begin(),
				mGrantMutationSlots.end(),
				slot
			);
			if (found != mGrantMutationSlots.end())
			{
				mGrantMutationSlots.erase(found);
			}
		}

		void NotifyReplacedAbility(
			AbilityHandle handle,
			std::unique_ptr<Instance>& instance
		)
		{
			if (!handle.IsValid() || !instance) return;
			// The collection commit precedes callbacks. The old instance remains
			// alive for cancellation, then removed and changed fire in that order.
			if (mCallbacks.cancel)
			{
				mCallbacks.cancel(*instance, AbilityEndReason::Cancelled);
			}
			if (mCallbacks.removed) mCallbacks.removed(handle);
			if (mCallbacks.changed) mCallbacks.changed(handle);
			instance.reset();
		}

		std::size_t mMaxPassiveAbilities = 0;
		Callbacks mCallbacks;
		Collection mAbilities;
		std::vector<AbilityHandle> mRemovalInProgress;
		std::vector<AbilitySlot> mGrantMutationSlots;
		std::vector<std::string> mGrantMutationAbilityIds;
		std::vector<AbilityHandle> mReplacementMutationHandles;
		std::size_t mMutationDepth = 0;
		bool mClearRequested = false;
		bool mClearing = false;
		bool mTicking = false;
	};
}
