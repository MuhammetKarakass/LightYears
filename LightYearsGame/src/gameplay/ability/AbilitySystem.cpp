#include "gameplay/ability/AbilitySystem.h"
#include "gameplay/ability/AbilityBehaviorRegistry.h"
#include "gameplay/ability/AbilityExecutor.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
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

				const PrimaryWeaponValidationResult validation = PrimaryWeaponExecutionSystem::ValidateDefinition(
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

		bool ValidateActions(const List<AbilityActionSpec>& actions, std::string* failureReason)
		{
			return ValidateWeaponActions(actions, failureReason) &&
				ValidateActorActions(actions, failureReason);
		}

		bool ValidateLevelProgression(const AbilityDefinition& definition, std::string* failureReason)
		{
			if (!definition.levelUpgradeScrapCosts.empty())
			{
				if (definition.levelUpgradeScrapCosts.size() != definition.levelProgression.size())
				{
					if (failureReason)
					{
						*failureReason = "Ability upgrade scrap costs must match the number of level steps.";
					}
					return false;
				}
				for (const unsigned int cost : definition.levelUpgradeScrapCosts)
				{
					if (cost == 0)
					{
						if (failureReason)
						{
							*failureReason = "Ability upgrade scrap costs must be greater than zero.";
						}
						return false;
					}
				}
			}

			List<GameplayTag> declaredUpgradeIds;
			for (const GameplayTag& upgradeId : definition.unlockedUpgradeIds)
			{
				const bool alreadyDeclared = std::find(
					declaredUpgradeIds.begin(),
					declaredUpgradeIds.end(),
					upgradeId
				) != declaredUpgradeIds.end();
				if (!upgradeId.IsValid() || alreadyDeclared)
				{
					if (failureReason)
					{
						*failureReason = "Ability upgrade IDs must be valid and unique.";
					}
					return false;
				}
				declaredUpgradeIds.push_back(upgradeId);
			}

			for (const AbilityLevelStep& step : definition.levelProgression)
			{
				for (const AttributeModifier& modifier : step.attributeModifiers)
				{
					if (!modifier.attributeId.IsValid())
					{
						if (failureReason)
						{
							*failureReason = "Ability level modifiers require a valid attribute ID.";
						}
						return false;
					}
				}

				for (const GameplayTag& upgradeId : step.unlockedUpgradeIds)
				{
					const bool alreadyDeclared = std::any_of(
						declaredUpgradeIds.begin(),
						declaredUpgradeIds.end(),
						[&](const GameplayTag& declaredUpgradeId)
						{
							return declaredUpgradeId == upgradeId;
						}
					);
					if (!upgradeId.IsValid() || alreadyDeclared)
					{
						if (failureReason)
						{
							*failureReason = "Ability level upgrade IDs must be valid and unique.";
						}
						return false;
					}
					declaredUpgradeIds.push_back(upgradeId);
				}

				if (!ValidateActions(step.addedActions, failureReason))
				{
					return false;
				}
				for (const AbilityTriggerSpec& trigger : step.addedTriggers)
				{
					if (!trigger.eventTag.IsValid())
					{
						if (failureReason)
						{
							*failureReason = "Ability level triggers require a valid event tag.";
						}
						return false;
					}
					if (!ValidateActions(trigger.actions, failureReason))
					{
						return false;
					}
				}
			}
			return true;
		}
	}

	bool AbilitySystem::ValidateDefinition(const AbilityDefinition& definition, std::string* failureReason)
	{
		if (definition.abilityId.empty())
		{
			if (failureReason)
			{
				*failureReason = "Ability definition requires an ID.";
			}
			return false;
		}
		if (definition.cooldown < 0.f || definition.duration < 0.f || definition.maxCharges < 0)
		{
			if (failureReason)
			{
				*failureReason = "Ability cooldown, duration, and charge count must be non-negative.";
			}
			return false;
		}
		if (definition.lifetimePolicy == AbilityLifetimePolicy::Duration && definition.duration <= 0.f)
		{
			if (failureReason)
			{
				*failureReason = "Duration abilities require a positive duration.";
			}
			return false;
		}

		const bool isPassive = definition.slot == AbilitySlot::None;
		if (!isPassive && definition.slot == AbilitySlot::None)
		{
			if (failureReason)
			{
				*failureReason = "Active abilities require an input slot.";
			}
			return false;
		}
		if (isPassive && definition.activationPolicy == AbilityActivationPolicy::Passive &&
			definition.lifetimePolicy != AbilityLifetimePolicy::UntilCancelled)
		{
			if (failureReason)
			{
				*failureReason = "Passive abilities must use UntilCancelled lifetime.";
			}
			return false;
		}
		if (!ValidateActions(definition.actions, failureReason))
		{
			return false;
		}
		for (const AbilityTriggerSpec& trigger : definition.triggers)
		{
			if (!trigger.eventTag.IsValid() || !ValidateActions(trigger.actions, failureReason))
			{
				if (failureReason && failureReason->empty())
				{
					*failureReason = "Ability triggers require a valid event tag.";
				}
				return false;
			}
		}
		if (!ValidateLevelProgression(definition, failureReason))
		{
			return false;
		}

		unique_ptr<AbilityBehavior> behavior = AbilityBehaviorRegistry::Create(definition.behaviorId);
		if (!behavior)
		{
			if (failureReason)
			{
				*failureReason = "Ability definition references an unregistered behavior.";
			}
			return false;
		}
		if (!behavior->Validate(definition, failureReason))
		{
			if (failureReason && failureReason->empty())
			{
				*failureReason = "Ability behavior rejected its definition.";
			}
			return false;
		}
		return true;
	}

	bool AbilitySystem::ValidateCatalog(
		const List<const AbilityDefinition*>& definitions,
		std::string* failureReason)
	{
		for (size_t index = 0; index < definitions.size(); ++index)
		{
			const AbilityDefinition* definition = definitions[index];
			if (!definition || !ValidateDefinition(*definition, failureReason))
			{
				if (failureReason && failureReason->empty())
				{
					*failureReason = "Shipped ability catalog contains an invalid definition.";
				}
				return false;
			}
			for (size_t previousIndex = 0; previousIndex < index; ++previousIndex)
			{
				if (definitions[previousIndex] && definitions[previousIndex]->abilityId == definition->abilityId)
				{
					if (failureReason)
					{
						*failureReason = "Shipped ability catalog contains duplicate ability IDs.";
					}
					return false;
				}
			}
		}
		return true;
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
		if (!ValidateDefinition(definition, failureReason))
		{
			return {};
		}
		unique_ptr<AbilityBehavior> behavior = AbilityBehaviorRegistry::Create(definition.behaviorId);
		if (!behavior)
		{
			if (failureReason)
			{
				*failureReason = "Ability behavior could not be created.";
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
		if (!isPassive)
		{
			if (const auto bound = mSlotBindings.find(definition.slot); bound != mSlotBindings.end())
			{
				RemoveAbility(bound->second, AbilityEndReason::Cancelled);
			}
		}

		const AbilityHandle handle{ mNextAbilityHandleId++ };
		mAbilities.emplace(
			handle,
			std::make_unique<AbilityInstance>(*this, handle, definition, std::move(behavior))
		);
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

	bool AbilitySystem::TrySetAbilityLevel(AbilityHandle handle, int level)
	{
		AbilityInstance* ability = GetAbility(handle);
		return ability && ability->SetLevel(level);
	}

	bool AbilitySystem::TrySetAbilityLevel(AbilitySlot slot, int level)
	{
		AbilityInstance* ability = GetAbility(slot);
		return ability && ability->SetLevel(level);
	}

	bool AbilitySystem::TryLevelUpAbility(AbilityHandle handle)
	{
		AbilityInstance* ability = GetAbility(handle);
		return ability && ability->SetLevel(ability->GetLevel() + 1);
	}

	bool AbilitySystem::TryLevelUpAbility(AbilitySlot slot)
	{
		AbilityInstance* ability = GetAbility(slot);
		return ability && ability->SetLevel(ability->GetLevel() + 1);
	}

	bool AbilitySystem::TryEquipAttachment(
		AbilityHandle handle,
		const AttachmentDefinition& definition,
		AttachmentHostKind hostKind,
		std::string* failureReason
	)
	{
		AbilityInstance* ability = GetAbility(handle);
		if (!ability)
		{
			if (failureReason)
			{
				*failureReason = "Attachment host ability was not found.";
			}
			return false;
		}
		return ability->TryEquipAttachment(definition, hostKind, failureReason);
	}

	bool AbilitySystem::TryEquipAttachment(
		AbilitySlot slot,
		const AttachmentDefinition& definition,
		AttachmentHostKind hostKind,
		std::string* failureReason
	)
	{
		AbilityInstance* ability = GetAbility(slot);
		if (!ability)
		{
			if (failureReason)
			{
				*failureReason = "Attachment host ability was not found.";
			}
			return false;
		}
		return ability->TryEquipAttachment(definition, hostKind, failureReason);
	}

	bool AbilitySystem::RemoveAttachment(
		AbilityHandle handle,
		const GameplayTag& attachmentId,
		AttachmentHostKind hostKind
	)
	{
		AbilityInstance* ability = GetAbility(handle);
		return ability && ability->RemoveAttachment(attachmentId, hostKind);
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
		onGameplayEvent.Broadcast(event);
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
			ability->HandleAttachmentEvent(event);
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
				AbilityExecutionContext context{ this, &triggerDefinition, &event, ability };
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

	void AbilitySystem::AddOwnerTag(const GameplayTag& tag)
	{
		if (mOwnedTags)
		{
			mOwnedTags->AddTag(tag);
		}
	}

	void AbilitySystem::RemoveOwnerTag(const GameplayTag& tag)
	{
		if (mOwnedTags)
		{
			mOwnedTags->RemoveTag(tag);
		}
	}

	void AbilitySystem::ReduceCooldowns(float amount, bool includePrimaryFire)
	{
		if (amount <= 0.f)
		{
			return;
		}
		for (const auto& entry : mAbilities)
		{
			AbilityInstance* ability = entry.second.get();
			if (!ability || (!includePrimaryFire && ability->GetDefinition().slot == AbilitySlot::PrimaryFire))
			{
				continue;
			}
			ability->ReduceCooldownRemaining(amount);
		}
	}
}


