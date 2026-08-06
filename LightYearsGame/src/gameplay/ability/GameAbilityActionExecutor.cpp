#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/GameAbilityActionExecutor.h"
#include "attributes/AttributeMath.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "attributes/AttributeSystem.h"
#include "AbilitySystemComponent.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameConfigs/combat/EffectConfig.h"
#include <algorithm>
#include <cmath>
#include <optional>

namespace ly
{
	namespace
	{
		float CalculateDefinitionAttributeValue(
			const sas::GameplayAttribute& attribute,
			const GameAbilityDefinition& abilityDefinition,
			const PrimaryWeaponDefinition* weaponDefinition,
			const GameAbility* instance
		)
		{
			float value = sas::CalculateModifiedAttributeValue(attribute, abilityDefinition.attributeModifiers);
			if (instance)
			{
				value = instance->ApplyAttachmentModifiers(
					AttachmentHostKind::Ability,
					sas::GameplayAttribute{ attribute.id, value, attribute.minValue, attribute.maxValue }
				).currentValue;
			}
			if (weaponDefinition)
			{
				value = sas::CalculateModifiedAttributeValue(
					sas::GameplayAttribute{ attribute.id, value, attribute.minValue, attribute.maxValue },
					weaponDefinition->attributeModifiers
				);
				if (instance)
				{
					value = instance->ApplyAttachmentModifiers(
						AttachmentHostKind::PrimaryWeapon,
						sas::GameplayAttribute{ attribute.id, value, attribute.minValue, attribute.maxValue }
					).currentValue;
				}
			}
			return value;
		}

		float ApplyAllStatScalings(
			float baseValue,
			const GameplayTag& attributeId,
			const GameAbilityDefinition& abilityDefinition,
			const PrimaryWeaponDefinition* weaponDefinition,
			LightYearsAbilitySystemComponent& abilitySystem
		)
		{
			float value = sas::ApplyAttributeScalings(
				baseValue,
				attributeId,
				abilityDefinition.scalingRules,
				abilitySystem.GetAttributes()
			);
			if (weaponDefinition)
			{
				value = sas::ApplyAttributeScalings(
					value,
					attributeId,
					weaponDefinition->scalingRules,
					abilitySystem.GetAttributes()
				);
			}
			return value;
		}

		float ResolveAttributeValue(
			LightYearsAbilitySystemComponent& abilitySystem,
			const GameAbilityDefinition& abilityDefinition,
			const PrimaryWeaponDefinition* weaponDefinition,
			const sas::GameplayAttribute& attribute,
			const GameAbility* instance = nullptr
		)
		{
			float leveledValue = CalculateDefinitionAttributeValue(
				attribute,
				abilityDefinition,
				weaponDefinition,
				instance
			);

			float scaledValue = ApplyAllStatScalings(
				leveledValue,
				attribute.id,
				abilityDefinition,
				weaponDefinition,
				abilitySystem
			);

			if (attribute.id == CommonAttributeIds::Interval && scaledValue > 0.f && !weaponDefinition)
			{
				const float hasteMultiplier = sas::AttributeMath::GetAbilityCooldownMultiplier(
					abilitySystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::AbilityHaste)
				);
				scaledValue *= hasteMultiplier;
			}

			return std::clamp(scaledValue, attribute.minValue, attribute.maxValue);
		}

		sas::GameplayAttributeList ResolveAttributes(
			LightYearsAbilitySystemComponent& abilitySystem,
			const GameAbilityDefinition& abilityDefinition,
			const PrimaryWeaponDefinition* weaponDefinition,
			const sas::GameplayAttributeList& attributes,
			const GameAbility* instance = nullptr,
			const List<GameplayTag>& originalDamageTags = {}
		)
		{
			sas::GameplayAttributeList sourceAttributes = attributes;
			if (instance)
			{
				sourceAttributes = instance->MergeAttachmentAttributes(
					AttachmentHostKind::Ability,
					sourceAttributes
				);
				if (weaponDefinition)
				{
					sourceAttributes = instance->MergeAttachmentAttributes(
						AttachmentHostKind::PrimaryWeapon,
						sourceAttributes
					);
				}
			}

			sas::GameplayAttributeList values;
			for (const sas::GameplayAttribute& attribute : sourceAttributes)
			{
				values.push_back(sas::GameplayAttribute{
					attribute.id,
					ResolveAttributeValue(abilitySystem, abilityDefinition, weaponDefinition, attribute, instance),
					attribute.minValue,
					attribute.maxValue
				});
			}
			if (instance)
			{
				values = instance->ApplyAttachmentConditions(
					AttachmentHostKind::Ability,
					values,
					originalDamageTags
				);
				if (weaponDefinition)
				{
					values = instance->ApplyAttachmentConditions(
						AttachmentHostKind::PrimaryWeapon,
						values,
						originalDamageTags
					);
				}
			}

			return values;
		}

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

		float BuildEffectiveActionInterval(
			LightYearsAbilitySystemComponent& abilitySystem,
			const GameAbilityDefinition& definition,
			float baseInterval,
			const PrimaryWeaponDefinition* weaponDefinition = nullptr,
			const GameAbility* instance = nullptr
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
				sas::GameplayAttribute{ CommonAttributeIds::Interval, baseInterval, 0.001f },
				instance
			);
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
			LightYearsAbilitySystemComponent& abilitySystem,
			const GameAbilityDefinition& abilityDefinition,
			const sas::GameplayEffectDefinition& effectDefinition,
			const GameAbility* instance,
			const List<GameplayTag>& originalDamageTags
		)
		{
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
				spec.attributes = ResolveAttributes(
					abilitySystem,
					abilityDefinition,
					nullptr,
					sourceSpec->attributes,
					instance,
					originalDamageTags
				);
				return spec;
			}
			spec.attributes = ResolveAttributes(
				abilitySystem,
				abilityDefinition,
				nullptr,
				effectDefinition.attributes,
				instance,
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

		List<GameplayTag> BuildBaseDamageTags(const GameAbilityDefinition* definition)
		{
			if (definition && !definition->damageTags.empty())
			{
				return definition->damageTags;
			}
			return { DamageTypeSchema::Photonic };
		}

		List<GameplayTag> ResolveDamageTags(
			const AbilityExecutionContext& context,
			AttachmentHostKind hostKind
		)
		{
			if (context.instance)
			{
				return context.instance->GetResolvedDamageTags(hostKind);
			}
			return BuildBaseDamageTags(context.definition);
		}

		const sas::GameplayAttributeList& ResolveFireWeaponAttributes(
			AbilityExecutionContext& context,
			const FireWeaponAction& fireAction,
			FireWeaponRuntimeState& state
		)
		{
			const uint64_t attributeRevision = context.abilitySystem
				? context.abilitySystem->GetAttributes().GetRevision()
				: 0;
			const uint64_t attachmentRevision = context.instance
				? context.instance->GetAttachments().GetRevision()
				: 0;
			if (!state.hasResolvedAttributes || state.resolvedAttributeRevision != attributeRevision ||
				state.resolvedAttachmentRevision != attachmentRevision)
			{
				state.resolvedAttributes = context.definition
					? ResolveAttributes(
						*context.abilitySystem,
						*context.definition,
						&fireAction.weaponDefinition,
						state.runtimeAttributes,
						context.instance,
						ResolveDamageTags(context, AttachmentHostKind::PrimaryWeapon)
					)
					: sas::BuildBaseGameplayAttributes(state.runtimeAttributes);
				state.resolvedAttributeRevision = attributeRevision;
				state.resolvedAttachmentRevision = attachmentRevision;
				state.hasResolvedAttributes = true;
			}
			return state.resolvedAttributes;
		}

		PrimaryWeaponExecutionContext MakePrimaryWeaponExecutionContext(
			Actor& owner,
			AbilityExecutionContext& context,
			const FireWeaponAction& fireAction,
			const sas::GameplayAttributeList& attributes
		)
		{
			return PrimaryWeaponExecutionContext{
				owner,
				fireAction.weaponDefinition,
				attributes,
				ResolveDamageTags(context, AttachmentHostKind::PrimaryWeapon),
				context.definition ? &context.definition->unlockedUpgradeIds : nullptr
			};
		}

		bool EnsureFireWeaponLifecycle(
			Actor& owner,
			AbilityExecutionContext& context,
			const FireWeaponAction& fireAction,
			FireWeaponRuntimeState& state,
			const sas::GameplayAttributeList& attributes
		)
		{
			if (!state.initialized)
			{
				state.runtimeAttributes = fireAction.weaponDefinition.attributes;
				state.initialized = true;
			}
			if (context.instance)
			{
				context.instance->UpdatePrimaryWeaponRuntimeContext(
					fireAction.weaponDefinition,
					attributes,
					ResolveDamageTags(context, AttachmentHostKind::PrimaryWeapon)
				);
				state.persistentWeaponRuntime = &context.instance->GetPrimaryWeaponRuntime();
			}
			PrimaryWeaponRuntimeState& weaponRuntime = state.GetWeaponRuntime();
			if (state.lifecycleStarted)
			{
				return true;
			}

			const PrimaryWeaponValidationResult validation =
				PrimaryWeaponExecutionSystem::EnsureRuntimeConfigured(
					fireAction.weaponDefinition,
					weaponRuntime,
					context.definition
						? &context.definition->unlockedUpgradeIds
						: nullptr
				);
			if (!validation.isValid)
			{
				return false;
			}
			PrimaryWeaponExecutionSystem::BeginFire(
				MakePrimaryWeaponExecutionContext(owner, context, fireAction, attributes),
				weaponRuntime
			);
			state.lifecycleStarted = true;
			return true;
		}

		float BuildWeaponFireInterval(
			const AbilityExecutionContext& context,
			const FireWeaponAction& fireAction,
			const AbilityActionSpec& actionSpec,
			const sas::GameplayAttributeList& attributes
		)
		{
			const float baseInterval = PrimaryWeaponExecutionSystem::BuildBaseFireInterval(attributes, actionSpec.interval);
			return context.definition
				? BuildEffectiveActionInterval(
					*context.abilitySystem,
					*context.definition,
					baseInterval,
					&fireAction.weaponDefinition,
					context.instance
				)
				: baseInterval;
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
				if (!context.abilitySystem ||
					!action.spec ||
					!std::holds_alternative<FireWeaponAction>(action.spec->action) ||
					!std::holds_alternative<FireWeaponRuntimeState>(action.runtimeState))
				{
					return;
				}

				Actor& owner = context.abilitySystem->GetOwner();
				const FireWeaponAction& fireAction = std::get<FireWeaponAction>(action.spec->action);
				FireWeaponRuntimeState& state = std::get<FireWeaponRuntimeState>(action.runtimeState);
				if (!state.lifecycleStarted)
				{
					return;
				}
				const sas::GameplayAttributeList& values = ResolveFireWeaponAttributes(context, fireAction, state);
				PrimaryWeaponExecutionSystem::EndFire(
					MakePrimaryWeaponExecutionContext(owner, context, fireAction, values),
					state.GetWeaponRuntime()
				);
				state.lifecycleStarted = false;
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
			const FireWeaponAction& fireAction = std::get<FireWeaponAction>(action.spec->action);
			if (!std::holds_alternative<FireWeaponRuntimeState>(action.runtimeState))
			{
				FireWeaponRuntimeState state;
				state.runtimeAttributes = fireAction.weaponDefinition.attributes;
				state.intervalRemaining = context.instance
					? context.instance->GetWeaponFireIntervalRemaining()
					: 0.f;
				state.initialized = true;
				action.runtimeState = std::move(state);
			}

			FireWeaponRuntimeState& state = std::get<FireWeaponRuntimeState>(action.runtimeState);
			const sas::GameplayAttributeList& values = ResolveFireWeaponAttributes(context, fireAction, state);
			Actor& owner = context.abilitySystem->GetOwner();
			state.intervalRemaining = std::max(0.f, state.intervalRemaining - deltaTime);
			if (!state.lifecycleStarted && state.intervalRemaining > 0.f)
			{
				if (context.instance)
				{
					context.instance->SetWeaponFireIntervalRemaining(state.intervalRemaining);
				}
				return;
			}
			if (!EnsureFireWeaponLifecycle(owner, context, fireAction, state, values))
			{
				return;
			}
			PrimaryWeaponExecutionContext weaponContext = MakePrimaryWeaponExecutionContext(owner, context, fireAction, values);
			const auto applyRequestedWeaponCooldown = [&]()
			{
				const float requestedCooldown = PrimaryWeaponExecutionSystem::ConsumeRequestedCooldown(state.GetWeaponRuntime());
				if (requestedCooldown <= 0.f)
				{
					return false;
				}

				PrimaryWeaponExecutionSystem::EndFire(weaponContext, state.GetWeaponRuntime());
				state.lifecycleStarted = false;
				state.intervalRemaining = std::max(state.intervalRemaining, requestedCooldown);
				if (context.instance)
				{
					context.instance->SetWeaponFireIntervalRemaining(state.intervalRemaining);
				}
				return true;
			};
			PrimaryWeaponExecutionSystem::TickFire(weaponContext, state.GetWeaponRuntime(), deltaTime);
			if (applyRequestedWeaponCooldown())
			{
				return;
			}

			if (!PrimaryWeaponExecutionSystem::UsesIntervalFire(state.GetWeaponRuntime()))
			{
				return;
			}

			while (state.intervalRemaining <= 0.f &&
				(action.spec->maxExecutions <= 0 || state.executionCount < action.spec->maxExecutions))
			{
				const bool fired = PrimaryWeaponExecutionSystem::FireOnce(weaponContext, state.GetWeaponRuntime());
				if (fired)
				{
					++state.executionCount;
				}
				state.intervalRemaining += BuildWeaponFireInterval(context, fireAction, *action.spec, values);
				if (!fired)
				{
					break;
				}
			}
			if (applyRequestedWeaponCooldown())
			{
				return;
			}
			if (context.instance)
			{
				context.instance->SetWeaponFireIntervalRemaining(state.intervalRemaining);
			}
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
			const float nextInterval = context.definition
				? BuildEffectiveActionInterval(
					*context.abilitySystem,
					*context.definition,
					action.spec->interval,
					nullptr,
					context.instance
				)
				: action.spec->interval;
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
							*context.abilitySystem,
							*abilityDefinition,
							*effectDefinition,
							context.instance,
							BuildBaseDamageTags(abilityDefinition)
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
				if (!std::holds_alternative<FireWeaponRuntimeState>(action.runtimeState))
				{
					FireWeaponRuntimeState state;
					state.runtimeAttributes = actionData.weaponDefinition.attributes;
					state.intervalRemaining = context.instance
						? context.instance->GetWeaponFireIntervalRemaining()
						: 0.f;
					action.runtimeState = std::move(state);
				}
				FireWeaponRuntimeState& state = std::get<FireWeaponRuntimeState>(action.runtimeState);
				const sas::GameplayAttributeList& values = ResolveFireWeaponAttributes(context, actionData, state);
				if (state.intervalRemaining <= 0.f && EnsureFireWeaponLifecycle(owner, context, actionData, state, values))
				{
					if (PrimaryWeaponExecutionSystem::FireOnce(
						MakePrimaryWeaponExecutionContext(owner, context, actionData, values),
						state.GetWeaponRuntime()
					))
					{
						state.intervalRemaining = BuildWeaponFireInterval(context, actionData, *action.spec, values);
						if (context.instance)
						{
							context.instance->SetWeaponFireIntervalRemaining(state.intervalRemaining);
						}
					}
				}
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
				const sas::GameplayAttributeList values = ResolveAttributes(
					*context.abilitySystem,
					*context.definition,
					nullptr,
					attributes,
					context.instance,
					ResolveDamageTags(context, AttachmentHostKind::Ability)
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
						CommonAttributeIds::CollisionRadius,
						sas::FindGameplayAttributeValue(values, CommonAttributeIds::Radius, 0.f)
					);

					actor->SetActorLocation(spawnLocation);
					actor->SetActorRotation(ResolveActionRotation(direction, owner.GetActorRotation()));
					actor->SetLifeTime(duration);
					actor->SetDamage(damage);
					actor->SetDamageTags(ResolveDamageTags(context, AttachmentHostKind::Ability));
					actor->SetAbilityUpgradeIds(context.definition->unlockedUpgradeIds);
					actor->SetAbilityCollisionRadius(collisionRadius);
					actor->ConfigureCollisionFromOwner();
					actor->ConfigureFromAttributes(values);
				}
			}
		}, action.spec->action);
	}
}
