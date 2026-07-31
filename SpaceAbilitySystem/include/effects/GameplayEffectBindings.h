#pragma once

#include "effects/GameplayEffectDefinition.h"
#include "effects/GameplayEffectRuntimeState.h"
#include "effects/GameplayEffectSpec.h"

namespace sas
{
	bool CanApplyGameplayEffect(
		const GameplayEffectDefinition& definition,
		const ly::GameplayTagContainer& ownedTags
	);
	void ApplyInstantGameplayEffect(
		const GameplayEffectSpec& spec,
		AttributeSystem& attributes
	);
	void ApplyGameplayEffectModifiers(
		const GameplayEffectSpec& spec,
		GameplayEffectRuntimeState& state,
		AttributeSystem& attributes
	);
	void RemoveGameplayEffectModifiers(
		GameplayEffectRuntimeState& state,
		AttributeSystem& attributes
	);
	void GrantGameplayEffectTags(
		const GameplayEffectDefinition& definition,
		ly::GameplayTagContainer& ownedTags
	);
	void RemoveGameplayEffectTags(
		const GameplayEffectDefinition& definition,
		ly::GameplayTagContainer& ownedTags
	);
}
