#include "gameplay/ability/executionDrive/ExecutionDriveAbility.h"

#include "effects/GameplayEffectSpec.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/executionDrive/ExecutionDriveContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/ship/ShipRuntimeModifiers.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <cmath>

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

		bool IsFiniteNonNegative(float value)
		{
			return std::isfinite(value) && value >= 0.f;
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
			definition.maxCharges != 1 ||
			!std::isfinite(definition.cooldown) || definition.cooldown <= 0.f ||
			!std::isfinite(definition.duration) || definition.duration <= 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Execution Drive requires a pressed, one-charge duration definition.";
			}
			return false;
		}

		for (const sas::AttributeId& attributeId : {
			AbilityData::ExecutionDrive::Attribute::BaseAttackPowerBonus,
			AbilityData::ExecutionDrive::Attribute::AttackPowerPerStack,
			AbilityData::ExecutionDrive::Attribute::BaseChaseMovementBonus,
			AbilityData::ExecutionDrive::Attribute::ChaseMovementPerStack,
			AbilityData::ExecutionDrive::Attribute::AttackPowerChaseScale,
			AbilityData::ExecutionDrive::Attribute::TargetingRange,
			AbilityData::ExecutionDrive::Attribute::DirectionThreshold
		})
		{
			const sas::GameplayAttribute* attribute = FindAttribute(definition, attributeId);
			if (!attribute || !std::isfinite(attribute->baseValue))
			{
				if (failureReason)
				{
					*failureReason = "Execution Drive must declare all runtime attributes.";
				}
				return false;
			}
		}

		const sas::GameplayAttribute* range = FindAttribute(
			definition,
			AbilityData::ExecutionDrive::Attribute::TargetingRange
		);
		const sas::GameplayAttribute* threshold = FindAttribute(
			definition,
			AbilityData::ExecutionDrive::Attribute::DirectionThreshold
		);
		if (!range || range->baseValue <= 0.f ||
			!threshold || threshold->baseValue < -1.f || threshold->baseValue > 1.f ||
			!EffectData::FindGameplayEffectDefinition(
				AbilityData::ExecutionDrive::Effect::AttackPowerId
			))
		{
			if (failureReason)
			{
				*failureReason = "Execution Drive has invalid targeting or effect configuration.";
			}
			return false;
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
		mBaseAttackPowerBonus = std::max(
			0.f,
			FindValue(values, AbilityData::ExecutionDrive::Attribute::BaseAttackPowerBonus, 10.f)
		);
		mAttackPowerPerStack = std::max(
			0.f,
			FindValue(values, AbilityData::ExecutionDrive::Attribute::AttackPowerPerStack, 3.f)
		);
		mBaseChaseMovementBonus = std::max(
			0.f,
			FindValue(values, AbilityData::ExecutionDrive::Attribute::BaseChaseMovementBonus, 0.10f)
		);
		mChaseMovementPerStack = std::max(
			0.f,
			FindValue(values, AbilityData::ExecutionDrive::Attribute::ChaseMovementPerStack, 0.02f)
		);
		mAttackPowerChaseScale = std::max(
			0.f,
			FindValue(values, AbilityData::ExecutionDrive::Attribute::AttackPowerChaseScale, 0.001f)
		);
		mTargetingRange = std::max(
			0.f,
			FindValue(values, AbilityData::ExecutionDrive::Attribute::TargetingRange, 700.f)
		);
		mDirectionThreshold = std::clamp(
			FindValue(values, AbilityData::ExecutionDrive::Attribute::DirectionThreshold, 0.25f),
			-1.f,
			1.f
		);
		mStackCount = 0;
		mCurrentAttackPowerBonus = mBaseAttackPowerBonus;

		mAbilitySystem = &context.abilitySystem;
		mShip = ship;
		RefreshPowerEffect(context);
		if (!mAttackPowerEffectHandle.IsValid())
		{
			mAbilitySystem = nullptr;
			mShip = nullptr;
			return false;
		}

		ShipRuntimeModifier modifier;
		modifier.movementSpeedResolver = [this](const sf::Vector2f& direction)
		{
			return ResolveChaseMovementMultiplier(direction);
		};
		ship->GetRuntimeModifiers().Set(
			AbilityData::ExecutionDrive::AbilityId::Basic,
			std::move(modifier)
		);

		mActive = true;
		context.abilitySystem.AddOwnedTag(AbilityData::ExecutionDrive::State::Active);
		EmitEvent(context, AbilityData::ExecutionDrive::Event::Started);
		return true;
	}

	void ExecutionDriveAbility::OnGameplayEvent(
		GameAbilityBehaviorContext& context,
		const sas::AbilityEvent& event
	)
	{
		if (!mActive || event.eventTag != GameplayTags::Event::Combat::KillConfirmed ||
			event.GetSource<Actor>() != &context.owner)
		{
			return;
		}

		const DamageContext* damageContext = event.GetContext<DamageContext>();
		Actor* targetActor = event.GetTarget<Actor>();
		SpaceShip* targetShip = dynamic_cast<SpaceShip*>(targetActor);
		if (!damageContext || !damageContext->targetWasKilled || !targetShip ||
			targetShip == mShip ||
			!HasCollisionLayer(targetShip->GetCollisionLayer(), CollisionLayer::Enemy))
		{
			return;
		}

		++mStackCount;
		context.instance.RefreshActiveDuration(context.definition.duration);
		RefreshPowerEffect(context);
	}

	void ExecutionDriveAbility::RefreshPowerEffect(GameAbilityBehaviorContext& context)
	{
		const sas::GameplayEffectDefinition* effectDefinition =
			EffectData::FindGameplayEffectDefinition(
				AbilityData::ExecutionDrive::Effect::AttackPowerId
			);
		if (!effectDefinition)
		{
			return;
		}

		mCurrentAttackPowerBonus = std::max(
			0.f,
			mBaseAttackPowerBonus +
				mAttackPowerPerStack * static_cast<float>(mStackCount)
		);
		sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*effectDefinition);
		spec.duration = context.definition.duration;
		spec.maxStacks = 1;
		spec.modifiers = {
			sas::AttributeModifier{
				OwnerAttributeIds::AttackPower,
				sas::AttributeModifierOperation::Add,
				mCurrentAttackPowerBonus
			}
		};
		mAttackPowerEffectHandle = context.abilitySystem.ApplyGameplayEffect(
			spec,
			sas::GameplayEffectSourceContext{
				&context.owner,
				&context.instance
			}
		);
	}

	float ExecutionDriveAbility::ResolveChaseMovementMultiplier(
		const sf::Vector2f& movementDirection
	) const
	{
		if (!mActive || !mShip || !mShip->GetWorld() || !mAbilitySystem)
		{
			return 1.f;
		}

		const shared_ptr<Actor> target = targeting::FindBestOpposingDamagedShip(
			*mShip->GetWorld(),
			*mShip,
			movementDirection,
			mTargetingRange,
			mDirectionThreshold
		);
		const SpaceShip* targetShip = target
			? dynamic_cast<const SpaceShip*>(target.get())
			: nullptr;
		if (!targetShip)
		{
			return 1.f;
		}

		const float maxHealth = targetShip->GetHealthComponent().GetMaxHealth();
		if (maxHealth <= 0.f)
		{
			return 1.f;
		}

		const float missingHealthRatio = std::clamp(
			1.f - targetShip->GetHealthComponent().GetHealth() / maxHealth,
			0.f,
			1.f
		);
		const float totalAttackPower = mAbilitySystem->GetAttributes().GetCurrentValue(
			OwnerAttributeIds::AttackPower
		);
		const float externalAttackPower = std::max(
			0.f,
			totalAttackPower - mCurrentAttackPowerBonus
		);
		const float potentialBonus =
			mBaseChaseMovementBonus +
			mChaseMovementPerStack * static_cast<float>(mStackCount) +
			externalAttackPower * mAttackPowerChaseScale;
		return 1.f + std::max(0.f, potentialBonus) * missingHealthRatio;
	}

	void ExecutionDriveAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		if (!mActive)
		{
			return;
		}

		if (mAttackPowerEffectHandle.IsValid())
		{
			context.abilitySystem.RemoveGameplayEffect(mAttackPowerEffectHandle);
		}
		if (mShip)
		{
			mShip->GetRuntimeModifiers().Remove(
				AbilityData::ExecutionDrive::AbilityId::Basic
			);
		}
		context.abilitySystem.RemoveOwnedTag(AbilityData::ExecutionDrive::State::Active);
		mAttackPowerEffectHandle = {};
		mStackCount = 0;
		mCurrentAttackPowerBonus = 0.f;
		mAbilitySystem = nullptr;
		mShip = nullptr;
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
