#include "gameplay/ability/actions/FireWeaponActionRuntime.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include "gameplay/time/IntervalDebt.h"
#include "framework/Actor.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>

namespace ly
{
	namespace
	{
		const PrimaryWeaponOverrideState::ActiveOverride* FindPrimaryOverride(
			const AbilityExecutionContext& context
		)
		{
			if (!context.abilitySystem || !context.definition ||
				context.definition->slot != sas::AbilitySlot::PrimaryFire)
			{
				return nullptr;
			}
			return context.abilitySystem->GetActivePrimaryWeaponOverride();
		}

		const FireWeaponAction& ResolveFireAction(
			const AbilityExecutionContext& context,
			const FireWeaponAction& authoredAction,
			FireWeaponAction& overrideStorage
		)
		{
			const PrimaryWeaponOverrideState::ActiveOverride* overrideEntry =
				FindPrimaryOverride(context);
			if (!overrideEntry)
			{
				return authoredAction;
			}

			// A normal primary shot does not allocate or copy its weapon definition.
			// The short-lived copy exists only while a transformation replaces it.
			overrideStorage = authoredAction;
			overrideStorage.weaponDefinition = overrideEntry->weaponDefinition;
			return overrideStorage;
		}

		List<GameplayTag> ResolveDamageTags(
			AbilityExecutionContext& context,
			const FireWeaponAction& fireAction
		)
		{
			if (const PrimaryWeaponOverrideState::ActiveOverride* overrideEntry =
				FindPrimaryOverride(context);
				overrideEntry &&
				overrideEntry->weaponDefinition.weaponId == fireAction.weaponDefinition.weaponId)
			{
				// A replacement weapon owns its damage family and must not inherit the
				// equipped weapon's tags.
				return fireAction.weaponDefinition.damageTags;
			}
			return AbilityActionAttributeResolver::ResolveDamageTags(
				context,
				AttachmentHostKind::PrimaryWeapon
			);
		}

		const sas::GameplayAttributeList& ResolveAttributes(
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
			const uint64_t configurationRevision = context.instance
				? context.instance->GetConfigurationRevision()
				: 0;
			if (!state.hasResolvedAttributes || state.resolvedAttributeRevision != attributeRevision ||
				state.resolvedAttachmentRevision != attachmentRevision ||
				state.resolvedConfigurationRevision != configurationRevision)
			{
				state.resolvedAttributes = context.definition
					? AbilityActionAttributeResolver::ResolveAttributes(
						context,
						&fireAction.weaponDefinition,
						state.runtimeAttributes,
						ResolveDamageTags(context, fireAction)
					)
					: sas::BuildBaseGameplayAttributes(state.runtimeAttributes);
				state.resolvedAttributeRevision = attributeRevision;
				state.resolvedAttachmentRevision = attachmentRevision;
				state.resolvedConfigurationRevision = configurationRevision;
				state.hasResolvedAttributes = true;
			}
			return state.resolvedAttributes;
		}

		PrimaryWeaponExecutionContext MakeExecutionContext(
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
				ResolveDamageTags(context, fireAction),
				context.definition ? &context.definition->unlockedUpgradeIds : nullptr
			};
		}

		bool EnsureLifecycle(
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
			if (PrimaryWeaponOverrideState::ActiveOverride* overrideEntry =
				context.abilitySystem
					? context.abilitySystem->GetActivePrimaryWeaponOverride()
					: nullptr;
				overrideEntry && context.definition &&
				context.definition->slot == sas::AbilitySlot::PrimaryFire &&
				overrideEntry->weaponDefinition.weaponId == fireAction.weaponDefinition.weaponId)
			{
				// The form owns a separate runtime. Its firing state therefore cannot
				// overwrite heat, cooldown, or burst state of the equipped primary.
				state.persistentWeaponRuntime = &overrideEntry->runtime;
			}
			else if (context.instance)
			{
				context.instance->UpdatePrimaryWeaponRuntimeContext(
					fireAction.weaponDefinition,
					attributes,
					ResolveDamageTags(context, fireAction)
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
					context.definition ? &context.definition->unlockedUpgradeIds : nullptr
				);
			if (!validation.isValid)
			{
				return false;
			}
			if (weaponRuntime.fireIntervalRemaining > 0.f)
			{
				return true;
			}
			PrimaryWeaponExecutionSystem::BeginFire(
				MakeExecutionContext(owner, context, fireAction, attributes),
				weaponRuntime
			);
			state.lifecycleStarted = true;
			return true;
		}

		float BuildFireInterval(
			AbilityExecutionContext& context,
			const FireWeaponAction& fireAction,
			const AbilityActionSpec& actionSpec,
			const sas::GameplayAttributeList& attributes
		)
		{
			float resolvedInterval = 0.f;
			if (fireAction.weaponDefinition.cadenceMode ==
				PrimaryWeaponCadenceMode::OwnerAttackSpeedPercentage)
			{
				const float resolvedWeaponFireRate = std::max(
					0.01f,
					sas::FindAttributeValue(
						attributes,
						CommonAttributeIds::FireRate,
						1.f
					)
				);
				const float attackSpeedMultiplier =
					context.abilitySystem
						? PrimaryWeaponExecutionSystem::CalculateAttackSpeedMultiplier(
							context.abilitySystem->GetOwner()
						)
						: 1.f;
				const float finalFireRate = resolvedWeaponFireRate * attackSpeedMultiplier;
				resolvedInterval = 1.f / std::max(0.0001f, finalFireRate);
			}
			else
			{
				const float baseInterval = PrimaryWeaponExecutionSystem::BuildBaseFireInterval(
					attributes,
					actionSpec.interval
				);
				resolvedInterval = context.definition
					? AbilityActionAttributeResolver::ResolveEffectiveInterval(
						context,
						baseInterval,
						&fireAction.weaponDefinition
					)
					: baseInterval;
			}

			if (!context.definition ||
				context.definition->slot != sas::AbilitySlot::PrimaryFire)
			{
				return resolvedInterval;
			}

			const auto* ship = dynamic_cast<const SpaceShip*>(&context.abilitySystem->GetOwner());
			const float fireRateMultiplier = ship
				? ship->GetPrimaryWeaponFireRateMultiplier()
				: 1.f;
			return fireRateMultiplier > 0.f
				? resolvedInterval / fireRateMultiplier
				: resolvedInterval;
		}

		FireWeaponRuntimeState& GetOrCreateState(
			ActiveAbilityAction& action,
			AbilityExecutionContext& context,
			const FireWeaponAction& fireAction,
			bool markInitialized
		)
		{
			if (!std::holds_alternative<FireWeaponRuntimeState>(action.runtimeState))
			{
				FireWeaponRuntimeState state;
				state.runtimeAttributes = fireAction.weaponDefinition.attributes;
				state.initialized = markInitialized;
				action.runtimeState = std::move(state);
			}
			return std::get<FireWeaponRuntimeState>(action.runtimeState);
		}
	}

	void FireWeaponActionRuntime::Execute(
		ActiveAbilityAction& action,
		AbilityExecutionContext& context
	)
	{
		if (!action.spec || !context.abilitySystem ||
			!std::holds_alternative<FireWeaponAction>(action.spec->action))
		{
			return;
		}
		Actor& owner = context.abilitySystem->GetOwner();
		const FireWeaponAction& authoredAction = std::get<FireWeaponAction>(action.spec->action);
		FireWeaponAction overrideAction;
		const FireWeaponAction& fireAction = ResolveFireAction(context, authoredAction, overrideAction);
		FireWeaponRuntimeState& state = GetOrCreateState(action, context, fireAction, false);
		const sas::GameplayAttributeList& values = ResolveAttributes(context, fireAction, state);
		if (!EnsureLifecycle(owner, context, fireAction, state, values))
		{
			return;
		}
		PrimaryWeaponRuntimeState& weaponRuntime = state.GetWeaponRuntime();
		if (weaponRuntime.fireIntervalRemaining <= 0.f)
		{
			if (PrimaryWeaponExecutionSystem::FireOnce(
					MakeExecutionContext(owner, context, fireAction, values),
					weaponRuntime
				))
			{
				weaponRuntime.fireIntervalRemaining = BuildFireInterval(context, fireAction, *action.spec, values);
			}
		}
	}

	void FireWeaponActionRuntime::Tick(
		ActiveAbilityAction& action,
		AbilityExecutionContext& context,
		float deltaTime
	)
	{
		if (!action.spec || !context.abilitySystem ||
			action.spec->phase != sas::AbilityActionPhase::WhileActive ||
			!std::holds_alternative<FireWeaponAction>(action.spec->action))
		{
			return;
		}
		Actor& owner = context.abilitySystem->GetOwner();
		const FireWeaponAction& authoredAction = std::get<FireWeaponAction>(action.spec->action);
		FireWeaponAction overrideAction;
		const FireWeaponAction& fireAction = ResolveFireAction(context, authoredAction, overrideAction);
		FireWeaponRuntimeState& state = GetOrCreateState(action, context, fireAction, true);
		const sas::GameplayAttributeList& values = ResolveAttributes(context, fireAction, state);

		if (!EnsureLifecycle(owner, context, fireAction, state, values))
		{
			return;
		}

		PrimaryWeaponExecutionContext weaponContext = MakeExecutionContext(
			owner,
			context,
			fireAction,
			values
		);
		PrimaryWeaponRuntimeState& weaponRuntime = state.GetWeaponRuntime();
		if (!state.lifecycleStarted && weaponRuntime.fireIntervalRemaining > 0.f)
		{
			const float inactiveDeltaTime = std::min(deltaTime, weaponRuntime.fireIntervalRemaining);
			PrimaryWeaponExecutionSystem::TickInactive(weaponContext, weaponRuntime, inactiveDeltaTime);
			deltaTime = std::max(0.f, deltaTime - inactiveDeltaTime);
			if (weaponRuntime.fireIntervalRemaining > 0.f)
			{
				return;
			}
			if (!EnsureLifecycle(owner, context, fireAction, state, values))
			{
				return;
			}
		}

		const float fireInterval = BuildFireInterval(context, fireAction, *action.spec, values);
		const auto simResult = PrimaryWeaponExecutionSystem::SimulateActiveFire(
			weaponContext,
			weaponRuntime,
			deltaTime,
			fireInterval,
			action.spec->maxExecutions,
			state.executionCount
		);
		state.executionCount += simResult.executionsProduced;
		if (simResult.lifecycleInterrupted)
		{
			state.lifecycleStarted = false;
		}
	}

	void FireWeaponActionRuntime::End(
		ActiveAbilityAction& action,
		AbilityExecutionContext& context
	)
	{
		if (!action.spec || !context.abilitySystem ||
			!std::holds_alternative<FireWeaponAction>(action.spec->action) ||
			!std::holds_alternative<FireWeaponRuntimeState>(action.runtimeState))
		{
			return;
		}
		FireWeaponRuntimeState& state = std::get<FireWeaponRuntimeState>(action.runtimeState);
		if (!state.lifecycleStarted)
		{
			return;
		}
		Actor& owner = context.abilitySystem->GetOwner();
		const FireWeaponAction& authoredAction = std::get<FireWeaponAction>(action.spec->action);
		FireWeaponAction overrideAction;
		const FireWeaponAction& fireAction = ResolveFireAction(context, authoredAction, overrideAction);
		PrimaryWeaponExecutionSystem::EndFire(
			MakeExecutionContext(owner, context, fireAction, ResolveAttributes(context, fireAction, state)),
			state.GetWeaponRuntime()
		);
		state.lifecycleStarted = false;

		// A catch-up-limited frame may leave elapsed simulation time on the
		// persistent weapon runtime. Once input is released, that elapsed time may
		// advance cooldown/reload, but it must not synthesize shots on the next press.
		PrimaryWeaponRuntimeState& weaponRuntime = state.GetWeaponRuntime();
		const float pendingTime = std::max(0.f, weaponRuntime.unprocessedSimulationTime);
		weaponRuntime.unprocessedSimulationTime = 0.f;
		if (pendingTime > 0.f)
		{
			PrimaryWeaponExecutionSystem::TickInactive(
				MakeExecutionContext(owner, context, fireAction, ResolveAttributes(context, fireAction, state)),
				weaponRuntime,
				pendingTime
			);
		}

		// The override runtime lives inside the override state's vector, so a
		// later erase or push_back would dangle this pointer between frames.
		// Nothing reads it before the next lifecycle starts, so drop it now.
		state.persistentWeaponRuntime = nullptr;
	}
}
