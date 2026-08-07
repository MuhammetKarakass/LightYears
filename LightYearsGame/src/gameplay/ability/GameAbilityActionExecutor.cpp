#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/GameAbilityActionExecutor.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actions/FireWeaponActionRuntime.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "AbilitySystemComponent.h"
#include "gameplay/combat/Combatant.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameConfigs/combat/EffectConfig.h"
#include <cmath>
#include <optional>

namespace ly
{
	namespace
	{
		sas::GameplayAttributeList BuildAbilityActorAttributes(const AbilityActorDefinition& actorDefinition)
		{
			sas::GameplayAttributeList attributes = actorDefinition.attributes;
			if (actorDefinition.lifeTime > 0.f &&
				!sas::HasGameplayAttribute(attributes, CommonAttributeIds::Duration))
			{
				attributes.push_back(sas::GameplayAttribute{ CommonAttributeIds::Duration, actorDefinition.lifeTime, 0.f });
			}
			return attributes;
		}

		sf::Vector2f ResolveActionDirection(
			const Actor& owner,
			sas::AbilityDirectionPolicy directionPolicy
		)
		{
			switch (directionPolicy)
			{
			case sas::AbilityDirectionPolicy::OwnerVelocity:
			{
				sf::Vector2f velocityDirection = owner.GetVelocity();
				if (GetVectorLength(velocityDirection) > 0.f)
				{
					NormalizeVector(velocityDirection);
					return velocityDirection;
				}
				break;
			}
			case sas::AbilityDirectionPolicy::MouseWorld:
				if (const World* world = owner.GetWorld(); world && world->GetApplication())
				{
					sf::Vector2f mouseDirection = world->GetMouseWorldPosition() - owner.GetActorLocation();
					if (GetVectorLength(mouseDirection) > 0.f)
					{
						NormalizeVector(mouseDirection);
						return mouseDirection;
					}
				}
				break;
			case sas::AbilityDirectionPolicy::OwnerForward:
			default:
				break;
			}

			return owner.GetActorForwardDirection();
		}

		float ResolveActionRotation(
			const sf::Vector2f& direction,
			float fallbackRotation)
		{
			if (GetVectorLength(direction) <= 0.001f)
			{
				return fallbackRotation;
			}

			constexpr float DegreesPerRadian = 57.2957795131f;
			return std::atan2(direction.y, direction.x) * DegreesPerRadian + 90.f;
		}

		sf::Vector2f ResolveAbilityActorSpawnLocation(
			const Actor& owner,
			const SpawnActorAction& actionData,
			const AbilityActorDefinition& actorDefinition,
			const AbilityExecutionContext& context,
			const sf::Vector2f& direction
		)
		{
			switch (actionData.spawnPolicy)
			{
			case sas::AbilitySpawnPolicy::AtEventTarget:
				if (context.event)
				{
					if (const Actor* target =
						context.event->GetTarget<Actor>())
					{
						return target->GetActorLocation();
					}
				}
				break;
			case sas::AbilitySpawnPolicy::MouseWorld:
				if (const World* world = owner.GetWorld())
				{
					return world->GetMouseWorldPosition();
				}
				break;
			case sas::AbilitySpawnPolicy::OwnerForward:
				return owner.GetActorLocation() + direction * actorDefinition.spawnDistance;
			case sas::AbilitySpawnPolicy::AtOwner:
			default:
				break;
			}

			return owner.GetActorLocation();
		}

		std::optional<sf::Vector2f> ResolveAbilityActorTargetLocation(
			const Actor& owner,
			const SpawnActorAction& actionData,
			const AbilityExecutionContext& context
		)
		{
			if (actionData.directionPolicy == sas::AbilityDirectionPolicy::MouseWorld
				|| actionData.spawnPolicy == sas::AbilitySpawnPolicy::MouseWorld)
			{
				if (const World* world = owner.GetWorld(); world && world->GetApplication())
				{
					return world->GetMouseWorldPosition();
				}
			}

			if (actionData.spawnPolicy == sas::AbilitySpawnPolicy::AtEventTarget
				&& context.event)
			{
				if (const Actor* target =
					context.event->GetTarget<Actor>())
				{
					return target->GetActorLocation();
				}
			}

			return std::nullopt;
		}

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
				if (const sas::GameplayEffectDefinition* effectDefinition = EffectData::FindGameplayEffectDefinition(actionData.effectId))
				{
					const GameAbilityDefinition* abilityDefinition = context.definition;
					const sas::GameplayEffectSpec effectSpec = abilityDefinition
						? BuildEffectiveEffectSpec(
							context,
							*effectDefinition,
							AbilityActionAttributeResolver::BuildBaseDamageTags(abilityDefinition)
						)
						: sas::MakeGameplayEffectSpec(*effectDefinition);
					if (sas::AbilitySystemComponent* targetAbilitySystem =
						ResolveEffectTarget(
						*context.abilitySystem,
						actionData.targetPolicy,
						context
					))
					{
						targetAbilitySystem->ApplyGameplayEffect(
							effectSpec,
							&owner
						);
					}
				}
			}
			else if constexpr (std::is_same_v<T, FireWeaponAction>)
			{
				FireWeaponActionRuntime::Execute(action, context);
			}
			else if constexpr (std::is_same_v<T, ApplyImpulseAction>)
			{
				owner.SetVelocity(owner.GetVelocity() + ResolveActionDirection(owner, actionData.directionPolicy) * actionData.magnitude);
			}
			else if constexpr (std::is_same_v<T, EmitGameplayEventAction>)
			{
				sas::AbilityEvent event;
				event.eventTag = actionData.eventTag;
				event.SetSource(&owner);
				event.SetTarget(&owner);
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

				const sas::GameplayAttributeList attributes = BuildAbilityActorAttributes(*actorDefinition);
				const sas::GameplayAttributeList values = AbilityActionAttributeResolver::ResolveAttributes(
					context,
					nullptr,
					attributes,
					AbilityActionAttributeResolver::ResolveDamageTags(
						context,
						AttachmentHostKind::Ability
					)
				);
				const sf::Vector2f direction =
					ResolveActionDirection(owner, actionData.directionPolicy);
				const sf::Vector2f spawnLocation = ResolveAbilityActorSpawnLocation(
					owner,
					actionData,
					*actorDefinition,
					context,
					direction
				);
				weak_ptr<AbilityWorldActor> spawnedActor = AbilityActorRegistry::Spawn(
					AbilityActorSpawnContext{
						owner,
						*actorDefinition,
						values,
						ResolveAbilityActorTargetLocation(owner, actionData, context)
					}
				);

				if (auto actor = spawnedActor.lock())
				{
					const float duration = sas::FindGameplayAttributeValue(values, CommonAttributeIds::Duration, actorDefinition->lifeTime);
					const float damage = sas::FindGameplayAttributeValue(values, CommonAttributeIds::Damage, 0.f);
					const float collisionRadius = sas::FindGameplayAttributeValue(
						values,
						CollisionAttributeIds::Radius,
						sas::FindGameplayAttributeValue(values, CommonAttributeIds::Radius, 0.f)
					);

					actor->SetActorLocation(spawnLocation);
					actor->SetActorRotation(ResolveActionRotation(direction, owner.GetActorRotation()));
					actor->SetLifeTime(duration);
					actor->SetDamage(damage);
					actor->SetDamageTags(AbilityActionAttributeResolver::ResolveDamageTags(
						context,
						AttachmentHostKind::Ability
					));
					actor->SetAbilityUpgradeIds(context.definition->unlockedUpgradeIds);
					actor->SetAbilityCollisionRadius(collisionRadius);
					actor->ConfigureCollisionFromOwner();
					actor->ConfigureFromAttributes(values);
				}
			}
		}, action.spec->action);
	}
}
