#include "gameplay/ability/actions/FireWeaponActionRuntime.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include "gameplay/time/IntervalDebt.h"
#include "framework/Actor.h"

#include <algorithm>

namespace ly
{
	namespace
	{
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
			if (!state.hasResolvedAttributes || state.resolvedAttributeRevision != attributeRevision ||
				state.resolvedAttachmentRevision != attachmentRevision)
			{
				state.resolvedAttributes = context.definition
					? AbilityActionAttributeResolver::ResolveAttributes(
						context,
						&fireAction.weaponDefinition,
						state.runtimeAttributes,
						AbilityActionAttributeResolver::ResolveDamageTags(
							context,
							AttachmentHostKind::PrimaryWeapon
						)
					)
					: sas::BuildBaseGameplayAttributes(state.runtimeAttributes);
				state.resolvedAttributeRevision = attributeRevision;
				state.resolvedAttachmentRevision = attachmentRevision;
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
				AbilityActionAttributeResolver::ResolveDamageTags(
					context,
					AttachmentHostKind::PrimaryWeapon
				),
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
			if (context.instance)
			{
				context.instance->UpdatePrimaryWeaponRuntimeContext(
					fireAction.weaponDefinition,
					attributes,
					AbilityActionAttributeResolver::ResolveDamageTags(
						context,
						AttachmentHostKind::PrimaryWeapon
					)
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
			const float baseInterval = PrimaryWeaponExecutionSystem::BuildBaseFireInterval(
				attributes,
				actionSpec.interval
			);
			return context.definition
				? AbilityActionAttributeResolver::ResolveEffectiveInterval(
					context,
					baseInterval,
					&fireAction.weaponDefinition
				)
				: baseInterval;
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
				state.intervalRemaining = context.instance
					? context.instance->GetWeaponFireIntervalRemaining()
					: 0.f;
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
		const FireWeaponAction& fireAction = std::get<FireWeaponAction>(action.spec->action);
		FireWeaponRuntimeState& state = GetOrCreateState(action, context, fireAction, false);
		const sas::GameplayAttributeList& values = ResolveAttributes(context, fireAction, state);
		if (state.intervalRemaining <= 0.f && EnsureLifecycle(owner, context, fireAction, state, values) &&
			PrimaryWeaponExecutionSystem::FireOnce(
				MakeExecutionContext(owner, context, fireAction, values),
				state.GetWeaponRuntime()
			))
		{
			state.intervalRemaining = BuildFireInterval(context, fireAction, *action.spec, values);
			if (context.instance)
			{
				context.instance->SetWeaponFireIntervalRemaining(state.intervalRemaining);
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
		const FireWeaponAction& fireAction = std::get<FireWeaponAction>(action.spec->action);
		FireWeaponRuntimeState& state = GetOrCreateState(action, context, fireAction, true);
		const sas::GameplayAttributeList& values = ResolveAttributes(context, fireAction, state);
		// Preserve negative interval debt. A long frame may owe more than one shot;
		// clamping here made automatic-weapon DPS depend on frame rate.
		time::AdvanceIntervalDebt(state.intervalRemaining, deltaTime);
		if (!state.lifecycleStarted && state.intervalRemaining > 0.f)
		{
			if (context.instance)
			{
				context.instance->SetWeaponFireIntervalRemaining(state.intervalRemaining);
			}
			return;
		}
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
		const auto applyRequestedWeaponCooldown = [&]()
		{
			const float requestedCooldown = PrimaryWeaponExecutionSystem::ConsumeRequestedCooldown(
				state.GetWeaponRuntime()
			);
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
		if (applyRequestedWeaponCooldown() ||
			!PrimaryWeaponExecutionSystem::UsesIntervalFire(state.GetWeaponRuntime()))
		{
			return;
		}
		int catchUpExecutions = 0;
		while (state.intervalRemaining <= 0.f &&
			catchUpExecutions < time::DefaultMaximumIntervalCatchUp &&
			(action.spec->maxExecutions <= 0 || state.executionCount < action.spec->maxExecutions))
		{
			const bool fired = PrimaryWeaponExecutionSystem::FireOnce(
				weaponContext,
				state.GetWeaponRuntime()
			);
			if (fired)
			{
				++state.executionCount;
				++catchUpExecutions;
			}
			time::CommitInterval(
				state.intervalRemaining,
				BuildFireInterval(context, fireAction, *action.spec, values)
			);
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
		const FireWeaponAction& fireAction = std::get<FireWeaponAction>(action.spec->action);
		PrimaryWeaponExecutionSystem::EndFire(
			MakeExecutionContext(owner, context, fireAction, ResolveAttributes(context, fireAction, state)),
			state.GetWeaponRuntime()
		);
		state.lifecycleStarted = false;
	}
}
