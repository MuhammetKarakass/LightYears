#pragma once

#include "effects/GameplayEffectDefinition.h"
#include "effects/GameplayEffectRuntimeState.h"
#include "effects/GameplayEffectSpec.h"

#include <functional>

namespace sas
{
	bool CanApplyGameplayEffect(
		const GameplayEffectDefinition& definition,
		const ly::GameplayTagContainer& ownedTags
	);
	void ApplyInstantGameplayEffect(
		const GameplayEffectSpec& spec,
		AttributeSystem& attributes,
		const std::function<bool()>& shouldContinue = {}
	);
	void ApplyGameplayEffectModifiers(
		const GameplayEffectSpec& spec,
		GameplayEffectRuntimeState& state,
		AttributeSystem& attributes,
		const std::function<bool()>& shouldContinue = {}
	);
	void RemoveGameplayEffectModifiers(
		GameplayEffectRuntimeState& state,
		AttributeSystem& attributes
	);
	void GrantGameplayEffectTags(
		const GameplayEffectDefinition& definition,
		GameplayEffectRuntimeState& state,
		ly::GameplayTagContainer& ownedTags
	);
	void RemoveGameplayEffectTags(
		GameplayEffectRuntimeState& state,
		ly::GameplayTagContainer& ownedTags
	);
}
