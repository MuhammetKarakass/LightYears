#pragma once

#include "framework/Core.h"
#include "abilities/AbilityPolicies.h"
#include "attributes/AttributeSystem.h"

#include <string>

namespace sas
{
	struct AbilityDefinition
	{
		std::string abilityId;
		// Transitional content default. Runtime loadout callers must pass an
		// AbilityRuntimeBinding; this field is no longer the source of input truth.
		AbilitySlot slot = AbilitySlot::Ability1;
		AbilityActivationPolicy activationPolicy = AbilityActivationPolicy::OnPressed;
		AbilityLifetimePolicy lifetimePolicy = AbilityLifetimePolicy::Instant;
		AbilityCooldownStartPolicy cooldownStartPolicy = AbilityCooldownStartPolicy::OnAbilityEnd;
		float cooldown = 0.f;
		float duration = 0.f;
		int maxCharges = 1;
		// Minimum gap between two charge uses. Applies only while the effective capacity exceeds one.
		float chargeUseInterval = 0.5f;
		vega::List<vega::GameplayTag> abilityTags;
		vega::List<vega::GameplayTag> requiredOwnerTags;
		vega::List<vega::GameplayTag> blockedOwnerTags;
		vega::List<AttributeModifier> attributeModifiers;
		vega::List<AttributeScalingRule> scalingRules;
		vega::List<std::string> unlockedUpgradeIds;
	};
}
