#pragma once

#include "gameplay/ability/GameAbilityActionExecutor.h"
#include "gameplay/attachment/AttachmentDefinition.h"

namespace ly::AbilityActionAttributeResolver
{
	// Shared value resolution for actions. Weapon execution and the generic
	// scheduler use the same ability scaling and attachment rules.
	sas::GameplayAttributeList ResolveAttributes(
		AbilityExecutionContext& context,
		const PrimaryWeaponDefinition* weaponDefinition,
		const sas::GameplayAttributeList& attributes,
		const List<GameplayTag>& originalDamageTags = {}
	);

	float ResolveEffectiveInterval(
		AbilityExecutionContext& context,
		float baseInterval,
		const PrimaryWeaponDefinition* weaponDefinition = nullptr
	);

	List<GameplayTag> BuildBaseDamageTags(const GameAbilityDefinition* definition);
	List<GameplayTag> ResolveDamageTags(
		const AbilityExecutionContext& context,
		AttachmentHostKind hostKind
	);
}
