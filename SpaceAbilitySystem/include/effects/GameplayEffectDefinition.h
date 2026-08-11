#pragma once

#include "framework/Core.h"
#include "attributes/AttributeSystem.h"
#include "effects/GameplayEffectBehaviorKey.h"
#include "effects/GameplayEffectPolicies.h"

#include <string>

namespace sas
{
	struct GameplayEffectDefinition
	{
		std::string effectId;
		GameplayEffectBehaviorKey behaviorKey;
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
		// Policy-only definitions receive duration, stacks and magnitudes from the
		// source-owned GameplayEffectSpec at application time.
		bool sourceParameterized = false;
		GameplayEffectDisposition disposition = GameplayEffectDisposition::Neutral;
		bool cleanseable = false;
		std::string category;
		std::string immunityCategory;
		// A non-empty value marks this effect as an immunity provider. Incoming
		// effects use immunityCategory to declare what can block them; providers
		// use this separate field so ordinary Slow effects do not immunize against
		// other Slow effects merely because they share the same classification.
		std::string grantedImmunityCategory;
	};
}
