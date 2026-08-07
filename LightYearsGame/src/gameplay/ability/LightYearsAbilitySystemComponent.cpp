#include "gameplay/ability/LightYearsAbilitySystemComponent.h"

#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/validation/GameAbilityDefinitionValidator.h"
#include "gameplay/ability/validation/GameplayEffectDefinitionValidator.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"

namespace ly
{
	namespace
	{
		bool IsEffectBehaviorRegistered(const GameplayTag& behaviorTag)
		{
			return ::ly::GetEffectBehaviorRuntime().IsRegistered(behaviorTag);
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
					GameAbilityBehaviorRegistry::Create(definition.behaviorTag);
				if (!behavior)
				{
					if (failureReason)
					{
						*failureReason = "Ability behavior could not be created.";
					}
					return {};
				}
				return std::make_unique<GameAbility>(
					*this,
					handle,
					definition,
					std::move(behavior)
				);
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
				GameAbilityDefinition triggerDefinition = definition;
				triggerDefinition.actions = trigger.actions;

				GameAbilityExecution execution;
				AbilityExecutionContext context{
					this,
					&triggerDefinition,
					&abilityEvent,
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
		);
	}
}
