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

	// Resolves the ability definition's own numeric values through the same
	// level, attachment, owner-scaling, and clamp pipeline used by actions.
	sas::GameplayAttributeList ResolveAbilityAttributes(
		AbilityExecutionContext& context
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
