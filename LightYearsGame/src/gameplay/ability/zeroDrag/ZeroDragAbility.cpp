#include "gameplay/ability/zeroDrag/ZeroDragAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/zeroDrag/ZeroDragContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/movement/MovementPolicyService.h"
#include "gameplay/ship/ShipRuntimeModifiers.h"
#include "gameplay/tags/GameplayTags.h"
#include "framework/Actor.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr float Epsilon = 0.0001f;

		bool NearlyEqual(float left, float right)
		{
			return std::isfinite(left) && std::isfinite(right) &&
				std::abs(left - right) <= Epsilon;
		}

		bool IsFiniteNonNegative(float value)
		{
			return std::isfinite(value) && value >= 0.f;
		}

		const sas::GameplayAttribute* FindAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId
		)
		{
			return sas::FindAttribute(definition.attributes, attributeId);
		}

		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& attributeId,
			float fallback
		)
		{
			return sas::FindAttributeValue(values, attributeId, fallback);
		}

		sas::GameplayAttributeList ResolveAbilityValues(
			const GameAbilityBehaviorContext& context
		)
		{
			AbilityExecutionContext executionContext{
				const_cast<LightYearsAbilitySystemComponent*>(&context.abilitySystem),
				&context.definition,
				nullptr,
				&context.instance
			};
			return AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		}

		bool HasExpectedProgressionStep(const AbilityLevelStep& step, float cooldown, std::size_t index)
		{
			if (step.attributeModifiers.size() != 4 ||
				!step.scalingRules.empty() ||
				!step.unlockedUpgradeIds.empty() || !step.addedActions.empty() ||
				!step.addedTriggers.empty())
			{
				return false;
			}

			return std::any_of(
				step.attributeModifiers.begin(),
				step.attributeModifiers.end(),
				[](const sas::AttributeModifier& modifier)
				{
					return modifier.attributeId == CommonAttributeIds::Duration &&
						modifier.operation == sas::AttributeModifierOperation::Add &&
						NearlyEqual(modifier.magnitude, 0.10f);
				}
			) && std::any_of(
				step.attributeModifiers.begin(),
				step.attributeModifiers.end(),
				[](const sas::AttributeModifier& modifier)
				{
					return modifier.attributeId == AbilityData::ZeroDrag::Attribute::EnergyPowerDurationScale &&
						modifier.operation == sas::AttributeModifierOperation::Add &&
						NearlyEqual(modifier.magnitude, 0.05f);
				}
			) && std::any_of(
				step.attributeModifiers.begin(),
				step.attributeModifiers.end(),
				[](const sas::AttributeModifier& modifier)
				{
					return modifier.attributeId == AbilityData::ZeroDrag::Attribute::ThrustBonus &&
						modifier.operation == sas::AttributeModifierOperation::Add &&
						NearlyEqual(modifier.magnitude, 0.05f);
				}
			) && std::any_of(
				step.attributeModifiers.begin(), step.attributeModifiers.end(),
				[cooldown, index](const sas::AttributeModifier& modifier)
				{
					return modifier.attributeId == CommonAttributeIds::Cooldown &&
						modifier.operation == sas::AttributeModifierOperation::Add &&
						NearlyEqual(modifier.magnitude, -GetGlobalAbilityCooldownStepReduction(cooldown, index));
				}
			);
		}
	}

	bool ZeroDragAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const bool validIdentity =
			definition.abilityId == AbilityData::ZeroDrag::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::ZeroDrag &&
			definition.abilityTags.size() == 2 &&
			std::any_of(
				definition.abilityTags.begin(),
				definition.abilityTags.end(),
				[](const GameplayTag& tag)
				{
					return tag.MatchesTagExact(AbilityData::ZeroDrag::CategoryTag);
				}
			) && std::any_of(
				definition.abilityTags.begin(),
				definition.abilityTags.end(),
				[](const GameplayTag& tag)
				{
					return tag.MatchesTagExact(AbilityData::ZeroDrag::FamilyTag);
				}
			);
		const bool validLifecycle =
			sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Duration &&
			definition.maxCharges == 1 && NearlyEqual(definition.cooldown, 12.f) &&
			NearlyEqual(definition.duration, 4.5f);
		const bool validProgression = definition.levelProgression.size() == 14 &&
			std::all_of(definition.levelProgression.begin(), definition.levelProgression.end(),
				[&definition, index = std::size_t{ 0 }](const AbilityLevelStep& step) mutable
				{
					return HasExpectedProgressionStep(step, definition.cooldown, index++);
				});

		const List<sas::AttributeId> requiredAttributes = {
			AbilityData::ZeroDrag::Attribute::EnergyPowerReference,
			AbilityData::ZeroDrag::Attribute::EnergyPowerDurationScale,
			AbilityData::ZeroDrag::Attribute::ThrustBonus,
			AbilityData::ZeroDrag::Attribute::DampingMultiplier,
			AbilityData::ZeroDrag::Attribute::NormalizationDuration
		};
		const bool validAttributes = definition.attributes.size() == requiredAttributes.size() &&
			std::all_of(
				requiredAttributes.begin(),
				requiredAttributes.end(),
				[&definition](const sas::AttributeId& attributeId)
				{
					return FindAttribute(definition, attributeId) != nullptr;
				}
			);

		if (validAttributes)
		{
			const float energyReference = FindAttribute(
				definition,
				AbilityData::ZeroDrag::Attribute::EnergyPowerReference
			)->baseValue;
			const float durationScale = FindAttribute(
				definition,
				AbilityData::ZeroDrag::Attribute::EnergyPowerDurationScale
			)->baseValue;
			const float thrustBonus = FindAttribute(
				definition,
				AbilityData::ZeroDrag::Attribute::ThrustBonus
			)->baseValue;
			const float dampingMultiplier = FindAttribute(
				definition,
				AbilityData::ZeroDrag::Attribute::DampingMultiplier
			)->baseValue;
			const float normalization = FindAttribute(
				definition,
				AbilityData::ZeroDrag::Attribute::NormalizationDuration
			)->baseValue;

			if (!IsFiniteNonNegative(energyReference) ||
				!IsFiniteNonNegative(durationScale) ||
				!std::isfinite(thrustBonus) || thrustBonus < 0.f ||
				!std::isfinite(dampingMultiplier) || dampingMultiplier < 0.f || dampingMultiplier > 1.f ||
				!std::isfinite(normalization) || normalization <= 0.f)
			{
				if (failureReason)
				{
					*failureReason = "Zero Drag contains an invalid movement-policy attribute.";
				}
				return false;
			}
		}

		if (!validIdentity || !validLifecycle || !validProgression ||
			!validAttributes ||
			!definition.actions.empty() || !definition.effectSpecs.empty() ||
			!definition.scalingRules.empty() || !definition.damageTags.empty() ||
			!definition.triggers.empty())
		{
			if (failureReason)
			{
				*failureReason =
				"Zero Drag requires a movement-only lifecycle, five runtime attributes and fourteen duration progression steps.";
			}
			return false;
		}
		return true;
	}

	sas::GameplayAttributeList ZeroDragAbility::ResolveValues(
		const GameAbilityBehaviorContext& context
	) const
	{
		return ResolveAbilityValues(context);
	}

	float ZeroDragAbility::ResolveActiveDuration(
		const GameAbilityBehaviorContext& context,
		float defaultDuration
	) const
	{
		const sas::GameplayAttributeList values = ResolveValues(context);
		const float energyPower = std::max(
			0.f,
			context.abilitySystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower)
		);
		const float energyPowerReference = std::max(
			0.001f,
			FindValue(values, AbilityData::ZeroDrag::Attribute::EnergyPowerReference, 100.f)
		);
		const float energyPowerDurationScale = std::max(
			0.f,
			FindValue(values, AbilityData::ZeroDrag::Attribute::EnergyPowerDurationScale, 0.5f)
		);
		return std::max(
			0.f,
			defaultDuration + energyPower / energyPowerReference * energyPowerDurationScale
		);
	}

	float ZeroDragAbility::ResolveNormalizationDuration(
		const sas::GameplayAttributeList& values
	) const
	{
		return std::max(
			0.01f,
			FindValue(values, AbilityData::ZeroDrag::Attribute::NormalizationDuration, 1.1f)
		);
	}

	bool ZeroDragAbility::Activate(GameAbilityBehaviorContext& context)
	{
		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		if (mActive || !ship || !movement::MovementPolicyService::SupportsPolicies(context.owner))
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		movement::MovementPolicyRequest policy;
		policy.sourceId = context.definition.abilityId;
		const auto& movementAttributes = ship->GetMovementComponent().GetAttributes();
		const float normalDampingRetention = std::clamp(movementAttributes.linearDamping.currentValue, 0.f, 1.f);
		const float dampingMultiplier = std::clamp(
			FindValue(values, AbilityData::ZeroDrag::Attribute::DampingMultiplier, 0.2f), 0.f, 1.f);
		policy.dampingRetentionOverride = 1.f - (1.f - normalDampingRetention) * dampingMultiplier;
		policy.speedCapDisabledOverride = true;
		if (!movement::MovementPolicyService::SetPolicy(context.owner, policy))
		{
			return false;
		}

		const float thrustBonus = std::max(
			0.f,
			FindValue(values, AbilityData::ZeroDrag::Attribute::ThrustBonus, 0.60f)
		);
		// The reusable runtime modifier affects thrust; the movement policy owns
		// the reduced drift damping and disabled speed cap.
		ShipRuntimeModifier modifier;
		modifier.thrustBonus = thrustBonus;
		ship->GetRuntimeModifiers().Set(context.definition.abilityId, std::move(modifier));
		mResolvedNormalizationDuration = ResolveNormalizationDuration(values);
		context.abilitySystem.AddOwnedTag(AbilityData::ZeroDrag::State::Active);
		mActive = true;
		EmitEvent(context, AbilityData::ZeroDrag::Event::Started);
		return true;
	}

	void ZeroDragAbility::End(
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
			mResolvedNormalizationDuration
		);
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner))
		{
			ship->GetRuntimeModifiers().Remove(context.definition.abilityId);
		}
		context.abilitySystem.RemoveOwnedTag(AbilityData::ZeroDrag::State::Active);
		mActive = false;
		EmitEvent(context, AbilityData::ZeroDrag::Event::Ended);
	}

	void ZeroDragAbility::OnOwnerAbilityActivated(
		GameAbilityBehaviorContext& context,
		const sas::AbilityLifecycleEvent& event
	)
	{
		if (!mActive || !std::any_of(
			event.abilityTags.begin(),
			event.abilityTags.end(),
			[](const GameplayTag& tag)
			{
				return tag.MatchesTagExact(GameplayTags::Ability::Family::PhaseDrift);
			}
		))
		{
			return;
		}

		// Phase Drift and Zero Drag are mutually exclusive movement policies:
		// Phase's activation is the explicit action that ends Zero Drag.
		context.instance.Cancel(sas::AbilityEndReason::Interrupted);
	}

	void ZeroDragAbility::EmitEvent(
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
