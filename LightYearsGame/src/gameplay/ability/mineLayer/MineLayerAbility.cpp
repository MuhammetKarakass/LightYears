#include "gameplay/ability/mineLayer/MineLayerAbility.h"

#include "attributes/AttributeMath.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/mineLayer/MineLayerContracts.h"
#include "gameplay/ability/mineLayer/MineLayerMineActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "framework/Actor.h"

#include <algorithm>
#include <cmath>
#include <optional>

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

		bool HasExpectedModifier(
			const AbilityLevelStep& step,
			const sas::AttributeId& attributeId,
			float magnitude
		)
		{
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == attributeId &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					std::abs(modifier.magnitude - magnitude) <= 0.0001f)
				{
					return true;
				}
			}
			return false;
		}

		std::optional<float> FindModifierMagnitude(
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

	bool MineLayerAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::MineLayer::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Instant ||
			definition.maxCharges != 1 || definition.cooldown <= 0.f ||
			definition.duration != 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Mine Layer requires a loadout slot, pressed activation, one charge and instant lifetime.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::MineLayer::Attribute::BaseMineCount,
			AbilityData::MineLayer::Attribute::MineSpacing,
			AbilityData::MineLayer::Attribute::LuckToBonusMineScale
		})
		{
			const sas::GameplayAttribute* attribute = FindAttribute(definition, required);
			if (!attribute || !std::isfinite(attribute->baseValue))
			{
				if (failureReason)
				{
					*failureReason =
						"Mine Layer must declare all ability-owned placement and Luck attributes.";
				}
				return false;
			}
		}

		const float baseMineCount = FindAttribute(
			definition,
			AbilityData::MineLayer::Attribute::BaseMineCount
		)->baseValue;
		if (baseMineCount < 1.f || std::round(baseMineCount) != baseMineCount ||
			FindAttribute(definition, AbilityData::MineLayer::Attribute::MineSpacing)->baseValue <= 0.f ||
			FindAttribute(definition, AbilityData::MineLayer::Attribute::LuckToBonusMineScale)->baseValue < 0.f)
		{
			if (failureReason)
			{
				*failureReason = "Mine Layer has invalid mine count, spacing or Luck scaling.";
			}
			return false;
		}

		if (definition.damageTags.size() != 1 ||
			definition.damageTags.front() != DamageTypeSchema::Energy ||
			definition.scalingRules.size() != 1 ||
			definition.scalingRules.front().targetAttributeId != CommonAttributeIds::Damage ||
			definition.scalingRules.front().sourceAttributeId != OwnerAttributeIds::AttackPower ||
			definition.scalingRules.front().operation != sas::AttributeModifierOperation::Add ||
			std::abs(definition.scalingRules.front().coefficient - 0.75f) > 0.0001f)
		{
			if (failureReason)
			{
				*failureReason = "Mine Layer requires Energy damage and AttackPower x0.75 scaling.";
			}
			return false;
		}

		const AbilityActorDefinition* mineDefinition =
			AbilityData::FindAbilityActorDefinition(
				AbilityData::MineLayer::Actor::Mine::BasicDefinitionId
			);
		if (!mineDefinition)
		{
			if (failureReason)
			{
				*failureReason = "Mine Layer requires a registered mine actor definition.";
			}
			return false;
		}
		const AbilityActorValidationResult actorValidation =
			AbilityActorRegistry::ValidateDefinition(*mineDefinition);
		if (!actorValidation.isValid)
		{
			if (failureReason)
			{
				*failureReason = actorValidation.reason;
			}
			return false;
		}

		const sas::GameplayEffectDefinition* stunDefinition =
			EffectData::FindGameplayEffectDefinition(AbilityData::MineLayer::Effect::StunId);
		if (!stunDefinition ||
			stunDefinition->durationPolicy != sas::GameplayEffectDurationPolicy::Duration)
		{
			if (failureReason)
			{
				*failureReason = "Mine Layer requires the reusable project-wide Stun effect.";
			}
			return false;
		}

		if (definition.levelProgression.size() != 14)
		{
			if (failureReason)
			{
				*failureReason = "Mine Layer requires fourteen progression steps through level fifteen.";
			}
			return false;
		}
		const std::optional<float> damagePerLevel = FindModifierMagnitude(
			definition.levelProgression.front(),
			CommonAttributeIds::Damage
		);
		const std::optional<float> cooldownReductionPerLevel = FindModifierMagnitude(
			definition.levelProgression.front(),
			CommonAttributeIds::Cooldown
		);
		if (!damagePerLevel || !cooldownReductionPerLevel || *damagePerLevel <= 0.f ||
			*cooldownReductionPerLevel >= 0.f)
		{
			if (failureReason)
			{
				*failureReason = "Mine Layer progression must add damage and reduce cooldown.";
			}
			return false;
		}

		float resolvedCooldown = definition.cooldown;
		for (const AbilityLevelStep& step : definition.levelProgression)
		{
			if (step.attributeModifiers.size() != 2 ||
				!HasExpectedModifier(step, CommonAttributeIds::Damage, *damagePerLevel) ||
				!HasExpectedModifier(step, CommonAttributeIds::Cooldown, *cooldownReductionPerLevel))
			{
				if (failureReason)
				{
					*failureReason = "Mine Layer progression may only increase damage and reduce cooldown.";
				}
				return false;
			}
			resolvedCooldown += *cooldownReductionPerLevel;
			if (resolvedCooldown <= 0.f)
			{
				if (failureReason)
				{
					*failureReason = "Mine Layer progression must keep cooldown positive at every level.";
				}
				return false;
			}
		}

		return true;
	}

	bool MineLayerAbility::Activate(GameAbilityBehaviorContext& context)
	{
		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const sas::GameplayAttributeList values =
			AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		const int baseMineCount = std::max(
			1,
			static_cast<int>(std::round(FindValue(
				values,
				AbilityData::MineLayer::Attribute::BaseMineCount,
				3.f
			)))
		);
		const float mineSpacing = std::max(
			1.f,
			FindValue(values, AbilityData::MineLayer::Attribute::MineSpacing, 120.f)
		);
		const float luckScale = std::max(
			0.f,
			FindValue(values, AbilityData::MineLayer::Attribute::LuckToBonusMineScale, 1.f)
		);

		float luckRating = 0.f;
		if (const Combatant* combatant = dynamic_cast<const Combatant*>(&context.owner))
		{
			luckRating = std::max(
				0.f,
				combatant->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
					OwnerAttributeIds::Luck
				)
			);
		}
		const float bonusMineValue = luckRating * luckScale;
		const int guaranteedBonusMines = static_cast<int>(std::floor(bonusMineValue));
		const float fractionalBonus = bonusMineValue - static_cast<float>(guaranteedBonusMines);
		const int bonusMines = guaranteedBonusMines +
			(RandRange(0.f, 1.f) < fractionalBonus ? 1 : 0);
		const int mineCount = baseMineCount + std::max(0, bonusMines);

		// Mine placement is based on the ship's facing, not its velocity. A ship
		// may be drifting sideways while looking elsewhere, but the mine field must
		// still appear behind the nose direction the player sees.
		const sf::Vector2f forward = context.owner.GetActorForwardDirection();
		const sf::Vector2f right = context.owner.GetActorRightDirection();
		const sf::Vector2f formationOrigin =
			context.owner.GetActorLocation() - forward * mineSpacing;
		// MineSpacing is the formation unit. The first row is the origin; each
		// following row moves one third of that unit toward the ship and places
		// mines side by side. With S=120, row 1 is (-80,+40),(+80,+40), exactly
		// matching the requested local XY layout around (0,0).
		const float rowDepth = mineSpacing / 3.f;
		const float lateralStep = mineSpacing * (4.f / 3.f);
		const SpawnActorAction spawnAction{
			AbilityData::MineLayer::Actor::Mine::BasicDefinitionId,
			sas::AbilitySpawnPolicy::AtOwner,
			sas::AbilityDirectionPolicy::OwnerForward
		};
		int spawnedMineCount = 0;
		for (int mineIndex = 0; mineIndex < mineCount; ++mineIndex)
		{
			const weak_ptr<AbilityWorldActor> spawned = AbilityActorSpawner::Spawn(
				spawnAction,
				executionContext,
				context.owner
			);
			if (const shared_ptr<AbilityWorldActor> mine = spawned.lock())
			{
				// Triangular row packing keeps bonus mines from becoming another
				// single-file trail: row r contains r+1 mines. The row start is the
				// r-th triangular number, so the formula works for any Luck-derived
				// mine count without adding a gameplay hard cap.
				int row = 0;
				int rowStart = 0;
				while (mineIndex >= rowStart + row + 1)
				{
					rowStart += row + 1;
					++row;
				}
				const int column = mineIndex - rowStart;
				const float localX =
					(static_cast<float>(column) - static_cast<float>(row) * 0.5f) *
					lateralStep;
				const float localForward = static_cast<float>(row) * rowDepth;

				const sf::Vector2f targetLocation =
					formationOrigin + right * localX + forward * localForward;
				if (MineLayerMineActor* mineActor =
					dynamic_cast<MineLayerMineActor*>(mine.get()))
				{
					// Spawn stays at the ship; the mine itself performs the visible
					// deployment flight instead of appearing at targetLocation.
					mineActor->LaunchTo(targetLocation);
				}
				++spawnedMineCount;
			}
		}

		return spawnedMineCount > 0;
	}
}
