#include "gameplay/ability/AbilityExecutor.h"
#include "gameplay/ability/AbilitySystem.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/attributes/AttributeSystem.h"
#include "gameplay/effects/GameplayEffectSystem.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/weapon/PrimaryWeaponRegistry.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "gameConfigs/AbilityActorStructs.h"
#include "gameConfigs/EffectStructs.h"
#include "gameConfigs/AbilityConfig.h"
#include <algorithm>

namespace ly
{
	namespace
	{
		float CalculateDefinitionAttributeValue(
			const GameplayAttribute& attribute,
			const AbilityDefinition& abilityDefinition,
			const PrimaryWeaponDefinition* weaponDefinition
		)
		{
			float value = CalculateModifiedAttributeValue(attribute, abilityDefinition.attributeModifiers);
			if (weaponDefinition)
			{
				value = CalculateModifiedAttributeValue(
					GameplayAttribute{ attribute.id, value, attribute.minValue, attribute.maxValue },
					weaponDefinition->attributeModifiers
				);
			}
			return value;
		}

		float ApplyStatScalings(
			float baseValue,
			const GameplayTag& attributeId,
			const List<AttributeScalingRule>& scalingRules,
			AbilitySystem& abilitySystem
		)
		{
			float value = baseValue;
			for (const AttributeScalingRule& scaling : scalingRules)
			{
				if (scaling.targetAttributeId != attributeId)
				{
					continue;
				}

				const float attributeValue = abilitySystem.GetAttributes().GetCurrentValue(scaling.sourceAttributeId);
				switch (scaling.operation)
				{
				case AttributeModifierOperation::Add:
					value += attributeValue * scaling.coefficient;
					break;
				case AttributeModifierOperation::Multiply:
					value *= 1.f + attributeValue * scaling.coefficient;
					break;
				case AttributeModifierOperation::Override:
					value = attributeValue * scaling.coefficient;
					break;
				}
			}
			return std::max(0.f, value);
		}

		float ApplyAllStatScalings(
			float baseValue,
			const GameplayTag& attributeId,
			const AbilityDefinition& abilityDefinition,
			const PrimaryWeaponDefinition* weaponDefinition,
			AbilitySystem& abilitySystem
		)
		{
			float value = ApplyStatScalings(baseValue, attributeId, abilityDefinition.scalingRules, abilitySystem);
			if (weaponDefinition)
			{
				value = ApplyStatScalings(value, attributeId, weaponDefinition->scalingRules, abilitySystem);
			}
			return value;
		}

		float ResolveAttributeValue(
			AbilitySystem& abilitySystem,
			const AbilityDefinition& abilityDefinition,
			const PrimaryWeaponDefinition* weaponDefinition,
			const GameplayAttribute& attribute
		)
		{
			float leveledValue = CalculateDefinitionAttributeValue(attribute, abilityDefinition, weaponDefinition);

			if (attribute.id == CommonAttributeIds::Damage)
			{
				const float attackPower = abilitySystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackPower);
				leveledValue = std::max(attribute.minValue, leveledValue * (1.f + attackPower));
			}

			float scaledValue = ApplyAllStatScalings(
				leveledValue,
				attribute.id,
				abilityDefinition,
				weaponDefinition,
				abilitySystem
			);

			if (attribute.id == CommonAttributeIds::Interval && scaledValue > 0.f)
			{
				const float hasteMultiplier = abilitySystem.GetAttributes().GetSequentialReductionMultiplier(OwnerAttributeIds::AbilityHaste);
				scaledValue *= hasteMultiplier;
			}

			return std::clamp(scaledValue, attribute.minValue, attribute.maxValue);
		}

		GameplayAttributeList ResolveAttributes(
			AbilitySystem& abilitySystem,
			const AbilityDefinition& abilityDefinition,
			const PrimaryWeaponDefinition* weaponDefinition,
			const GameplayAttributeList& attributes
		)
		{
			GameplayAttributeList values;
			for (const GameplayAttribute& attribute : attributes)
			{
				values.push_back(GameplayAttribute{
					attribute.id,
					ResolveAttributeValue(abilitySystem, abilityDefinition, weaponDefinition, attribute),
					attribute.minValue,
					attribute.maxValue
				});
			}

			return values;
		}

		GameplayAttributeList ResolveBaseAttributes(const GameplayAttributeList& attributes)
		{
			GameplayAttributeList values;
			for (const GameplayAttribute& attribute : attributes)
			{
				values.push_back(GameplayAttribute{
					attribute.id,
					std::max(attribute.minValue, attribute.baseValue)
				});
			}
			return values;
		}

		bool HasGameplayAttribute(const GameplayAttributeList& attributes, const GameplayTag& attributeId)
		{
			for (const GameplayAttribute& attribute : attributes)
			{
				if (attribute.id == attributeId)
				{
					return true;
				}
			}
			return false;
		}

		GameplayAttributeList BuildAbilityActorAttributes(const AbilityActorDefinition& actorDefinition)
		{
			GameplayAttributeList attributes = actorDefinition.attributes;
			if (actorDefinition.lifeTime > 0.f && !HasGameplayAttribute(attributes, CommonAttributeIds::Duration))
			{
				attributes.push_back(GameplayAttribute{ CommonAttributeIds::Duration, actorDefinition.lifeTime, 0.f });
			}
			return attributes;
		}

		float BuildEffectiveActionInterval(
			AbilitySystem& abilitySystem,
			const AbilityDefinition& definition,
			float baseInterval,
			const PrimaryWeaponDefinition* weaponDefinition = nullptr
		)
		{
			if (baseInterval <= 0.f)
			{
				return 0.f;
			}

			return ResolveAttributeValue(
				abilitySystem,
				definition,
				weaponDefinition,
				GameplayAttribute{ CommonAttributeIds::Interval, baseInterval, 0.001f }
			);
		}

		sf::Vector2f ResolveActionDirection(
			const Actor& owner,
			AbilityDirectionPolicy directionPolicy
		)
		{
			switch (directionPolicy)
			{
			case AbilityDirectionPolicy::OwnerVelocity:
			{
				sf::Vector2f velocityDirection = owner.GetVelocity();
				if (GetVectorLength(velocityDirection) > 0.f)
				{
					NormalizeVector(velocityDirection);
					return velocityDirection;
				}
				break;
			}
			case AbilityDirectionPolicy::MouseWorld:
				if (const World* world = owner.GetWorld())
				{
					sf::Vector2f mouseDirection = world->GetMouseWorldPosition() - owner.GetActorLocation();
					if (GetVectorLength(mouseDirection) > 0.f)
					{
						NormalizeVector(mouseDirection);
						return mouseDirection;
					}
				}
				break;
			case AbilityDirectionPolicy::OwnerForward:
			default:
				break;
			}

			return owner.GetActorForwardDirection();
		}

		sf::Vector2f ResolveAbilityActorSpawnLocation(
			const Actor& owner,
			const SpawnActorAction& actionData,
			const AbilityActorDefinition& actorDefinition,
			const AbilityExecutionContext& context
		)
		{
			switch (actionData.spawnPolicy)
			{
			case AbilitySpawnPolicy::AtEventTarget:
				if (context.event && context.event->target)
				{
					return context.event->target->GetActorLocation();
				}
				break;
			case AbilitySpawnPolicy::MouseWorld:
				if (const World* world = owner.GetWorld())
				{
					return world->GetMouseWorldPosition();
				}
				break;
			case AbilitySpawnPolicy::OwnerForward:
				return owner.GetActorLocation() + owner.GetActorForwardDirection() * actorDefinition.spawnDistance;
			case AbilitySpawnPolicy::AtOwner:
			default:
				break;
			}

			return owner.GetActorLocation();
		}

		GameplayEffectDefinition BuildEffectiveEffectDefinition(
			AbilitySystem& abilitySystem,
			const AbilityDefinition& abilityDefinition,
			const GameplayEffectDefinition& effectDefinition
		)
		{
			GameplayEffectDefinition effectiveDefinition = effectDefinition;
			effectiveDefinition.attributes = ResolveAttributes(
				abilitySystem,
				abilityDefinition,
				nullptr,
				effectDefinition.attributes
			);
			return effectiveDefinition;
		}

		GameplayEffectSystem* ResolveEffectTarget(
			AbilitySystem& abilitySystem,
			AbilityTargetPolicy targetPolicy,
			const AbilityExecutionContext& context
		)
		{
			if (targetPolicy == AbilityTargetPolicy::Self || targetPolicy == AbilityTargetPolicy::OwnerForward)
			{
				return &abilitySystem.GetEffects();
			}

			Actor* target = nullptr;
			if (context.event)
			{
				target = targetPolicy == AbilityTargetPolicy::EventSource
					? context.event->source
					: context.event->target;
			}
			auto* combatant = dynamic_cast<Combatant*>(target);
			return combatant ? &combatant->GetCombatRuntime().GetEffects() : nullptr;
		}

		const List<GameplayTag> EmptyDamageTags{};

		const GameplayAttributeList& ResolveFireWeaponAttributes(
			AbilityExecutionContext& context,
			const FireWeaponAction& fireAction,
			FireWeaponRuntimeState& state
		)
		{
			const uint64_t attributeRevision = context.abilitySystem
				? context.abilitySystem->GetAttributes().GetRevision()
				: 0;
			if (!state.hasResolvedAttributes || state.resolvedAttributeRevision != attributeRevision)
			{
				state.resolvedAttributes = context.definition
					? ResolveAttributes(*context.abilitySystem, *context.definition, &fireAction.weaponDefinition, state.runtimeAttributes)
					: ResolveBaseAttributes(state.runtimeAttributes);
				state.resolvedAttributeRevision = attributeRevision;
				state.hasResolvedAttributes = true;
			}
			return state.resolvedAttributes;
		}

		PrimaryWeaponExecutionContext MakePrimaryWeaponExecutionContext(
			Actor& owner,
			AbilityExecutionContext& context,
			const FireWeaponAction& fireAction,
			const GameplayAttributeList& attributes
		)
		{
			return PrimaryWeaponExecutionContext{
				owner,
				fireAction.weaponDefinition,
				attributes,
				context.definition ? context.definition->abilityTags : EmptyDamageTags
			};
		}

		bool EnsureFireWeaponLifecycle(
			Actor& owner,
			AbilityExecutionContext& context,
			const FireWeaponAction& fireAction,
			FireWeaponRuntimeState& state,
			const GameplayAttributeList& attributes
		)
		{
			if (!state.initialized)
			{
				state.runtimeAttributes = fireAction.weaponDefinition.attributes;
				state.initialized = true;
			}
			if (state.lifecycleStarted)
			{
				return true;
			}

			const PrimaryWeaponValidationResult validation = PrimaryWeaponRegistry::InitializeRuntime(
				fireAction.weaponDefinition,
				state.weaponRuntime
			);
			if (!validation.isValid)
			{
				return false;
			}
			PrimaryWeaponRegistry::BeginFire(
				MakePrimaryWeaponExecutionContext(owner, context, fireAction, attributes),
				state.weaponRuntime
			);
			state.lifecycleStarted = true;
			return true;
		}
	}

	void AbilityExecutor::BeginExecution(AbilityExecution& execution, AbilityExecutionContext& context)
	{
		execution.actions.clear();
		if (context.definition)
		{
			AddPhaseActions(execution, context.definition->actions, AbilityActionPhase::OnActivate);
			AddPhaseActions(execution, context.definition->actions, AbilityActionPhase::WhileActive);
			for (ActiveAbilityAction& action : execution.actions)
			{
				if (action.spec && action.spec->phase == AbilityActionPhase::OnActivate)
				{
					ExecuteAction(action, context);
				}
			}
		}
	}

	void AbilityExecutor::TickExecution(AbilityExecution& execution, AbilityExecutionContext& context, float deltaTime)
	{
		for (ActiveAbilityAction& action : execution.actions)
		{
			TickAction(action, context, deltaTime);
		}
	}

	void AbilityExecutor::EndExecution(AbilityExecution& execution, AbilityExecutionContext& context, AbilityEndReason reason)
	{
		(void)reason;
		if (context.abilitySystem)
		{
			Actor& owner = context.abilitySystem->GetOwner();
			for (ActiveAbilityAction& action : execution.actions)
			{
				if (!action.spec || !std::holds_alternative<FireWeaponAction>(action.spec->action) ||
					!std::holds_alternative<FireWeaponRuntimeState>(action.runtimeState))
				{
					continue;
				}

				const FireWeaponAction& fireAction = std::get<FireWeaponAction>(action.spec->action);
				FireWeaponRuntimeState& state = std::get<FireWeaponRuntimeState>(action.runtimeState);
				if (!state.lifecycleStarted)
				{
					continue;
				}
				const GameplayAttributeList& values = ResolveFireWeaponAttributes(context, fireAction, state);
				PrimaryWeaponRegistry::EndFire(
					MakePrimaryWeaponExecutionContext(owner, context, fireAction, values),
					state.weaponRuntime
				);
				state.lifecycleStarted = false;
			}
		}
		if (context.definition)
		{
			AbilityExecution endExecution;
			AddPhaseActions(endExecution, context.definition->actions, AbilityActionPhase::OnEnd);
			for (ActiveAbilityAction& action : endExecution.actions)
			{
				ExecuteAction(action, context);
			}
		}
		execution.actions.clear();
	}

	void AbilityExecutor::AddPhaseActions(AbilityExecution& execution, const List<AbilityActionSpec>& actions, AbilityActionPhase phase)
	{
		for (const AbilityActionSpec& spec : actions)
		{
			if (spec.phase == phase)
			{
				execution.actions.push_back(ActiveAbilityAction{ &spec, std::monostate{} });
			}
		}
	}

	void AbilityExecutor::TickAction(ActiveAbilityAction& action, AbilityExecutionContext& context, float deltaTime)
	{
		if (!action.spec || action.spec->phase != AbilityActionPhase::WhileActive)
		{
			return;
		}

		if (std::holds_alternative<FireWeaponAction>(action.spec->action))
		{
			const FireWeaponAction& fireAction = std::get<FireWeaponAction>(action.spec->action);
			if (!std::holds_alternative<FireWeaponRuntimeState>(action.runtimeState))
			{
				FireWeaponRuntimeState state;
				state.runtimeAttributes = fireAction.weaponDefinition.attributes;
				state.intervalRemaining = 0.f;
				state.initialized = true;
				action.runtimeState = std::move(state);
			}

			FireWeaponRuntimeState& state = std::get<FireWeaponRuntimeState>(action.runtimeState);
			const GameplayAttributeList& values = ResolveFireWeaponAttributes(context, fireAction, state);
			Actor& owner = context.abilitySystem->GetOwner();
			if (!EnsureFireWeaponLifecycle(owner, context, fireAction, state, values))
			{
				return;
			}
			PrimaryWeaponExecutionContext weaponContext = MakePrimaryWeaponExecutionContext(owner, context, fireAction, values);
			PrimaryWeaponRegistry::TickFire(weaponContext, state.weaponRuntime, deltaTime);

			if (!PrimaryWeaponRegistry::UsesIntervalFire(state.weaponRuntime))
			{
				return;
			}

			state.intervalRemaining -= deltaTime;
			while (state.intervalRemaining <= 0.f &&
				(action.spec->maxExecutions <= 0 || state.executionCount < action.spec->maxExecutions))
			{
				const bool fired = PrimaryWeaponRegistry::FireOnce(weaponContext, state.weaponRuntime);
				if (fired)
				{
					++state.executionCount;
				}
				const float baseInterval = PrimaryWeaponRegistry::BuildBaseFireInterval(values, action.spec->interval);
				const float interval = context.definition
					? BuildEffectiveActionInterval(*context.abilitySystem, *context.definition, baseInterval, &fireAction.weaponDefinition)
					: baseInterval;
				state.intervalRemaining += interval;
				if (!fired)
				{
					break;
				}
			}
			return;
		}

		auto* repeated = std::get_if<RepeatedActionRuntimeState>(&action.runtimeState);
		if (!repeated)
		{
			action.runtimeState = RepeatedActionRuntimeState{};
			repeated = &std::get<RepeatedActionRuntimeState>(action.runtimeState);
		}

		repeated->intervalAccumulator -= deltaTime;
		if (repeated->intervalAccumulator <= 0.f &&
			(action.spec->maxExecutions <= 0 || repeated->executionCount < action.spec->maxExecutions))
		{
			ExecuteAction(action, context);
			++repeated->executionCount;
			repeated->intervalAccumulator = context.definition
				? BuildEffectiveActionInterval(*context.abilitySystem, *context.definition, action.spec->interval)
				: action.spec->interval;
		}
	}

	void AbilityExecutor::ExecuteAction(ActiveAbilityAction& action, AbilityExecutionContext& context)
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
				if (const GameplayEffectDefinition* effectDefinition = EffectData::FindGameplayEffectDefinition(actionData.effectId))
				{
					const AbilityDefinition* abilityDefinition = context.definition;
					const GameplayEffectDefinition effectiveDefinition = abilityDefinition
						? BuildEffectiveEffectDefinition(*context.abilitySystem, *abilityDefinition, *effectDefinition)
						: *effectDefinition;
					if (GameplayEffectSystem* targetEffects = ResolveEffectTarget(
						*context.abilitySystem,
						actionData.targetPolicy,
						context
					))
					{
						targetEffects->ApplyEffect(effectiveDefinition, &owner);
					}
				}
			}
			else if constexpr (std::is_same_v<T, FireWeaponAction>)
			{
				if (!std::holds_alternative<FireWeaponRuntimeState>(action.runtimeState))
				{
					FireWeaponRuntimeState state;
					state.runtimeAttributes = actionData.weaponDefinition.attributes;
					action.runtimeState = std::move(state);
				}
				FireWeaponRuntimeState& state = std::get<FireWeaponRuntimeState>(action.runtimeState);
				const GameplayAttributeList& values = ResolveFireWeaponAttributes(context, actionData, state);
				if (EnsureFireWeaponLifecycle(owner, context, actionData, state, values))
				{
					PrimaryWeaponRegistry::FireOnce(
						MakePrimaryWeaponExecutionContext(owner, context, actionData, values),
						state.weaponRuntime
					);
				}
			}
			else if constexpr (std::is_same_v<T, ApplyImpulseAction>)
			{
				owner.SetVelocity(owner.GetVelocity() + ResolveActionDirection(owner, actionData.directionPolicy) * actionData.magnitude);
			}
			else if constexpr (std::is_same_v<T, EmitGameplayEventAction>)
			{
				AbilityEvent event;
				event.eventTag = actionData.eventTag;
				event.source = &owner;
				event.target = &owner;
				event.magnitude = actionData.magnitude;
				context.abilitySystem->HandleGameplayEvent(event);
			}
			else if constexpr (std::is_same_v<T, SpawnActorAction>)
			{
				if (!owner.GetWorld() || !context.definition)
				{
					return;
				}

				const AbilityActorDefinition* actorDefinition = AbilityData::FindAbilityActorDefinition(actionData.actorDefinitionId);
				if (!actorDefinition)
				{
					return;
				}

				const GameplayAttributeList attributes = BuildAbilityActorAttributes(*actorDefinition);
				const GameplayAttributeList values = ResolveAttributes(
					*context.abilitySystem,
					*context.definition,
					nullptr,
					attributes
				);
				weak_ptr<AbilityWorldActor> spawnedActor = AbilityActorRegistry::Spawn(
					AbilityActorSpawnContext{ owner, *actorDefinition, values }
				);

				if (auto actor = spawnedActor.lock())
				{
					const float duration = FindGameplayAttributeValue(values, CommonAttributeIds::Duration, actorDefinition->lifeTime);
					const float damage = FindGameplayAttributeValue(values, CommonAttributeIds::Damage, 0.f);
					const float collisionRadius = FindGameplayAttributeValue(
						values,
						CommonAttributeIds::CollisionRadius,
						FindGameplayAttributeValue(values, CommonAttributeIds::Radius, 0.f)
					);

					actor->SetActorLocation(ResolveAbilityActorSpawnLocation(owner, actionData, *actorDefinition, context));
					actor->SetActorRotation(owner.GetActorRotation());
					actor->SetLifeTime(duration);
					actor->SetDamage(damage);
					actor->SetDamageTags(context.definition->abilityTags);
					actor->SetAbilityCollisionRadius(collisionRadius);
					actor->ConfigureCollisionFromOwner();
					actor->ConfigureFromAttributes(values);
				}
			}
		}, action.spec->action);
	}
}
