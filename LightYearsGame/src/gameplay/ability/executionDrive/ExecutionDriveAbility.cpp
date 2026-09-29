#include "gameplay/ability/executionDrive/ExecutionDriveAbility.h"

#include "effects/GameplayEffectSpec.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/executionDrive/ExecutionDriveContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/ship/ShipRuntimeModifiers.h"
#include "spaceShip/SpaceShip.h"

#include <cmath>
#include <optional>
#include <utility>

namespace ly
{
	namespace
	{
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

		sas::GameplayAttributeList ResolveValues(GameAbilityBehaviorContext& context)
		{
			AbilityExecutionContext executionContext{
				&context.abilitySystem,
				&context.definition,
				nullptr,
				&context.instance
			};
			return AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		}

		std::optional<float> FindAddModifierMagnitude(
			const AbilityLevelStep& step,
			const sas::AttributeId& attributeId
		)
		{
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == attributeId &&
					modifier.operation == sas::AttributeModifierOperation::Add)
				{
					return modifier.magnitude;
				}
			}
			return std::nullopt;
		}
	}

	bool ExecutionDriveAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::ExecutionDrive::AbilityId::Basic ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 || !std::isfinite(definition.cooldown) ||
			definition.cooldown <= 0.f || !std::isfinite(definition.duration) ||
			definition.duration <= 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Execution Drive requires a pressed, one-charge duration definition.";
			}
			return false;
		}

		for (const sas::AttributeId& attributeId : {
			AbilityData::ExecutionDrive::Attribute::FlatAttackPowerBonus,
			AbilityData::ExecutionDrive::Attribute::AttackPowerScale,
			AbilityData::ExecutionDrive::Attribute::MoveSpeedBonus,
			AbilityData::ExecutionDrive::Attribute::Range
		})
		{
			const sas::GameplayAttribute* attribute = FindAttribute(definition, attributeId);
			if (!attribute || !std::isfinite(attribute->baseValue))
			{
				if (failureReason)
				{
					*failureReason = "Execution Drive must declare finite buff and range attributes.";
				}
				return false;
			}
		}

		if (FindAttribute(definition, AbilityData::ExecutionDrive::Attribute::FlatAttackPowerBonus)->baseValue < 0.f ||
			FindAttribute(definition, AbilityData::ExecutionDrive::Attribute::AttackPowerScale)->baseValue < 0.f ||
			FindAttribute(definition, AbilityData::ExecutionDrive::Attribute::MoveSpeedBonus)->baseValue < 0.f ||
			FindAttribute(definition, AbilityData::ExecutionDrive::Attribute::Range)->baseValue <= 0.f ||
			!EffectData::FindGameplayEffectDefinition(AbilityData::ExecutionDrive::Effect::AttackPowerId))
		{
			if (failureReason)
			{
				*failureReason = "Execution Drive has invalid buff, range, or effect configuration.";
			}
			return false;
		}

		if ((definition.levelProgression.empty() && definition.repeatingLevelProgression.empty()) ||
			!definition.triggers.empty())
		{
			if (failureReason)
			{
				*failureReason = "Execution Drive requires at least one level step and no kill triggers.";
			}
			return false;
		}
		List<AbilityLevelStep> levelSteps = definition.levelProgression;
		levelSteps.insert(levelSteps.end(), definition.repeatingLevelProgression.begin(),
			definition.repeatingLevelProgression.end());
		for (const AbilityLevelStep& step : levelSteps)
		{
			const std::optional<float> flatBonus = FindAddModifierMagnitude(
				step,
				AbilityData::ExecutionDrive::Attribute::FlatAttackPowerBonus
			);
			const std::optional<float> attackPowerScale = FindAddModifierMagnitude(
				step,
				AbilityData::ExecutionDrive::Attribute::AttackPowerScale
			);
			const std::optional<float> moveSpeedBonus = FindAddModifierMagnitude(
				step,
				AbilityData::ExecutionDrive::Attribute::MoveSpeedBonus
			);
			const bool validModifiers = flatBonus && std::isfinite(*flatBonus) && *flatBonus >= 0.f &&
				attackPowerScale && std::isfinite(*attackPowerScale) && *attackPowerScale >= 0.f &&
				moveSpeedBonus && std::isfinite(*moveSpeedBonus) && *moveSpeedBonus >= 0.f;
			if (step.attributeModifiers.size() != 3 || !step.scalingRules.empty() ||
				!validModifiers)
			{
				if (failureReason)
				{
					*failureReason = "Execution Drive progression requires three finite additive buff modifiers and no scaling rules.";
				}
				return false;
			}
		}
		return true;
	}

	bool ExecutionDriveAbility::Activate(GameAbilityBehaviorContext& context)
	{
		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		if (!ship || mActive)
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		const float preBuffAttackPower = context.abilitySystem.GetAttributes().GetCurrentValue(
			OwnerAttributeIds::AttackPower
		);
		const float flatAttackPowerBonus = std::max(
			0.f,
			FindValue(values, AbilityData::ExecutionDrive::Attribute::FlatAttackPowerBonus, 20.f)
		);
		const float attackPowerScale = std::max(
			0.f,
			FindValue(values, AbilityData::ExecutionDrive::Attribute::AttackPowerScale, 0.10f)
		);
		const float moveSpeedBonus = std::max(
			0.f,
			FindValue(values, AbilityData::ExecutionDrive::Attribute::MoveSpeedBonus, 0.15f)
		);
		const float totalAttackPowerBonus = std::max(
			0.f,
			flatAttackPowerBonus + preBuffAttackPower * attackPowerScale
		);

		const sas::GameplayEffectDefinition* effectDefinition =
			EffectData::FindGameplayEffectDefinition(
				AbilityData::ExecutionDrive::Effect::AttackPowerId
			);
		if (!effectDefinition)
		{
			return false;
		}

		sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*effectDefinition);
		spec.duration = context.definition.duration;
		spec.maxStacks = 1;
		spec.modifiers = {
			sas::AttributeModifier{
				OwnerAttributeIds::AttackPower,
				sas::AttributeModifierOperation::Add,
				totalAttackPowerBonus
			}
		};
		mAttackPowerEffectHandle = context.abilitySystem.ApplyGameplayEffect(
			spec,
			sas::GameplayEffectSourceContext{ &context.owner, &context.instance }
		);
		if (!mAttackPowerEffectHandle.IsValid())
		{
			return false;
		}

		ShipRuntimeModifier modifier;
		modifier.movementSpeedMultiplier = 1.f + moveSpeedBonus;
		ship->GetRuntimeModifiers().Set(
			AbilityData::ExecutionDrive::AbilityId::Basic,
			std::move(modifier)
		);

		mActive = true;
		context.abilitySystem.AddOwnedTag(AbilityData::ExecutionDrive::State::Active);
		EmitEvent(context, AbilityData::ExecutionDrive::Event::Started);
		return true;
	}

	void ExecutionDriveAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason
	)
	{
		if (!mActive && !mAttackPowerEffectHandle.IsValid())
		{
			return;
		}

		if (mAttackPowerEffectHandle.IsValid())
		{
			context.abilitySystem.RemoveGameplayEffect(mAttackPowerEffectHandle);
			mAttackPowerEffectHandle = {};
		}
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner))
		{
			ship->GetRuntimeModifiers().Remove(AbilityData::ExecutionDrive::AbilityId::Basic);
		}
		context.abilitySystem.RemoveOwnedTag(AbilityData::ExecutionDrive::State::Active);
		mActive = false;
		EmitEvent(context, AbilityData::ExecutionDrive::Event::Ended);
	}

	void ExecutionDriveAbility::EmitEvent(
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
