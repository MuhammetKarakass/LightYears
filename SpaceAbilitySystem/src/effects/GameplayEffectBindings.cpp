#include "effects/GameplayEffectBindings.h"

namespace sas
{
	bool CanApplyGameplayEffect(
		const GameplayEffectDefinition& definition,
		const ly::GameplayTagContainer& ownedTags
	)
	{
		return ownedTags.HasAll(definition.applicationRequiredTags) &&
			!ownedTags.HasAny(definition.applicationBlockedTags);
	}

	void ApplyInstantGameplayEffect(
		const GameplayEffectSpec& spec,
		AttributeSystem& attributes
	)
	{
		for (const AttributeModifier& modifier : spec.modifiers)
		{
			attributes.ApplyBaseModifier(modifier);
		}
	}

	void ApplyGameplayEffectModifiers(
		const GameplayEffectSpec& spec,
		GameplayEffectRuntimeState& state,
		AttributeSystem& attributes
	)
	{
		for (const AttributeModifier& modifier : spec.modifiers)
		{
			const AttributeModifierHandle handle =
				attributes.AddModifier(modifier);
			if (handle.IsValid())
			{
				state.appliedModifierHandles.push_back(handle);
			}
		}
	}

	void RemoveGameplayEffectModifiers(
		GameplayEffectRuntimeState& state,
		AttributeSystem& attributes
	)
	{
		for (const AttributeModifierHandle handle :
			state.appliedModifierHandles)
		{
			attributes.RemoveModifier(handle);
		}
		state.appliedModifierHandles.clear();
	}

	void GrantGameplayEffectTags(
		const GameplayEffectDefinition& definition,
		ly::GameplayTagContainer& ownedTags
	)
	{
		for (const ly::GameplayTag& tag : definition.grantedTags)
		{
			ownedTags.AddTag(tag);
		}
	}

	void RemoveGameplayEffectTags(
		const GameplayEffectDefinition& definition,
		ly::GameplayTagContainer& ownedTags
	)
	{
		for (const ly::GameplayTag& tag : definition.grantedTags)
		{
			ownedTags.RemoveTag(tag);
		}
	}
}
