#include "gameplay/ability/shieldHarvest/ShieldHarvestAbility.h"

#include "gameplay/ability/runtime/FocusActionLocks.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/ability/shieldHarvest/ShieldHarvestContracts.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/portal/PortalTransferParticipant.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/damage/DamageContext.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/shieldHarvest/ShieldHarvestPresentationIds.h"
#include "presentation/ability/shieldHarvest/ShieldHarvestPresentationProfile.h"
#include "spaceShip/SpaceShip.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>

namespace ly
{
namespace
	{
		constexpr float BaseCooldown = 14.f;
		constexpr float FocusDuration = 1.5f;
		constexpr float PulseStunDuration = 0.25f;
		constexpr float BaseRadius = 700.f;
		constexpr float BaseShieldPerEnemy = 20.f;
		constexpr float BasePulseDamage = 10.f;
		constexpr float BaseOvershieldHoldDuration = 5.f;
		constexpr float BaseOvershieldDecayPerSecond = 100.f;
		constexpr float BaseEnergyPowerShieldScale = 0.10f;
		constexpr float EnergyPowerShieldScalePerLevel = 0.01f;
		constexpr float EnergyPowerPulseDamageScale = 0.05f;
		constexpr float ShieldPerEnemyPerLevel = 5.f;
		constexpr float PulseDamagePerLevel = 2.f;

		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& attributeId,
			float fallback
		)
		{
			return sas::FindAttributeValue(values, attributeId, fallback);
		}

		sas::GameplayAttributeList ResolveValues(
			GameAbilityBehaviorContext& context
		)
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

		bool NearlyEqual(float left, float right)
		{
			return std::isfinite(left) && std::isfinite(right) &&
				std::abs(left - right) <= 0.0001f;
		}

		bool HasExpectedScalingRule(
			const List<sas::AttributeScalingRule>& scalingRules,
			const sas::AttributeId& targetAttributeId,
			const sas::AttributeId& sourceAttributeId,
			float expectedCoefficient
		)
		{
			return std::any_of(
				scalingRules.begin(),
				scalingRules.end(),
				[&](const sas::AttributeScalingRule& scalingRule)
				{
					return scalingRule.targetAttributeId == targetAttributeId &&
						scalingRule.sourceAttributeId == sourceAttributeId &&
						scalingRule.operation == sas::AttributeModifierOperation::Add &&
						NearlyEqual(scalingRule.coefficient, expectedCoefficient);
				}
			);
		}

		bool HasExpectedModifier(
			const AbilityLevelStep& step,
			const sas::AttributeId& attributeId,
			float magnitude
		)
		{
			return std::any_of(step.attributeModifiers.begin(), step.attributeModifiers.end(),
				[&](const sas::AttributeModifier& modifier)
				{
					return modifier.attributeId == attributeId &&
						modifier.operation == sas::AttributeModifierOperation::Add &&
						NearlyEqual(modifier.magnitude, magnitude);
				});
		}

		bool HasExpectedDamageTags(const GameAbilityDefinition& definition)
		{
			return definition.damageTags.size() == 1 &&
				definition.damageTags[0].MatchesTagExact(DamageTypeSchema::Energy);
		}
	}

	bool ShieldHarvestAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::ShieldHarvest::AbilityId::Basic ||
			definition.behaviorType != AbilityBehaviorType::ShieldHarvest ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 ||
			!NearlyEqual(definition.cooldown, BaseCooldown) ||
			!NearlyEqual(definition.duration, FocusDuration) ||
			!HasExpectedDamageTags(definition))
		{
			if (failureReason)
			{
				*failureReason =
					"Shield Harvest requires its authored identity, loadout lifecycle, duration, and Energy damage tag.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::ShieldHarvest::Attribute::Radius,
			AbilityData::ShieldHarvest::Attribute::ShieldPerEnemy,
			AbilityData::ShieldHarvest::Attribute::OvershieldHoldDuration,
			AbilityData::ShieldHarvest::Attribute::OvershieldDecayPerSecond,
			CommonAttributeIds::Damage
		})
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes,
				required
			);
			if (!attribute || !std::isfinite(attribute->baseValue))
			{
				if (failureReason)
				{
					*failureReason = "Shield Harvest must declare all runtime attributes.";
				}
				return false;
			}
		}

		const sas::GameplayAttribute* radius = sas::FindAttribute(
			definition.attributes,
			AbilityData::ShieldHarvest::Attribute::Radius
		);
		const sas::GameplayAttribute* shieldPerEnemy = sas::FindAttribute(
			definition.attributes,
			AbilityData::ShieldHarvest::Attribute::ShieldPerEnemy
		);
		const sas::GameplayAttribute* holdDuration = sas::FindAttribute(
			definition.attributes,
			AbilityData::ShieldHarvest::Attribute::OvershieldHoldDuration
		);
		const sas::GameplayAttribute* decayPerSecond = sas::FindAttribute(
			definition.attributes,
			AbilityData::ShieldHarvest::Attribute::OvershieldDecayPerSecond
		);
		const sas::GameplayAttribute* pulseDamage = sas::FindAttribute(
			definition.attributes,
			CommonAttributeIds::Damage
		);
		if (!NearlyEqual(radius->baseValue, BaseRadius) ||
			!NearlyEqual(shieldPerEnemy->baseValue, BaseShieldPerEnemy) ||
			!NearlyEqual(pulseDamage->baseValue, BasePulseDamage) ||
			!NearlyEqual(holdDuration->baseValue, BaseOvershieldHoldDuration) ||
			!NearlyEqual(decayPerSecond->baseValue, BaseOvershieldDecayPerSecond) ||
			!IsFiniteNonNegative(holdDuration->baseValue) ||
			!IsFiniteNonNegative(decayPerSecond->baseValue))
		{
			if (failureReason)
			{
				*failureReason = "Shield Harvest contains an invalid radius or overshield value.";
			}
			return false;
		}
		if (definition.scalingRules.size() != 2 ||
			!HasExpectedScalingRule(
				definition.scalingRules,
				AbilityData::ShieldHarvest::Attribute::ShieldPerEnemy,
				OwnerAttributeIds::EnergyPower,
				BaseEnergyPowerShieldScale
			) ||
			!HasExpectedScalingRule(
				definition.scalingRules,
				CommonAttributeIds::Damage,
				OwnerAttributeIds::EnergyPower,
				EnergyPowerPulseDamageScale
			))
		{
			if (failureReason)
			{
				*failureReason = "Shield Harvest requires its authored Energy Power shield and damage scaling rules.";
			}
			return false;
		}

		if (definition.levelProgression.empty() && definition.repeatingLevelProgression.empty())
		{
			if (failureReason)
			{
				*failureReason = "Shield Harvest requires at least one level progression step.";
			}
			return false;
		}
		List<AbilityLevelStep> levelSteps = definition.levelProgression;
		levelSteps.insert(levelSteps.end(), definition.repeatingLevelProgression.begin(),
			definition.repeatingLevelProgression.end());
		for (const AbilityLevelStep& step : levelSteps)
		{
			if (step.attributeModifiers.size() != 2 ||
				!HasExpectedModifier(step, AbilityData::ShieldHarvest::Attribute::ShieldPerEnemy, ShieldPerEnemyPerLevel) ||
				!HasExpectedModifier(step, CommonAttributeIds::Damage, PulseDamagePerLevel) ||
				!HasExpectedScalingRule(
					step.scalingRules,
					AbilityData::ShieldHarvest::Attribute::ShieldPerEnemy,
					OwnerAttributeIds::EnergyPower,
					EnergyPowerShieldScalePerLevel
				) ||
				step.scalingRules.size() != 1)
			{
				if (failureReason)
				{
					*failureReason = "Shield Harvest progression requires shield, and pulse damage gains.";
				}
				return false;
			}
		}
		return true;
	}

	bool ShieldHarvestAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (!dynamic_cast<SpaceShip*>(&context.owner) || mHarvested)
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		const float radius = std::max(
			1.f,
			FindValue(values, AbilityData::ShieldHarvest::Attribute::Radius, 700.f)
		);
		const float focusDuration = std::max(0.001f, context.definition.duration);
		mFocusElapsed = 0.f;
		mHarvested = false;
		ability::ApplyFocusActionLocks(context.abilitySystem, false);
		context.abilitySystem.AddOwnedTag(AbilityData::ShieldHarvest::State::Focusing);

		if (World* world = context.owner.GetWorld())
		{
			if (const ShieldHarvestPresentationProfile* profile =
				PresentationProfileRegistry<ShieldHarvestPresentationProfile>::Find(
					ShieldHarvestPresentationIds::FocusBasic
				))
			{
				mTelegraph = world->SpawnActor<AreaTelegraphActor>(
					AreaTelegraphActor::SpawnParams{
						context.owner.GetActorLocation(),
						radius,
						0.f,
						profile->focusTelegraph,
						AreaTelegraphAnchorMode::FollowActor,
						AreaTelegraphProgressDriver::External,
						&context.owner
					}
				);
			}
		}

		EmitEvent(context, AbilityData::ShieldHarvest::Event::Started);
		return true;
	}

	void ShieldHarvestAbility::Tick(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		if (mHarvested)
		{
			return;
		}
		if (const auto* participant = dynamic_cast<const PortalTransferParticipant*>(
			&context.owner); participant && participant->IsInPortalTransit())
		{
			// The enemy count is a single completion-edge snapshot. Pause the
			// focus rather than resolving it from the hidden entrance position.
			return;
		}

		mFocusElapsed += std::max(0.f, deltaTime);
		const float focusDuration = std::max(0.001f, context.definition.duration);
		const float progress = std::clamp(mFocusElapsed / focusDuration, 0.f, 1.f);
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->SetExternalProgress(progress);
		}

		if (progress < 1.f)
		{
			return;
		}

		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		World* world = context.owner.GetWorld();
		if (!ship || !world)
		{
			return;
		}

		// The query is intentionally executed once at the completion edge. Enemies
		// entering or leaving the area during overshield hold do not change the grant.
		const sas::GameplayAttributeList values = ResolveValues(context);
		const float radius = std::max(
			1.f,
			FindValue(values, AbilityData::ShieldHarvest::Attribute::Radius, 700.f)
		);
		const float shieldPerEnemy = std::max(
			0.f,
			FindValue(values, AbilityData::ShieldHarvest::Attribute::ShieldPerEnemy, 40.f)
		);
		const float holdDuration = std::max(
			0.f,
			FindValue(values, AbilityData::ShieldHarvest::Attribute::OvershieldHoldDuration, 5.f)
		);
		const float decayPerSecond = std::max(
			0.f,
			FindValue(values, AbilityData::ShieldHarvest::Attribute::OvershieldDecayPerSecond, 100.f)
		);
		const float pulseDamage = std::max(0.f,
			FindValue(values, CommonAttributeIds::Damage, BasePulseDamage));
		const List<shared_ptr<Actor>> targets = targeting::FindOpposingCombatants(
			*world, context.owner, context.owner.GetActorLocation(), radius);
		std::size_t harvestedCount = 0;
		for (const shared_ptr<Actor>& target : targets)
		{
			if (!target || target->GetIsPendingDestroy())
			{
				continue;
			}
			++harvestedCount;
			ApplyCombatDamage(*target, pulseDamage, &context.owner,
				{ DamageTypeSchema::Energy }, DamagePayload{},
				sas::ContentId{ context.definition.abilityId }, context.definition.abilityTags);
			if (auto* combatant = dynamic_cast<Combatant*>(target.get()))
			{
				ApplyStun(context, *combatant, PulseStunDuration);
			}
		}
		ship->GetShieldComponent().GrantTemporaryOvershield(
			context.definition.abilityId,
			static_cast<float>(harvestedCount) * shieldPerEnemy,
			holdDuration,
			decayPerSecond
		);
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			// Keep the completed area visible briefly so the player receives
			// explicit feedback that the enemy count was captured.
			telegraph->Complete();
		}
		mHarvested = true;
		EmitEvent(context, AbilityData::ShieldHarvest::Event::Harvested);
	}

	void ShieldHarvestAbility::ApplyStun(
		GameAbilityBehaviorContext& context,
		Combatant& target,
		float duration
	) const
	{
		if (duration <= 0.f)
		{
			return;
		}
		const ControlResponse response = target.ResolveControlResponse(
			GameplayTags::State::Effect::Control::Stunned);
		if (response.mode == ControlResponseMode::Immune ||
			response.mode == ControlResponseMode::InterruptOnly)
		{
			return;
		}
		const sas::GameplayEffectDefinition* definition =
			EffectData::FindGameplayEffectDefinition(AbilityData::ShieldHarvest::Effect::StunId);
		if (!definition)
		{
			return;
		}
		sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*definition);
		spec.duration = duration * std::max(0.f, response.durationMultiplier);
		spec.maxStacks = 1;
		if (spec.duration > 0.f)
		{
			target.GetAbilitySystemComponent().ApplyGameplayEffect(spec,
				sas::GameplayEffectSourceContext{ &context.owner, &context.instance });
		}
	}

	void ShieldHarvestAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		ability::RemoveFocusActionLocks(context.abilitySystem, false);
		context.abilitySystem.RemoveOwnedTag(AbilityData::ShieldHarvest::State::Focusing);
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			if (!telegraph->IsInCompletionFeedback())
			{
				telegraph->Destroy();
			}
		}
		mTelegraph.reset();
		mFocusElapsed = 0.f;
		mHarvested = false;
		EmitEvent(context, AbilityData::ShieldHarvest::Event::Ended);
	}

	void ShieldHarvestAbility::EmitEvent(
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
