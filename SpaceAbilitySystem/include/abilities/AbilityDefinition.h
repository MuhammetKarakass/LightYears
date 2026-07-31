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
		AbilitySlot slot = AbilitySlot::Ability1;
		AbilityActivationPolicy activationPolicy = AbilityActivationPolicy::OnPressed;
		AbilityLifetimePolicy lifetimePolicy = AbilityLifetimePolicy::Instant;
		float cooldown = 0.f;
		float duration = 0.f;
		int maxCharges = 1;
		ly::List<ly::GameplayTag> abilityTags;
		ly::List<ly::GameplayTag> requiredOwnerTags;
		ly::List<ly::GameplayTag> blockedOwnerTags;
		ly::List<AttributeModifier> attributeModifiers;
		ly::List<AttributeScalingRule> scalingRules;
		ly::List<ly::GameplayTag> unlockedUpgradeIds;
	};
}
