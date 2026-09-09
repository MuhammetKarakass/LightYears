#include "gameplay/ability/inertialWake/InertialWakeAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/inertialWake/InertialWakeContracts.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/movement/MovementPolicyService.h"
#include "gameplay/ship/ShipRuntimeModifiers.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <variant>

namespace ly
{
	namespace
	{
		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& id,
			float fallback
		)
		{
			return sas::FindAttributeValue(values, id, fallback);
		}

		bool HasWakeSpawnAction(const GameAbilityDefinition& definition)
		{
			return std::any_of(
				definition.actions.begin(),
				definition.actions.end(),
				[](const AbilityActionSpec& action)
				{
					return action.phase == sas::AbilityActionPhase::OnActivate &&
						std::holds_alternative<SpawnActorAction>(action.action);
				}
			);
		}
	}

	bool InertialWakeAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const bool validIdentity =
			definition.abilityId == AbilityData::InertialWake::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::InertialWake &&
			std::any_of(
				definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag) { return tag.MatchesTagExact(AbilityData::InertialWake::CategoryTag); }
			) && std::any_of(
				definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag) { return tag.MatchesTagExact(AbilityData::InertialWake::FamilyTag); }
			);
		const bool validLifecycle =
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Duration &&
			definition.duration > 0.f && definition.cooldown > 0.f &&
			definition.maxCharges == 1;
		const bool hasMovementAttributes =
			sas::FindAttribute(definition.attributes, AbilityData::InertialWake::Attribute::TopSpeedBonus) &&
			sas::FindAttribute(definition.attributes, AbilityData::InertialWake::Attribute::ThrustBonus) &&
			sas::FindAttribute(definition.attributes, AbilityData::InertialWake::Attribute::NormalizationDuration);

		if (!validIdentity || !validLifecycle || !hasMovementAttributes ||
			!HasWakeSpawnAction(definition))
		{
			if (failureReason)
			{
				*failureReason =
					"Inertial Wake requires its offense/family identity, duration lifecycle, movement attributes and an OnActivate wake spawn.";
			}
			return false;
		}
		return true;
	}

	sas::GameplayAttributeList InertialWakeAbility::ResolveValues(
		const GameAbilityBehaviorContext& context
	) const
	{
		AbilityExecutionContext executionContext{
			const_cast<LightYearsAbilitySystemComponent*>(&context.abilitySystem),
			&context.definition,
			nullptr,
			&context.instance
		};
		return AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
	}

	bool InertialWakeAbility::Activate(GameAbilityBehaviorContext& context)
	{
		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		if (mActive || !ship || !movement::MovementPolicyService::SupportsPolicies(context.owner))
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		movement::MovementPolicyRequest policy;
		policy.sourceId = context.definition.abilityId;
		// Flat cap bonuses compose in the policy controller. Inertial Wake never
		// replaces another movement source's cap contribution.
		policy.speedCapFlatBonus = std::max(
			0.f,
			FindValue(values, AbilityData::InertialWake::Attribute::TopSpeedBonus, 100.f)
		);
		if (!movement::MovementPolicyService::SetPolicy(context.owner, policy))
		{
			return false;
		}

		ShipRuntimeModifier modifier;
		// Runtime modifiers are additive too: Zero Drag, Afterburner-related
		// systems and later movement abilities retain their own contributions.
		modifier.thrustBonus = std::max(
			0.f,
			FindValue(values, AbilityData::InertialWake::Attribute::ThrustBonus, 0.15f)
		);
		ship->GetRuntimeModifiers().Set(context.definition.abilityId, std::move(modifier));
		mNormalizationDuration = std::max(
			0.01f,
			FindValue(values, AbilityData::InertialWake::Attribute::NormalizationDuration, 0.5f)
		);
		context.abilitySystem.AddOwnedTag(AbilityData::InertialWake::State::Active);
		mActive = true;
		EmitEvent(context, AbilityData::InertialWake::Event::Started);
		return true;
	}

	void InertialWakeAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		if (!mActive)
		{
			return;
		}

		movement::MovementPolicyService::ReleasePolicy(
			context.owner,
			context.definition.abilityId,
			movement::MovementPolicyReleaseMode::Normalize,
			mNormalizationDuration
		);
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner))
		{
			ship->GetRuntimeModifiers().Remove(context.definition.abilityId);
		}
		context.abilitySystem.RemoveOwnedTag(AbilityData::InertialWake::State::Active);
		mActive = false;
		EmitEvent(context, AbilityData::InertialWake::Event::Ended);
	}

	void InertialWakeAbility::EmitEvent(
		GameAbilityBehaviorContext& context,
		const GameplayTag& eventTag
	) const
	{
		sas::AbilityEvent event;
		event.eventTag = eventTag;
		event.sourceAbilityId = sas::ContentId{ context.definition.abilityId };
		event.sourceAbilityTags = context.definition.abilityTags;
		event.SetSource(&context.owner);
		event.SetTarget(&context.owner);
		context.abilitySystem.HandleGameplayEvent(event);
	}
}
