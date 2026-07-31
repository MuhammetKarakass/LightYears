#pragma once

#include "framework/Core.h"
#include "attributes/AttributeSystem.h"
#include "effects/GameplayEffectPolicies.h"

#include <string>

namespace sas
{
	struct GameplayEffectDefinition
	{
		std::string effectId;
		ly::GameplayTag behaviorTag;
		GameplayEffectDurationPolicy durationPolicy = GameplayEffectDurationPolicy::Instant;
		GameplayEffectStackingPolicy stackingPolicy = GameplayEffectStackingPolicy::None;
		float duration = 0.f;
		int maxStacks = 1;
		ly::List<ly::GameplayTag> grantedTags;
		ly::List<AttributeModifier> modifiers;
		GameplayAttributeList attributes;
		std::string activeVisualId;
		ly::List<ly::GameplayTag> applicationRequiredTags;
		ly::List<ly::GameplayTag> applicationBlockedTags;
		bool sourceScopedApplication = false;
	};
}
