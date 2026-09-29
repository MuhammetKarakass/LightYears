#include "gameplay/ability/shieldGraft/ShieldGraftAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/shieldGraft/ShieldGraftContracts.h"
#include "gameplay/ability/shieldGraft/ShieldGraftVisualActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/shieldGraft/ShieldGraftPresentationIds.h"
#include "presentation/ability/shieldGraft/ShieldGraftPresentationProfile.h"
#include "spaceShip/SpaceShip.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
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

		bool IsFinitePositive(float value)
		{
			return std::isfinite(value) && value > 0.f;
		}

		bool IsFiniteNonNegative(float value)
		{
			return std::isfinite(value) && value >= 0.f;
		}
	}

	bool ShieldGraftAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::ShieldGraft::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Instant ||
			definition.maxCharges != 1 ||
			!std::isfinite(definition.cooldown) || definition.cooldown <= 0.f ||
			definition.duration != 0.f ||
			definition.behaviorType != AbilityBehaviorType::ShieldGraft)
		{
			if (failureReason)
			{
				*failureReason =
					"Shield Graft requires a loadout slot, pressed activation, instant lifetime, one charge, and positive cooldown.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::ShieldGraft::Attribute::ConversionRatio,
			AbilityData::ShieldGraft::Attribute::EnergyPowerReference,
			AbilityData::ShieldGraft::Attribute::EnergyPowerScale
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
					*failureReason = "Shield Graft must declare all runtime attributes.";
				}
				return false;
			}
		}

		const sas::GameplayAttribute* conversion = sas::FindAttribute(
			definition.attributes,
			AbilityData::ShieldGraft::Attribute::ConversionRatio
		);
		const sas::GameplayAttribute* energyRef = sas::FindAttribute(
			definition.attributes,
			AbilityData::ShieldGraft::Attribute::EnergyPowerReference
		);
		const sas::GameplayAttribute* energyScale = sas::FindAttribute(
			definition.attributes,
			AbilityData::ShieldGraft::Attribute::EnergyPowerScale
		);

		if (!IsFinitePositive(conversion->baseValue) ||
			!IsFinitePositive(energyRef->baseValue) ||
			!IsFiniteNonNegative(energyScale->baseValue))
		{
			if (failureReason)
			{
				*failureReason = "Shield Graft contains invalid conversion or energy scaling attributes.";
			}
			return false;
		}

		std::size_t progressionIndex = 0;
		const bool validProgression = definition.levelProgression.size() == 14 &&
			std::all_of(definition.levelProgression.begin(), definition.levelProgression.end(),
				[&definition, &progressionIndex](const AbilityLevelStep& step)
				{
					const std::size_t index = progressionIndex++;
					if (step.attributeModifiers.size() != 3 || !step.scalingRules.empty()) return false;
					const auto has = [&step](const sas::AttributeId& id, float magnitude)
					{
						return std::any_of(step.attributeModifiers.begin(), step.attributeModifiers.end(),
							[&id, magnitude](const sas::AttributeModifier& modifier)
							{
								return modifier.attributeId == id &&
									modifier.operation == sas::AttributeModifierOperation::Add &&
									std::abs(modifier.magnitude - magnitude) <= 0.0001f;
							});
					};
					return has(AbilityData::ShieldGraft::Attribute::ConversionRatio, 0.015f) &&
						has(AbilityData::ShieldGraft::Attribute::EnergyPowerScale, 0.01f) &&
						has(CommonAttributeIds::Cooldown, -GetGlobalAbilityCooldownStepReduction(definition.cooldown, index));
				});
		if (definition.attributes.size() != 3 || !validProgression)
		{
			if (failureReason)
			{
				*failureReason = "Shield Graft requires three attributes and fourteen conversion progression steps.";
			}
			return false;
		}

		return true;
	}

	bool ShieldGraftAbility::Activate(GameAbilityBehaviorContext& context)
	{
		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		if (!ship || ship->GetIsPendingDestroy())
		{
			return false;
		}

		const float currentHealth = ship->GetHealthComponent().GetHealth();
		const float maxHealth = ship->GetHealthComponent().GetMaxHealth();
		// Living owner, current health < max health required before cooldown/use-history
		if (!std::isfinite(currentHealth) || currentHealth <= 0.f ||
			!std::isfinite(maxHealth) || currentHealth >= maxHealth)
		{
			return false;
		}

		const float currentShield = ship->GetShieldComponent().GetShield();
		// Current shield > 0 required before any cooldown/use-history
		if (!std::isfinite(currentShield) || currentShield <= 0.f)
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		const float baseConversion = FindValue(
			values,
			AbilityData::ShieldGraft::Attribute::ConversionRatio,
			0.55f
		);

		float ownerEnergyPower = 0.f;
		if (context.abilitySystem.GetAttributes().HasAttribute(OwnerAttributeIds::EnergyPower))
		{
			ownerEnergyPower = context.abilitySystem.GetAttributes().GetCurrentValue(
				OwnerAttributeIds::EnergyPower
			);
		}
		if (!std::isfinite(ownerEnergyPower) || ownerEnergyPower < 0.f)
		{
			ownerEnergyPower = 0.f;
		}

		const float energyReference = FindValue(
			values,
			AbilityData::ShieldGraft::Attribute::EnergyPowerReference,
			100.f
		);
		const float energyScale = FindValue(
			values,
			AbilityData::ShieldGraft::Attribute::EnergyPowerScale,
			0.12f
		);

		const float energyBonus = ownerEnergyPower / std::max(energyReference, 0.001f) * energyScale;
		const float conversion = baseConversion + energyBonus;
		if (!std::isfinite(conversion) || conversion <= 0.f)
		{
			return false;
		}

		const float missingHealth = std::max(0.f, maxHealth - currentHealth);
		if (missingHealth <= 0.f)
		{
			return false;
		}

		// heal = min(missingHealth, currentShield * conversion)
		float heal = std::min(missingHealth, currentShield * conversion);
		if (!std::isfinite(heal) || heal <= 0.f)
		{
			return false;
		}

		// cost = heal / conversion
		float cost = heal / conversion;
		if (cost > currentShield)
		{
			cost = currentShield;
		}
		if (!std::isfinite(cost) || cost <= 0.f)
		{
			return false;
		}

		// Clamp heal against float precision discrepancy to guarantee no overheal/overhealth
		heal = std::min(missingHealth, cost * conversion);
		if (!std::isfinite(heal) || heal <= 0.f)
		{
			return false;
		}

		// Non-damage shield consumption preserving recharge timer and reconciling
		// temporary overcap ledger; no damage events.
		ship->GetShieldComponent().ChangeShield(-cost);
		ship->GetHealthComponent().ChangeHealth(heal);

		// Spawn brief outer-to-inner visual on owner
		if (World* world = context.owner.GetWorld())
		{
			if (const ShieldGraftPresentationProfile* profile =
				PresentationProfileRegistry<ShieldGraftPresentationProfile>::Find(
					ShieldGraftPresentationIds::VisualBasic
				))
			{
				world->SpawnActor<ShieldGraftVisualActor>(
					&context.owner,
					*profile
				);
			}
		}

		return true;
	}
}
