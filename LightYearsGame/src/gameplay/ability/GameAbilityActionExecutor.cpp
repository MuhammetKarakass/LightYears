#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/GameAbilityActionExecutor.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actions/FireWeaponActionRuntime.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/movement/MovementInfluenceService.h"
#include "AbilitySystemComponent.h"
#include "gameplay/combat/Combatant.h"
#include "framework/Actor.h"
#include "gameConfigs/combat/EffectConfig.h"

namespace ly
{
	namespace
	{
		sas::GameplayEffectSpec BuildEffectiveEffectSpec(
			AbilityExecutionContext& context,
			const sas::GameplayEffectDefinition& effectDefinition,
			const List<GameplayTag>& originalDamageTags
		)
		{
			const GameAbilityDefinition& abilityDefinition = *context.definition;
			sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(effectDefinition);
			spec.sourceAbilityUpgradeIds = abilityDefinition.unlockedUpgradeIds;
			const AbilityEffectSpecDefinition* sourceSpec =
				abilityDefinition.FindEffectSpec(effectDefinition.effectId);
			if (sourceSpec)
			{
				if (sourceSpec->useAbilityDuration)
				{
					spec.duration = abilityDefinition.duration;
				}
				else if (sourceSpec->duration.has_value())
				{
					spec.duration = *sourceSpec->duration;
				}
				if (sourceSpec->maxStacks.has_value())
				{
					spec.maxStacks = *sourceSpec->maxStacks;
				}
				spec.modifiers = sourceSpec->modifiers;
				spec.attributes = AbilityActionAttributeResolver::ResolveAttributes(
					context,
					nullptr,
					sourceSpec->attributes,
					originalDamageTags
				);
				return spec;
			}
			spec.attributes = AbilityActionAttributeResolver::ResolveAttributes(
				context,
				nullptr,
				effectDefinition.attributes,
				originalDamageTags
			);
			return spec;
		}

		sas::AbilitySystemComponent* ResolveEffectTarget(
			LightYearsAbilitySystemComponent& abilitySystem,
			sas::AbilityTargetPolicy targetPolicy,
			const AbilityExecutionContext& context
		)
		{
			if (targetPolicy == sas::AbilityTargetPolicy::Self || targetPolicy == sas::AbilityTargetPolicy::OwnerForward)
			{
				return &abilitySystem;
			}

			Actor* target = nullptr;
			if (context.event)
			{
				target = targetPolicy == sas::AbilityTargetPolicy::EventSource
					? context.event->GetSource<Actor>()
					: context.event->GetTarget<Actor>();
			}
			auto* combatant = dynamic_cast<Combatant*>(target);
			return combatant ? &combatant->GetAbilitySystemComponent() : nullptr;
		}

		void ExecuteApplyEffectAction(
			const ApplyEffectAction& actionData,
			AbilityExecutionContext& context,
			Actor& owner
		)
		{
			const sas::GameplayEffectDefinition* effectDefinition =
				EffectData::FindGameplayEffectDefinition(actionData.effectId.ToString());
			if (!effectDefinition)
			{
				return;
			}

			const GameAbilityDefinition* abilityDefinition = context.definition;
			const sas::GameplayEffectSpec effectSpec = abilityDefinition
				? BuildEffectiveEffectSpec(
					context,
					*effectDefinition,
					AbilityActionAttributeResolver::BuildBaseDamageTags(abilityDefinition)
				)
				: sas::MakeGameplayEffectSpec(*effectDefinition);
			sas::AbilitySystemComponent* targetAbilitySystem = ResolveEffectTarget(
				*context.abilitySystem,
				actionData.targetPolicy,
				context
			);
			if (targetAbilitySystem)
			{
				targetAbilitySystem->ApplyGameplayEffect(effectSpec, &owner);
			}
		}

		void ExecuteRemoveEffectsAction(
			const RemoveEffectsAction& actionData,
			AbilityExecutionContext& context
		)
		{
			sas::AbilitySystemComponent* targetAbilitySystem = ResolveEffectTarget(
				*context.abilitySystem,
				actionData.targetPolicy,
				context
			);
			if (!targetAbilitySystem)
			{
				return;
			}

			targetAbilitySystem->RemoveGameplayEffectsIf(
			[&](const sas::ActiveGameplayEffect& activeEffect)
			{
				const sas::GameplayEffectDefinition& definition =
					activeEffect.spec.definition;
				return
					(!actionData.disposition.has_value() ||
						definition.disposition == *actionData.disposition) &&
					(!actionData.cleanseableOnly || definition.cleanseable) &&
					(actionData.category.empty() ||
						definition.category == actionData.category) &&
					(actionData.immunityCategory.empty() ||
						definition.immunityCategory == actionData.immunityCategory);
			}
		);
		}

		void ExecuteApplyImpulseAction(
			const ApplyImpulseAction& actionData,
			Actor& owner
		)
		{
			movement::MovementInfluenceService::ApplyImpulse(
				owner,
				movement::ImpulseRequest{
				AbilityActorSpawner::ResolveDirection(
					owner,
					actionData.directionPolicy
				) * actionData.magnitude
				}
			);
		}

		void ExecuteGameplayEventAction(
			const EmitGameplayEventAction& actionData,
			AbilityExecutionContext& context,
			Actor& owner
		)
		{
			sas::AbilityEvent event;
			event.eventTag = actionData.eventTag;
			event.SetSource(&owner);
			event.SetTarget(&owner);
			event.magnitude = actionData.magnitude;
			event.payloadTags = actionData.payloadTags;
			context.abilitySystem->HandleGameplayEvent(event);
		}

	}

	void GameAbilityActionExecutor::BeginExecution(GameAbilityExecution& execution, AbilityExecutionContext& context)
	{
		if (!context.definition)
		{
			execution.actions.clear();
			return;
		}
		sas::AbilityExecutionLifecycle::Begin(
			execution,
			context.definition->actions,
			[&](ActiveAbilityAction& action)
			{
				ExecuteAction(action, context);
			}
		);
	}

	void GameAbilityActionExecutor::TickExecution(GameAbilityExecution& execution, AbilityExecutionContext& context, float deltaTime)
	{
		sas::AbilityExecutionLifecycle::Tick(
			execution,
			deltaTime,
			[&](ActiveAbilityAction& action, float tickDeltaTime)
			{
				TickAction(action, context, tickDeltaTime);
			}
		);
	}

	void GameAbilityActionExecutor::EndExecution(GameAbilityExecution& execution, AbilityExecutionContext& context, sas::AbilityEndReason reason)
	{
		(void)reason;
		const List<AbilityActionSpec> emptyActions;
		const List<AbilityActionSpec>& actions = context.definition
			? context.definition->actions
			: emptyActions;
		sas::AbilityExecutionLifecycle::End(
			execution,
			actions,
			[&](ActiveAbilityAction& action)
			{
				FireWeaponActionRuntime::End(action, context);
			},
			[&](ActiveAbilityAction& action)
			{
				ExecuteAction(action, context);
			}
		);
	}

	void GameAbilityActionExecutor::TickAction(ActiveAbilityAction& action, AbilityExecutionContext& context, float deltaTime)
	{
		if (!action.spec || action.spec->phase != sas::AbilityActionPhase::WhileActive)
		{
			return;
		}

		if (std::holds_alternative<FireWeaponAction>(action.spec->action))
		{
			FireWeaponActionRuntime::Tick(action, context, deltaTime);
			return;
		}

		auto* repeated = std::get_if<sas::RepeatedAbilityActionState>(&action.runtimeState);
		if (!repeated)
		{
			action.runtimeState = sas::RepeatedAbilityActionState{};
			repeated = &std::get<sas::RepeatedAbilityActionState>(action.runtimeState);
		}

		if (sas::AbilityActionScheduler::IsExecutionDue(
			*repeated,
			deltaTime,
			action.spec->maxExecutions
		))
		{
			ExecuteAction(action, context);
			const float nextInterval = AbilityActionAttributeResolver::ResolveEffectiveInterval(
				context,
				action.spec->interval
			);
			sas::AbilityActionScheduler::RecordExecution(*repeated, nextInterval);
		}
	}

	void GameAbilityActionExecutor::ExecuteAction(ActiveAbilityAction& action, AbilityExecutionContext& context)
	{
		if (!action.spec || !context.abilitySystem)
		{
			return;
		}

		Actor& owner = context.abilitySystem->GetOwner();

		std::visit([&](auto&& actionData)
		{
			using T = std::decay_t<decltype(actionData)>;
			if constexpr (std::is_same_v<T, ApplyEffectAction>)
			{
				ExecuteApplyEffectAction(actionData, context, owner);
			}
			else if constexpr (std::is_same_v<T, RemoveEffectsAction>)
			{
				ExecuteRemoveEffectsAction(actionData, context);
			}
			else if constexpr (std::is_same_v<T, FireWeaponAction>)
			{
				FireWeaponActionRuntime::Execute(action, context);
			}
			else if constexpr (std::is_same_v<T, ApplyImpulseAction>)
			{
				ExecuteApplyImpulseAction(actionData, owner);
			}
			else if constexpr (std::is_same_v<T, EmitGameplayEventAction>)
			{
				ExecuteGameplayEventAction(actionData, context, owner);
			}
			else if constexpr (std::is_same_v<T, SpawnActorAction>)
			{
				AbilityActorSpawner::Spawn(actionData, context, owner);
			}
		}, action.spec->action);
	}
}
