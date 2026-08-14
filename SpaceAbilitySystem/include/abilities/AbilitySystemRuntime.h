#pragma once

#include "abilities/AbilityCooldownTracker.h"
#include "abilities/AbilityRuntimeSystem.h"
#include "abilities/AbilityTrigger.h"

#include <cstddef>
#include <functional>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>

namespace sas
{
	class AbilitySystemRuntimeBase
	{
	public:
		virtual ~AbilitySystemRuntimeBase() = default;
		virtual const std::type_info& GetDefinitionType() const = 0;
		virtual const std::type_info& GetInstanceType() const = 0;
		virtual AbilityHandle GrantAbilityUntyped(
			const void* definition,
			std::string* failureReason
		) = 0;
		virtual AbilityHandle GrantAbilityUntyped(
			const void* definition,
			AbilityRuntimeBinding binding,
			std::string* failureReason
		) = 0;
		virtual bool RebindAbility(
			AbilityHandle handle,
			AbilityRuntimeBinding binding,
			std::string* failureReason
		) = 0;
		virtual bool RemoveAbility(
			AbilityHandle handle,
			AbilityEndReason reason
		) = 0;
		virtual void ClearSlot(AbilitySlot slot) = 0;
		virtual void SetSlotInput(AbilitySlot slot, bool inputHeld) = 0;
		virtual bool SetAbilityLevel(AbilityHandle handle, int level) = 0;
		virtual bool SetAbilityLevel(AbilitySlot slot, int level) = 0;
		virtual bool LevelUpAbility(AbilityHandle handle) = 0;
		virtual bool LevelUpAbility(AbilitySlot slot) = 0;
		virtual void ReduceCooldowns(
			float amount,
			bool includePrimaryFire
		) = 0;
		virtual void* FindAbility(AbilityHandle handle) = 0;
		virtual const void* FindAbility(AbilityHandle handle) const = 0;
		virtual void* FindAbility(AbilitySlot slot) = 0;
		virtual const void* FindAbility(AbilitySlot slot) const = 0;
		virtual void* FindAbilityById(const std::string& abilityId) = 0;
		virtual const void* FindAbilityById(
			const std::string& abilityId
		) const = 0;
		virtual ly::List<AbilityRuntimeSnapshot> BuildSnapshots() const = 0;
		virtual const ly::List<AbilityHandle>& GetPassiveAbilities() const = 0;
		virtual void Tick(float deltaTime) = 0;
		virtual void Clear() = 0;
	};

	template <typename Definition, typename Instance>
	class AbilitySystemRuntime final : public AbilitySystemRuntimeBase
	{
	public:
		using Runtime = AbilityRuntimeSystem<Definition, Instance>;
		using Callbacks = typename Runtime::Callbacks;

		explicit AbilitySystemRuntime(std::size_t maxPassiveAbilities)
			: mRuntime{ maxPassiveAbilities }
		{
		}

		void SetCallbacks(Callbacks callbacks)
		{
			mRuntime.SetCallbacks(std::move(callbacks));
		}

		const std::type_info& GetDefinitionType() const override
		{
			return typeid(Definition);
		}

		const std::type_info& GetInstanceType() const override
		{
			return typeid(Instance);
		}

		AbilityHandle GrantAbilityUntyped(
			const void* definition,
			std::string* failureReason
		) override
		{
			return definition
				? mRuntime.GrantAbility(
					*static_cast<const Definition*>(definition),
					failureReason
				)
				: AbilityHandle{};
		}

		AbilityHandle GrantAbilityUntyped(
			const void* definition,
			AbilityRuntimeBinding binding,
			std::string* failureReason
		) override
		{
			return definition
				? mRuntime.GrantAbility(
					*static_cast<const Definition*>(definition),
					binding,
					failureReason
				)
				: AbilityHandle{};
		}

		bool RebindAbility(
			AbilityHandle handle,
			AbilityRuntimeBinding binding,
			std::string* failureReason
		) override
		{
			return mRuntime.RebindAbility(handle, binding, failureReason);
		}

		bool RemoveAbility(
			AbilityHandle handle,
			AbilityEndReason reason
		) override
		{
			return mRuntime.RemoveAbility(handle, reason);
		}

		void ClearSlot(AbilitySlot slot) override
		{
			mRuntime.ClearSlot(slot);
		}

		void SetSlotInput(AbilitySlot slot, bool inputHeld) override
		{
			mRuntime.SetSlotInput(slot, inputHeld);
		}

		bool SetAbilityLevel(AbilityHandle handle, int level) override
		{
			Instance* ability = mRuntime.Find(handle);
			return ability && ability->SetLevel(level);
		}

		bool SetAbilityLevel(AbilitySlot slot, int level) override
		{
			Instance* ability = mRuntime.Find(slot);
			return ability && ability->SetLevel(level);
		}

		bool LevelUpAbility(AbilityHandle handle) override
		{
			Instance* ability = mRuntime.Find(handle);
			return ability && ability->SetLevel(ability->GetLevel() + 1);
		}

		bool LevelUpAbility(AbilitySlot slot) override
		{
			Instance* ability = mRuntime.Find(slot);
			return ability && ability->SetLevel(ability->GetLevel() + 1);
		}

		void ReduceCooldowns(
			float amount,
			bool includePrimaryFire
		) override
		{
			if (amount <= 0.f)
			{
				return;
			}
			for (const AbilityHandle handle : mRuntime.GetHandles())
			{
				Instance* ability = mRuntime.Find(handle);
				if (!ability ||
					(!includePrimaryFire &&
						ability->GetDefinition().slot ==
							AbilitySlot::PrimaryFire))
				{
					continue;
				}
				ability->ReduceCooldownRemaining(amount);
			}
		}

		void* FindAbility(AbilityHandle handle) override
		{
			return mRuntime.Find(handle);
		}

		const void* FindAbility(AbilityHandle handle) const override
		{
			return mRuntime.Find(handle);
		}

		void* FindAbility(AbilitySlot slot) override
		{
			return mRuntime.Find(slot);
		}

		const void* FindAbility(AbilitySlot slot) const override
		{
			return mRuntime.Find(slot);
		}

		void* FindAbilityById(const std::string& abilityId) override
		{
			return mRuntime.FindById(abilityId);
		}

		const void* FindAbilityById(
			const std::string& abilityId
		) const override
		{
			return mRuntime.FindById(abilityId);
		}

		ly::List<AbilityRuntimeSnapshot> BuildSnapshots() const override
		{
			return mRuntime.BuildSnapshots();
		}

		const ly::List<AbilityHandle>& GetPassiveAbilities() const override
		{
			return mRuntime.GetPassiveAbilities();
		}

		template <
			typename Event,
			typename InstanceEventHandler,
			typename TriggerExecutor
		>
		void HandleGameplayEvent(
			const Event& event,
			const ly::GameplayTagContainer& ownedTags,
			InstanceEventHandler&& handleInstanceEvent,
			TriggerExecutor&& executeTrigger
		)
		{
			for (const AbilityHandle handle : mRuntime.GetHandles())
			{
				Instance* ability = mRuntime.Find(handle);
				if (!ability)
				{
					continue;
				}

				// Event callbacks may remove or replace the current instance. Keep
				// the definition snapshot and re-resolve the handle after callbacks
				// instead of holding a reference into the live collection.
				const Definition definition = ability->GetDefinition();
				if (!ownedTags.HasAll(definition.requiredOwnerTags) ||
					ownedTags.HasAny(definition.blockedOwnerTags))
				{
					continue;
				}

				std::invoke(handleInstanceEvent, *ability, event);
				if (!mRuntime.Find(handle))
				{
					continue;
				}
				for (std::size_t triggerIndex = 0;
					triggerIndex < definition.triggers.size();
					++triggerIndex)
				{
					const auto& trigger = definition.triggers[triggerIndex];
					if (!MatchesAbilityTrigger(event, trigger, ownedTags))
					{
						continue;
					}

					const std::string cooldownKey =
						MakeAbilityTriggerCooldownKey(
							definition.abilityId,
							trigger,
							triggerIndex
						);
					if (mTriggerCooldowns.IsActive(cooldownKey))
					{
						continue;
					}
					const int matchLimit = trigger.consumeOnMatch
						? 1
						: trigger.maxMatches;
					const auto matchCount = mTriggerMatchCounts.find(cooldownKey);
					if (matchLimit > 0 &&
						matchCount != mTriggerMatchCounts.end() &&
						matchCount->second >= matchLimit)
					{
						continue;
					}

					std::invoke(
						executeTrigger,
						*ability,
						definition,
						trigger,
						event
					);
					if (matchLimit > 0)
					{
						++mTriggerMatchCounts[cooldownKey];
					}
					if (trigger.internalCooldown > 0.f)
					{
						mTriggerCooldowns.Start(
							cooldownKey,
							trigger.internalCooldown
						);
					}
				}
			}
		}

		void Tick(float deltaTime) override
		{
			mTriggerCooldowns.Tick(deltaTime);
			mRuntime.Tick(deltaTime);
		}

		void Clear() override
		{
			mRuntime.Clear();
			mTriggerCooldowns.Clear();
			mTriggerMatchCounts.clear();
		}

	private:
		Runtime mRuntime;
		AbilityCooldownTracker mTriggerCooldowns;
		std::unordered_map<std::string, int> mTriggerMatchCounts;
	};
}
