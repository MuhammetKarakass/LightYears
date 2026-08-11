#include "gameplay/ability/overdriveCore/OverdriveCoreAbility.h"

#include "attributes/AttributeSystem.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/targeting/TargetAllocation.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "framework/Actor.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		const sas::GameplayAttribute* FindAbilityAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId
		)
		{
			return sas::FindAttribute(definition.attributes, attributeId);
		}

		bool HasExpectedScaling(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& target,
			const sas::AttributeId& source,
			sas::AttributeModifierOperation operation,
			float coefficient
		)
		{
			return std::any_of(
				definition.scalingRules.begin(),
				definition.scalingRules.end(),
				[&](const sas::AttributeScalingRule& rule)
				{
					return rule.targetAttributeId == target &&
						rule.sourceAttributeId == source &&
						rule.operation == operation &&
						std::abs(rule.coefficient - coefficient) <= 0.0001f;
				}
			);
		}

		float FindActorAttribute(
			const AbilityActorDefinition& actor,
			const sas::AttributeId& attributeId
		)
		{
			return sas::FindAttributeValue(actor.attributes, attributeId, 0.f);
		}

		CollisionLayer ResolveTargetLayer(const Actor& owner)
		{
			if (owner.GetCollisionLayer() == CollisionLayer::Player)
			{
				return CollisionLayer::Enemy;
			}
			if (owner.GetCollisionLayer() == CollisionLayer::Enemy)
			{
				return CollisionLayer::Player;
			}
			return CollisionLayer::None;
		}

		bool IsFinitePositive(float value)
		{
			return std::isfinite(value) && value > 0.f;
		}
	}

	bool OverdriveCoreAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.slot != sas::AbilitySlot::Ability4 ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 || definition.cooldown <= 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Overdrive Core requires an Ability4, pressed, one-charge duration definition.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::OverdriveCore::Attribute::ProjectileCount,
			AbilityData::OverdriveCore::Attribute::RocketLaunchDuration,
			AbilityData::OverdriveCore::Attribute::SameTargetDamageDecay,
			AbilityData::OverdriveCore::Attribute::AttackSpeedBoostDuration,
			AbilityData::OverdriveCore::Attribute::AttackSpeedBoostBase,
			AbilityData::OverdriveCore::Attribute::AttackSpeedBoostPerLevel,
			AbilityData::OverdriveCore::Attribute::AttackSpeedBoostCriticalChanceScale
		})
		{
			const sas::GameplayAttribute* attribute = FindAbilityAttribute(definition, required);
			if (!attribute || !std::isfinite(attribute->baseValue))
			{
				if (failureReason)
				{
					*failureReason =
						"Overdrive Core must declare every ability-owned runtime attribute.";
				}
				return false;
			}
		}

		const auto projectileCount = FindAbilityAttribute(
			definition,
			AbilityData::OverdriveCore::Attribute::ProjectileCount
		);
		const auto launchDuration = FindAbilityAttribute(
			definition,
			AbilityData::OverdriveCore::Attribute::RocketLaunchDuration
		);
		const auto decay = FindAbilityAttribute(
			definition,
			AbilityData::OverdriveCore::Attribute::SameTargetDamageDecay
		);
		const auto boostDuration = FindAbilityAttribute(
			definition,
			AbilityData::OverdriveCore::Attribute::AttackSpeedBoostDuration
		);
		const auto boostBase = FindAbilityAttribute(
			definition,
			AbilityData::OverdriveCore::Attribute::AttackSpeedBoostBase
		);
		const auto boostPerLevel = FindAbilityAttribute(
			definition,
			AbilityData::OverdriveCore::Attribute::AttackSpeedBoostPerLevel
		);
		const auto boostCriticalScale = FindAbilityAttribute(
			definition,
			AbilityData::OverdriveCore::Attribute::AttackSpeedBoostCriticalChanceScale
		);
		if (!projectileCount || projectileCount->baseValue < 1.f ||
			std::round(projectileCount->baseValue) != projectileCount->baseValue ||
			!IsFinitePositive(launchDuration->baseValue) ||
			decay->baseValue <= 0.f || decay->baseValue > 1.f ||
			!IsFinitePositive(boostDuration->baseValue) ||
			boostBase->baseValue < 0.f || boostPerLevel->baseValue < 0.f ||
			boostCriticalScale->baseValue < 0.f ||
			std::abs(definition.duration -
				(launchDuration->baseValue + boostDuration->baseValue)) > 0.0001f)
		{
			if (failureReason)
			{
				*failureReason =
					"Overdrive Core has invalid projectile, decay, lifecycle, or boost values.";
			}
			return false;
		}

		if (!HasExpectedScaling(
			definition,
			CommonAttributeIds::ProjectileCount,
			OwnerAttributeIds::AttackSpeed,
			sas::AttributeModifierOperation::Multiply,
			0.5f
		) || !HasExpectedScaling(
			definition,
			CommonAttributeIds::Damage,
			OwnerAttributeIds::AttackPower,
			sas::AttributeModifierOperation::Add,
			0.5f
		))
		{
			if (failureReason)
			{
				*failureReason =
					"Overdrive Core requires ProjectileCount from AttackSpeed and rocket damage from AttackPower.";
			}
			return false;
		}

		const sas::GameplayEffectDefinition* boostEffect =
			EffectData::FindGameplayEffectDefinition(
				AbilityData::OverdriveCore::Effect::AttackSpeedBoostId
			);
		const AbilityActorDefinition* rocketActor =
			AbilityData::FindAbilityActorDefinition(
				AbilityData::OverdriveCore::Actor::Projectile::BasicDefinitionId
			);
		if (!boostEffect || boostEffect->durationPolicy != sas::GameplayEffectDurationPolicy::Duration ||
			!rocketActor ||
			!IsFinitePositive(FindActorAttribute(*rocketActor, CommonAttributeIds::Damage)) ||
			!IsFinitePositive(FindActorAttribute(*rocketActor, CommonAttributeIds::Range)) ||
			!IsFinitePositive(FindActorAttribute(
				*rocketActor,
				AbilityData::OverdriveCore::Actor::Projectile::ProjectileSpeed
			)))
		{
			if (failureReason)
			{
				*failureReason =
					"Overdrive Core requires its projectile actor and attack-speed effect contracts.";
			}
			return false;
		}

		return true;
	}

	bool OverdriveCoreAbility::Activate(GameAbilityBehaviorContext& context)
	{
		ExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const sas::GameplayAttributeList abilityValues =
			AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		mProjectileCount = static_cast<std::size_t>(std::max(
			1.f,
			std::round(sas::FindAttributeValue(
				abilityValues,
				AbilityData::OverdriveCore::Attribute::ProjectileCount,
				1.f
			))
		));
		mRocketLaunchDuration = sas::FindAttributeValue(
			abilityValues,
			AbilityData::OverdriveCore::Attribute::RocketLaunchDuration,
			0.f
		);
		mSameTargetDamageDecay = std::clamp(
			sas::FindAttributeValue(
				abilityValues,
				AbilityData::OverdriveCore::Attribute::SameTargetDamageDecay,
				1.f
			),
			0.f,
			1.f
		);
		mLaunchedProjectileCount = 0;
		mRocketLaunchElapsed = 0.f;
		mSameTargetRocketCounts.clear();
		mTargetAllocation.clear();

		const AbilityActorDefinition* rocketActor =
			AbilityData::FindAbilityActorDefinition(
				AbilityData::OverdriveCore::Actor::Projectile::BasicDefinitionId
			);
		World* world = context.owner.GetWorld();
		if (!rocketActor || !world)
		{
			return false;
		}

		const sas::GameplayAttributeList rocketValues =
			AbilityActionAttributeResolver::ResolveAttributes(
				executionContext,
				nullptr,
				rocketActor->attributes,
				context.instance.GetResolvedDamageTags(AttachmentHostKind::Ability)
			);
		const float targetRange = sas::FindAttributeValue(
			rocketValues,
			CommonAttributeIds::Range,
			0.f
		);
		if (targetRange <= 0.f)
		{
			return false;
		}

		targeting::TargetingQuery query;
		query.source = &context.owner;
		query.origin = context.owner.GetActorLocation();
		query.range = targetRange;
		query.requiredTargetLayers = ResolveTargetLayer(context.owner);
		query.requireCollisionCompatibility = true;
		const List<targeting::TargetingCandidate> targets =
			targeting::AutoTargeting::FindTargets(*world, query);
		mTargetAllocation = targeting::TargetAllocation::DistributeEvenly(
			targets,
			mProjectileCount
		);

		context.abilitySystem.AddOwnedTag(
			GameplayTagSchema::BlockAbilityActivation
		);
		context.abilitySystem.AddOwnedTag(
			GameplayTagSchema::BlockPrimaryWeaponFire
		);
		context.abilitySystem.AddOwnedTag(
			AbilityData::OverdriveCore::State::RocketLaunchActive
		);
		mRocketLaunchActive = true;
		EmitEvent(context, AbilityData::OverdriveCore::Event::RocketLaunchStarted);

		// The first rocket is immediate; the remaining rockets are spread over
		// the configured launch window by Tick().
		LaunchRocket(context, executionContext);
		return true;
	}

	void OverdriveCoreAbility::Tick(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		if (!mRocketLaunchActive)
		{
			return;
		}

		ExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		mRocketLaunchElapsed += std::max(0.f, deltaTime);
		const float interval = mRocketLaunchDuration /
			static_cast<float>(std::max<std::size_t>(1, mProjectileCount));
		while (mLaunchedProjectileCount < mProjectileCount &&
			mRocketLaunchElapsed >= interval * static_cast<float>(mLaunchedProjectileCount))
		{
			LaunchRocket(context, executionContext);
		}

		if (mRocketLaunchElapsed >= mRocketLaunchDuration &&
			mLaunchedProjectileCount >= mProjectileCount)
		{
			FinishRocketLaunch(context, executionContext);
		}
	}

	void OverdriveCoreAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason
	)
	{
		if (mRocketLaunchActive)
		{
			ClearRocketLaunchState(context);
			EmitEvent(context, AbilityData::OverdriveCore::Event::RocketLaunchEnded);
		}

		if (mAttackSpeedBoostActive)
		{
			if (mAttackSpeedBoostEffectHandle.IsValid())
			{
				context.abilitySystem.RemoveGameplayEffect(mAttackSpeedBoostEffectHandle);
				mAttackSpeedBoostEffectHandle = {};
			}
			context.abilitySystem.RemoveOwnedTag(
				AbilityData::OverdriveCore::State::AttackSpeedBoostActive
			);
			EmitEvent(context, AbilityData::OverdriveCore::Event::AttackSpeedBoostEnded);
			mAttackSpeedBoostActive = false;
		}

		mTargetAllocation.clear();
		mSameTargetRocketCounts.clear();
		mProjectileCount = 0;
		mLaunchedProjectileCount = 0;
	}

	void OverdriveCoreAbility::LaunchRocket(
		GameAbilityBehaviorContext& context,
		ExecutionContext& executionContext
	)
	{
		if (mLaunchedProjectileCount >= mProjectileCount)
		{
			return;
		}

		if (mLaunchedProjectileCount < mTargetAllocation.size())
		{
			const shared_ptr<Actor> target =
				mTargetAllocation[mLaunchedProjectileCount].lock();
			if (target && !target->GetIsPendingDestroy())
			{
				sf::Vector2f direction =
					target->GetActorLocation() - context.owner.GetActorLocation();
				if (GetVectorLength(direction) <= 0.001f)
				{
					direction = context.owner.GetActorForwardDirection();
				}
				else
				{
					NormalizeVector(direction);
				}

				const std::size_t sameTargetIndex =
					mSameTargetRocketCounts[target.get()]++;
				const float damageMultiplier = std::pow(
					mSameTargetDamageDecay,
					static_cast<float>(sameTargetIndex)
				);
				AbilityActorSpawner::SpawnToTarget(
					AbilityData::OverdriveCore::Actor::Projectile::BasicDefinitionId,
					executionContext,
					context.owner,
					direction,
					target->GetActorLocation(),
					damageMultiplier,
					target.get()
				);
			}
		}

		++mLaunchedProjectileCount;
	}

	void OverdriveCoreAbility::FinishRocketLaunch(
		GameAbilityBehaviorContext& context,
		ExecutionContext& executionContext
	)
	{
		ClearRocketLaunchState(context);
		EmitEvent(context, AbilityData::OverdriveCore::Event::RocketLaunchEnded);

		const sas::GameplayEffectDefinition* effectDefinition =
			EffectData::FindGameplayEffectDefinition(
				AbilityData::OverdriveCore::Effect::AttackSpeedBoostId
			);
		if (!effectDefinition)
		{
			return;
		}

		const sas::GameplayAttributeList abilityValues =
			AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		const float boostDuration = sas::FindAttributeValue(
			abilityValues,
			AbilityData::OverdriveCore::Attribute::AttackSpeedBoostDuration,
			0.f
		);
		const float boostBase = sas::FindAttributeValue(
			abilityValues,
			AbilityData::OverdriveCore::Attribute::AttackSpeedBoostBase,
			0.f
		);
		const float boostPerLevel = sas::FindAttributeValue(
			abilityValues,
			AbilityData::OverdriveCore::Attribute::AttackSpeedBoostPerLevel,
			0.f
		);
		const float criticalScale = sas::FindAttributeValue(
			abilityValues,
			AbilityData::OverdriveCore::Attribute::AttackSpeedBoostCriticalChanceScale,
			0.f
		);
		const float criticalChance = context.abilitySystem.GetAttributes().GetCurrentValue(
			OwnerAttributeIds::CriticalChance
		);
		const float boostMagnitude = std::max(
			0.f,
			(boostBase + boostPerLevel * static_cast<float>(
				std::max(0, context.instance.GetLevel() - 1)
			)) * (1.f + std::max(0.f, criticalChance) * criticalScale)
		);
		if (boostDuration <= 0.f || boostMagnitude <= 0.f)
		{
			return;
		}

		sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*effectDefinition);
		spec.duration = boostDuration;
		spec.maxStacks = 1;
		spec.modifiers = {
			sas::AttributeModifier{
				OwnerAttributeIds::AttackSpeed,
				sas::AttributeModifierOperation::Add,
				boostMagnitude
			}
		};
		mAttackSpeedBoostEffectHandle = context.abilitySystem.ApplyGameplayEffect(spec);
		if (!mAttackSpeedBoostEffectHandle.IsValid())
		{
			return;
		}

		mAttackSpeedBoostActive = true;
		context.abilitySystem.AddOwnedTag(
			AbilityData::OverdriveCore::State::AttackSpeedBoostActive
		);
		EmitEvent(context, AbilityData::OverdriveCore::Event::AttackSpeedBoostStarted);
	}

	void OverdriveCoreAbility::EmitEvent(
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

	void OverdriveCoreAbility::ClearRocketLaunchState(
		GameAbilityBehaviorContext& context
	)
	{
		if (!mRocketLaunchActive)
		{
			return;
		}
		context.abilitySystem.RemoveOwnedTag(
			GameplayTagSchema::BlockAbilityActivation
		);
		context.abilitySystem.RemoveOwnedTag(
			GameplayTagSchema::BlockPrimaryWeaponFire
		);
		context.abilitySystem.RemoveOwnedTag(
			AbilityData::OverdriveCore::State::RocketLaunchActive
		);
		mRocketLaunchActive = false;
	}
}
