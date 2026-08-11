#include "gameplay/ability/LightYearsAbilitySystemComponent.h"

#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/validation/GameAbilityDefinitionValidator.h"
#include "gameplay/ability/validation/GameplayEffectDefinitionValidator.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "attributes/AttributeMath.h"

#include <algorithm>
#include <utility>

namespace ly
{
	namespace
	{
		bool IsEffectBehaviorRegistered(const sas::GameplayEffectBehaviorKey& behaviorKey)
		{
			return ::ly::GetEffectBehaviorRuntime().IsRegistered(behaviorKey);
		}

		bool ValidateEffectForRuntime(
			const sas::GameplayEffectDefinition& definition,
			std::string* failureReason
		)
		{
			return GameplayEffectDefinitionValidator::Validate(
				definition,
				IsEffectBehaviorRegistered,
				failureReason
			);
		}

		void ExecuteTriggeredActions(
			LightYearsAbilitySystemComponent& abilitySystem,
			GameAbility& ability,
			const GameAbilityDefinition& definition,
			const AbilityTriggerSpec& trigger,
			const sas::AbilityEvent& event
		)
		{
			GameAbilityDefinition triggerDefinition = definition;
			triggerDefinition.actions = trigger.actions;

			GameAbilityExecution execution;
			AbilityExecutionContext context{
				&abilitySystem,
				&triggerDefinition,
				&event,
				&ability
			};
			GameAbilityActionExecutor::BeginExecution(execution, context);
			GameAbilityActionExecutor::TickExecution(execution, context, 0.f);
			GameAbilityActionExecutor::EndExecution(
				execution,
				context,
				sas::AbilityEndReason::Completed
			);
		}
	}

	LightYearsAbilitySystemComponent::LightYearsAbilitySystemComponent(Actor& owner)
		: mOwner{ owner },
		mComponentRuntime{
			InitializeAbilitySystem<GameAbilityDefinition, GameAbility>(MaxPassiveAbilities)
		}
	{
		sas::AbilityRuntimeSystem<GameAbilityDefinition, GameAbility>::Callbacks callbacks;
		callbacks.validate =
			[](const GameAbilityDefinition& definition, std::string* failureReason)
			{
				return GameAbilityDefinitionValidator::Validate(
					definition,
					ValidateEffectForRuntime,
					failureReason
				);
			};
		callbacks.create =
			[this](
				sas::AbilityHandle handle,
				const GameAbilityDefinition& definition,
				std::string* failureReason
			) -> unique_ptr<GameAbility>
			{
				unique_ptr<GameAbilityBehavior> behavior =
					GameAbilityBehaviorRegistry::Create(definition.behaviorType);
				if (!behavior)
				{
					if (failureReason)
					{
						*failureReason = "Ability behavior could not be created.";
					}
					return {};
				}
				unique_ptr<GameAbility> ability = std::make_unique<GameAbility>(
					*this,
					handle,
					definition,
					std::move(behavior)
				);
				// Scoped progression rules may exist before a content ability is
				// granted. Materialize their level effect immediately so future
				// abilities receive the same category-based rule as existing ones.
				ability->RefreshScopedConfiguration();
				return ability;
			};
		callbacks.cancel = [](GameAbility& ability, sas::AbilityEndReason reason)
		{
			ability.Cancel(reason);
		};
		callbacks.tick = [](GameAbility& ability, float deltaTime)
		{
			ability.Tick(deltaTime);
		};
		callbacks.setInput = [](GameAbility& ability, bool inputHeld)
		{
			ability.SetInputHeld(inputHeld);
		};
		callbacks.snapshot = [](const GameAbility& ability)
		{
			return ability.BuildSnapshot();
		};
		ConfigureAbilityRuntime(mComponentRuntime, std::move(callbacks));
		SetGameplayEventHandler<sas::AbilityEvent>(
			[this](const sas::AbilityEvent& event)
			{
				ProcessGameGameplayEvent(event);
			}
		);
	}

	bool LightYearsAbilitySystemComponent::TryEquipAttachment(
		sas::AbilityHandle handle,
		const AttachmentDefinition& definition,
		AttachmentHostKind hostKind,
		std::string* failureReason
	)
	{
		GameAbility* ability = GetAbility(handle);
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

	bool LightYearsAbilitySystemComponent::TryEquipAttachment(
		sas::AbilitySlot slot,
		const AttachmentDefinition& definition,
		AttachmentHostKind hostKind,
		std::string* failureReason
	)
	{
		GameAbility* ability = GetAbility(slot);
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

	bool LightYearsAbilitySystemComponent::RemoveAttachment(
		sas::AbilityHandle handle,
		const std::string& attachmentId,
		AttachmentHostKind hostKind
	)
	{
		GameAbility* ability = GetAbility(handle);
		return ability && ability->RemoveAttachment(attachmentId, hostKind);
	}

	GameAbility* LightYearsAbilitySystemComponent::GetAbility(sas::AbilitySlot slot)
	{
		return FindAbility<GameAbility>(slot);
	}

	const GameAbility* LightYearsAbilitySystemComponent::GetAbility(sas::AbilitySlot slot) const
	{
		return FindAbility<GameAbility>(slot);
	}

	GameAbility* LightYearsAbilitySystemComponent::GetAbility(sas::AbilityHandle handle)
	{
		return FindAbility<GameAbility>(handle);
	}

	const GameAbility* LightYearsAbilitySystemComponent::GetAbility(sas::AbilityHandle handle) const
	{
		return FindAbility<GameAbility>(handle);
	}

	GameAbility* LightYearsAbilitySystemComponent::GetAbilityById(const std::string& abilityId)
	{
		return FindAbilityById<GameAbility>(abilityId);
	}

	const GameAbility* LightYearsAbilitySystemComponent::GetAbilityById(
		const std::string& abilityId
	) const
	{
		return FindAbilityById<GameAbility>(abilityId);
	}

	void LightYearsAbilitySystemComponent::AddOwnedTag(const GameplayTag& tag)
	{
		sas::AbilitySystemComponent::AddOwnedTag(tag);

		if (tag != GameplayTagSchema::BlockPrimaryWeaponFire)
		{
			return;
		}

		// The primary weapon may already be active when another ability grants
		// this shared lock. EndAbility() runs the primary weapon's EndExecution,
		// which clears its isFiring state and prevents another projectile from
		// being emitted on the next frame.
		GameAbility* primaryWeapon = GetAbility(sas::AbilitySlot::PrimaryFire);
		if (primaryWeapon && primaryWeapon->IsActive())
		{
			primaryWeapon->Cancel(sas::AbilityEndReason::Interrupted);
		}
	}

	std::size_t LightYearsAbilitySystemComponent::AddScopedAbilityRule(
		ScopedAbilityRule rule
	)
	{
		const std::size_t handle = mNextScopedAbilityRuleHandle++;
		mScopedAbilityRules.emplace_back(handle, std::move(rule));
		RefreshScopedAbilityRules();
		return handle;
	}

	bool LightYearsAbilitySystemComponent::RemoveScopedAbilityRule(
		std::size_t ruleHandle
	)
	{
		const auto found = std::find_if(
			mScopedAbilityRules.begin(),
			mScopedAbilityRules.end(),
			[&](const auto& entry)
			{
				return entry.first == ruleHandle;
			}
		);
		if (found == mScopedAbilityRules.end())
		{
			return false;
		}
		mScopedAbilityRules.erase(found);
		RefreshScopedAbilityRules();
		return true;
	}

	void LightYearsAbilitySystemComponent::ClearScopedAbilityRules()
	{
		if (mScopedAbilityRules.empty())
		{
			return;
		}
		mScopedAbilityRules.clear();
		RefreshScopedAbilityRules();
	}

	sas::GameplayAttribute LightYearsAbilitySystemComponent::ApplyScopedAbilityModifiers(
		const GameAbilityDefinition& definition,
		const sas::GameplayAttribute& attribute
	) const
	{
		sas::GameplayAttribute result = attribute;
		for (const auto& entry : mScopedAbilityRules)
		{
			const ScopedAbilityRule& rule = entry.second;
			if (!MatchesScopedAbilityRule(rule, definition))
			{
				continue;
			}
			result.currentValue = sas::CalculateModifiedAttributeValue(
				sas::GameplayAttribute{
					result.id,
					result.currentValue,
					result.minValue,
					result.maxValue
				},
				rule.attributeModifiers
			);
		}
		return result;
	}

	int LightYearsAbilitySystemComponent::GetScopedAbilityLevelBonus(
		const GameAbilityDefinition& definition
	) const
	{
		int bonus = 0;
		for (const auto& entry : mScopedAbilityRules)
		{
			const ScopedAbilityRule& rule = entry.second;
			if (MatchesScopedAbilityRule(rule, definition))
			{
				bonus += rule.levelBonus;
			}
		}
		return bonus;
	}

	bool LightYearsAbilitySystemComponent::MatchesScopedAbilityRule(
		const ScopedAbilityRule& rule,
		const GameAbilityDefinition& definition
	) const
	{
		GameplayTagContainer abilityTags;
		for (const GameplayTag& tag : definition.abilityTags)
		{
			abilityTags.AddTag(tag);
		}
		return abilityTags.HasAll(rule.requiredAbilityTags) &&
			!abilityTags.HasAny(rule.blockedAbilityTags);
	}

	void LightYearsAbilitySystemComponent::RefreshScopedAbilityRules()
	{
		for (const sas::AbilityRuntimeSnapshot& snapshot : BuildAbilitySnapshots())
		{
			if (GameAbility* ability = GetAbilityById(snapshot.abilityId))
			{
				ability->RefreshScopedConfiguration();
				NotifyAbilityChanged(ability->GetHandle());
			}
		}
	}

	void LightYearsAbilitySystemComponent::ProcessGameGameplayEvent(
		const sas::AbilityEvent& event
	)
	{
		mComponentRuntime.HandleGameplayEvent(
			event,
			GetOwnedTags(),
			[](GameAbility& ability, const sas::AbilityEvent& abilityEvent)
			{
				ability.HandleAttachmentEvent(abilityEvent);
			},
			[this](
				GameAbility& ability,
				const GameAbilityDefinition& definition,
				const AbilityTriggerSpec& trigger,
				const sas::AbilityEvent& abilityEvent
			)
			{
				ExecuteTriggeredActions(
					*this,
					ability,
					definition,
					trigger,
					abilityEvent
				);
			}
		);
	}

	void LightYearsAbilitySystemComponent::HandleAbilityLifecycleEvent(
		const sas::AbilityLifecycleEvent& event
	)
	{
		onGameplayEvent.Broadcast(event);
		ProcessAbilityLifecycleEvent(event);
	}

	void LightYearsAbilitySystemComponent::ProcessAbilityLifecycleEvent(
		const sas::AbilityLifecycleEvent& event
	)
	{
		mComponentRuntime.HandleGameplayEvent(
			event,
			GetOwnedTags(),
			[](GameAbility& ability, const sas::AbilityLifecycleEvent& abilityEvent)
			{
				ability.HandleAbilityLifecycleEvent(abilityEvent);
			},
			[this](
				GameAbility& ability,
				const GameAbilityDefinition& definition,
				const AbilityTriggerSpec& trigger,
				const sas::AbilityLifecycleEvent& abilityEvent
			)
			{
				ExecuteTriggeredActions(
					*this,
					ability,
					definition,
					trigger,
					abilityEvent
				);
			}
		);
	}
}
