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
		ly::List<ly::GameplayTag> abilityTags;
		ly::List<ly::GameplayTag> requiredOwnerTags;
		ly::List<ly::GameplayTag> blockedOwnerTags;
		ly::List<AttributeModifier> attributeModifiers;
		ly::List<AttributeScalingRule> scalingRules;
		ly::List<std::string> unlockedUpgradeIds;
	};
}
