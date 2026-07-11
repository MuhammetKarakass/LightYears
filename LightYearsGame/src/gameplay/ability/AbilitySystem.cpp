#include "gameplay/ability/AbilitySystem.h"
#include "gameplay/ability/AbilityExecutor.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/weapon/PrimaryWeaponRegistry.h"
#include <algorithm>

namespace ly
{
	namespace
	{
		bool ValidateWeaponActions(const List<AbilityActionSpec>& actions, std::string* failureReason)
		{
			for (const AbilityActionSpec& action : actions)
			{
				if (!std::holds_alternative<FireWeaponAction>(action.action))
				{
					continue;
				}

				const PrimaryWeaponValidationResult validation = PrimaryWeaponRegistry::ValidateDefinition(
					std::get<FireWeaponAction>(action.action).weaponDefinition
				);
				if (!validation.isValid)
				{
					if (failureReason)
					{
						*failureReason = validation.reason;
					}
					return false;
				}
			}
			return true;
		}

		bool ValidateActorActions(const List<AbilityActionSpec>& actions, std::string* failureReason)
		{
			for (const AbilityActionSpec& action : actions)
			{
				if (!std::holds_alternative<SpawnActorAction>(action.action))
				{
					continue;
				}

				const SpawnActorAction& spawnAction = std::get<SpawnActorAction>(action.action);
				const AbilityActorDefinition* actorDefinition = AbilityData::FindAbilityActorDefinition(
					spawnAction.actorDefinitionId
				);
				if (!actorDefinition)
				{
					if (failureReason)
					{
						*failureReason = "Spawn actor action references an unknown ability actor definition.";
					}
					return false;
				}

				const AbilityActorValidationResult validation = AbilityActorRegistry::ValidateDefinition(*actorDefinition);
				if (!validation.isValid)
				{
					if (failureReason)
					{
						*failureReason = validation.reason;
					}
					return false;
				}
			}
			return true;
		}
	}

	AbilitySystem::AbilitySystem(
		Actor& owner,
		AttributeSystem& attributes,
		GameplayEffectSystem& effects,
		GameplayTagContainer& ownedTags
	)
		: mOwner{ &owner },
		mAttributes{ &attributes },
		mEffects{ &effects },
		mOwnedTags{ &ownedTags }
	{
	}

	AbilityHandle AbilitySystem::GrantAbility(const AbilityDefinition& definition, std::string* failureReason)
	{
		if (definition.abilityId.empty())
		{
			if (failureReason)
			{
				*failureReason = "Ability definition requires an ID.";
			}
			return {};
		}
		if (const auto found = mAbilityIds.find(definition.abilityId); found != mAbilityIds.end())
		{
			return found->second;
		}

		const bool isPassive = IsPassiveDefinition(definition);
		if (isPassive && mPassiveAbilities.size() >= MaxPassiveAbilities)
		{
			if (failureReason)
			{
				*failureReason = "Passive ability limit reached.";
			}
			return {};
		}
		if (!isPassive && definition.slot == AbilitySlot::None)
		{
			if (failureReason)
			{
				*failureReason = "Active abilities require an input slot.";
			}
			return {};
		}
		if (isPassive && definition.activationPolicy == AbilityActivationPolicy::Passive &&
			definition.lifetimePolicy != AbilityLifetimePolicy::UntilCancelled)
		{
			if (failureReason)
			{
				*failureReason = "Passive abilities must use UntilCancelled lifetime.";
			}
			return {};
		}
		if (!ValidateWeaponActions(definition.actions, failureReason))
		{
			return {};
		}
		if (!ValidateActorActions(definition.actions, failureReason))
		{
			return {};
		}
		for (const AbilityTriggerSpec& trigger : definition.triggers)
		{
			if (!ValidateWeaponActions(trigger.actions, failureReason))
			{
				return {};
			}
			if (!ValidateActorActions(trigger.actions, failureReason))
			{
				return {};
			}
		}
		if (!isPassive)
		{
			if (const auto bound = mSlotBindings.find(definition.slot); bound != mSlotBindings.end())
			{
				RemoveAbility(bound->second, AbilityEndReason::Cancelled);
			}
		}

		const AbilityHandle handle{ mNextAbilityHandleId++ };
		mAbilities.emplace(handle, std::make_unique<AbilityInstance>(*this, handle, definition));
		mAbilityIds.emplace(definition.abilityId, handle);
		if (isPassive)
		{
			mPassiveAbilities.push_back(handle);
		}
		else
		{
			mSlotBindings[definition.slot] = handle;
		}

		onAbilityGranted.Broadcast(handle);
		onAbilityChanged.Broadcast(handle);
		return handle;
	}

	bool AbilitySystem::RemoveAbility(AbilityHandle handle, AbilityEndReason reason)
	{
		auto found = mAbilities.find(handle);
		if (found == mAbilities.end() || !found->second)
		{
			return false;
		}

		const AbilityDefinition& definition = found->second->GetDefinition();
		const std::string abilityId = definition.abilityId;
		const AbilitySlot slot = definition.slot;
		const bool isPassive = IsPassiveDefinition(definition);
		found->second->Cancel(reason);
		mAbilities.erase(found);
		mAbilityIds.erase(abilityId);
		if (isPassive)
		{
			RemovePassiveHandle(handle);
		}
		else
		{
			auto bound = mSlotBindings.find(slot);
			if (bound != mSlotBindings.end() && bound->second == handle)
			{
				mSlotBindings.erase(bound);
			}
		}

		onAbilityRemoved.Broadcast(handle);
		onAbilityChanged.Broadcast(handle);
		return true;
	}

	void AbilitySystem::ClearSlot(AbilitySlot slot)
	{
		if (const auto bound = mSlotBindings.find(slot); bound != mSlotBindings.end())
		{
			RemoveAbility(bound->second, AbilityEndReason::Cancelled);
		}
	}

	void AbilitySystem::SetSlotInput(AbilitySlot slot, bool inputHeld)
	{
		if (AbilityInstance* ability = GetAbility(slot))
		{
			ability->SetInputHeld(inputHeld);
		}
	}

	void AbilitySystem::Tick(float deltaTime)
	{
		UpdateTriggerCooldowns(deltaTime);

		for (auto& entry : mAbilities)
		{
			if (entry.second)
			{
				entry.second->Tick(deltaTime);
			}
		}
	}

	void AbilitySystem::Clear()
	{
		for (auto& entry : mAbilities)
		{
			if (entry.second)
			{
				entry.second->Cancel(AbilityEndReason::OwnerDestroyed);
			}
		}
		mAbilities.clear();
		mSlotBindings.clear();
		mAbilityIds.clear();
		mPassiveAbilities.clear();
		mTriggerCooldowns.clear();
		mNextAbilityHandleId = 1;
		onAbilitiesCleared.Broadcast();
	}

	AbilityInstance* AbilitySystem::GetAbility(AbilitySlot slot)
	{
		auto bound = mSlotBindings.find(slot);
		return bound != mSlotBindings.end() ? GetAbility(bound->second) : nullptr;
	}

	const AbilityInstance* AbilitySystem::GetAbility(AbilitySlot slot) const
	{
		auto bound = mSlotBindings.find(slot);
		return bound != mSlotBindings.end() ? GetAbility(bound->second) : nullptr;
	}

	AbilityInstance* AbilitySystem::GetAbility(AbilityHandle handle)
	{
		auto found = mAbilities.find(handle);
		return found != mAbilities.end() ? found->second.get() : nullptr;
	}

	const AbilityInstance* AbilitySystem::GetAbility(AbilityHandle handle) const
	{
		auto found = mAbilities.find(handle);
		return found != mAbilities.end() ? found->second.get() : nullptr;
	}

	AbilityInstance* AbilitySystem::GetAbilityById(const std::string& abilityId)
	{
		auto found = mAbilityIds.find(abilityId);
		return found != mAbilityIds.end() ? GetAbility(found->second) : nullptr;
	}

	const AbilityInstance* AbilitySystem::GetAbilityById(const std::string& abilityId) const
	{
		auto found = mAbilityIds.find(abilityId);
		return found != mAbilityIds.end() ? GetAbility(found->second) : nullptr;
	}

	List<AbilityRuntimeSnapshot> AbilitySystem::BuildSnapshots() const
	{
		List<AbilityRuntimeSnapshot> snapshots;
		for (const auto& entry : mAbilities)
		{
			if (entry.second)
			{
				snapshots.push_back(entry.second->BuildSnapshot());
			}
		}
		return snapshots;
	}

	void AbilitySystem::HandleGameplayEvent(const AbilityEvent& event)
	{
		for (const auto& entry : mAbilities)
		{
			AbilityInstance* ability = entry.second.get();
			if (!ability)
			{
				continue;
			}

			const AbilityDefinition& definition = ability->GetDefinition();
			if (!HasAllOwnerTags(definition.requiredOwnerTags) || HasAnyOwnerTags(definition.blockedOwnerTags))
			{
				continue;
			}
			for (size_t triggerIndex = 0; triggerIndex < definition.triggers.size(); ++triggerIndex)
			{
				const AbilityTriggerSpec& trigger = definition.triggers[triggerIndex];
				if (!event.eventTag.MatchesTag(trigger.eventTag))
				{
					continue;
				}
				if (!HasAllOwnerTags(trigger.requiredTags) || HasAnyOwnerTags(trigger.blockedTags))
				{
					continue;
				}

				const std::string cooldownKey = MakeTriggerCooldownKey(definition, trigger, triggerIndex);
				auto foundCooldown = mTriggerCooldowns.find(cooldownKey);
				if (foundCooldown != mTriggerCooldowns.end() && foundCooldown->second > 0.f)
				{
					continue;
				}

				AbilityDefinition triggerDefinition = definition;
				triggerDefinition.actions = trigger.actions;

				AbilityExecution execution;
				AbilityExecutionContext context{ this, &triggerDefinition, &event };
				AbilityExecutor::BeginExecution(execution, context);
				AbilityExecutor::TickExecution(execution, context, 0.f);
				AbilityExecutor::EndExecution(execution, context, AbilityEndReason::Completed);

				if (trigger.internalCooldown > 0.f)
				{
					mTriggerCooldowns[cooldownKey] = trigger.internalCooldown;
				}
			}
		}
	}

	bool AbilitySystem::HasAllOwnerTags(const List<GameplayTag>& tags) const
	{
		return mOwnedTags && mOwnedTags->HasAll(tags);
	}

	bool AbilitySystem::HasAnyOwnerTags(const List<GameplayTag>& tags) const
	{
		return mOwnedTags && mOwnedTags->HasAny(tags);
	}

	void AbilitySystem::UpdateTriggerCooldowns(float deltaTime)
	{
		for (auto it = mTriggerCooldowns.begin(); it != mTriggerCooldowns.end();)
		{
			it->second -= deltaTime;
			if (it->second <= 0.f)
			{
				it = mTriggerCooldowns.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	std::string AbilitySystem::MakeTriggerCooldownKey(const AbilityDefinition& definition, const AbilityTriggerSpec& trigger, size_t triggerIndex) const
	{
		return definition.abilityId + "|" + trigger.eventTag.ToString() + "|" + std::to_string(triggerIndex);
	}

	bool AbilitySystem::IsPassiveDefinition(const AbilityDefinition& definition) const
	{
		return definition.slot == AbilitySlot::None;
	}

	void AbilitySystem::RemovePassiveHandle(AbilityHandle handle)
	{
		mPassiveAbilities.erase(
			std::remove(mPassiveAbilities.begin(), mPassiveAbilities.end(), handle),
			mPassiveAbilities.end()
		);
	}

	void AbilitySystem::NotifyAbilityChanged(AbilityHandle handle)
	{
		onAbilityChanged.Broadcast(handle);
	}

	void AbilitySystem::NotifyAbilityActivated(AbilityHandle handle)
	{
		onAbilityActivated.Broadcast(handle);
		onAbilityChanged.Broadcast(handle);
	}

	void AbilitySystem::NotifyAbilityEnded(AbilityHandle handle, AbilityEndReason reason)
	{
		onAbilityEnded.Broadcast(handle, reason);
		onAbilityChanged.Broadcast(handle);
	}

	void AbilitySystem::NotifyAbilityLevelChanged(AbilityHandle handle, int level)
	{
		onAbilityLevelChanged.Broadcast(handle, level);
		onAbilityChanged.Broadcast(handle);
	}
}


