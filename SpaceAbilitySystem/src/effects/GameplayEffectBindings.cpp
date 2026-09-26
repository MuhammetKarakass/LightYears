#include "effects/GameplayEffectBindings.h"

#include <exception>
#include <utility>

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
		AttributeSystem& attributes,
		const std::function<bool()>& shouldContinue
	)
	{
		for (const AttributeModifier& modifier : spec.modifiers)
		{
			if (shouldContinue && !shouldContinue()) return;
			attributes.ApplyBaseModifier(modifier);
			if (shouldContinue && !shouldContinue()) return;
		}
	}

	void ApplyGameplayEffectModifiers(
		const GameplayEffectSpec& spec,
		GameplayEffectRuntimeState& state,
		AttributeSystem& attributes,
		const std::function<bool()>& shouldContinue
	)
	{
		state.appliedModifierHandles.reserve(
			state.appliedModifierHandles.size() + spec.modifiers.size()
		);
		for (const AttributeModifier& modifier : spec.modifiers)
		{
			if (shouldContinue && !shouldContinue())
			{
				return;
			}
			state.appliedModifierHandles.emplace_back();
			AttributeModifierHandle& committedHandle =
				state.appliedModifierHandles.back();
			AttributeModifierHandle returnedHandle;
			try { returnedHandle = attributes.AddModifier(modifier, committedHandle); }
			catch (...)
			{
				if (!committedHandle.IsValid()) state.appliedModifierHandles.pop_back();
				throw;
			}
			if (!returnedHandle.IsValid() || !committedHandle.IsValid())
			{
				state.appliedModifierHandles.pop_back();
			}
			if (shouldContinue && !shouldContinue())
			{
				return;
			}
		}
	}

	void RemoveGameplayEffectModifiers(
		GameplayEffectRuntimeState& state,
		AttributeSystem& attributes
	)
	{
		std::exception_ptr error;
		while (!state.appliedModifierHandles.empty())
		{
			const AttributeModifierHandle handle =
				state.appliedModifierHandles.back();
			state.appliedModifierHandles.pop_back();
			try { attributes.RemoveModifier(handle); }
			catch (...) { if (!error) error = std::current_exception(); }
		}
		if (error) std::rethrow_exception(error);
	}

	void GrantGameplayEffectTags(
		const GameplayEffectDefinition& definition,
		GameplayEffectRuntimeState& state,
		ly::GameplayTagContainer& ownedTags
	)
	{
		state.appliedGrantedTags.reserve(
			state.appliedGrantedTags.size() + definition.grantedTags.size()
		);
		for (const ly::GameplayTag& tag : definition.grantedTags)
		{
			ly::GameplayTag acquiredTag{ tag };
			ownedTags.AddTag(tag);
			state.appliedGrantedTags.emplace_back(std::move(acquiredTag));
		}
	}

	void RemoveGameplayEffectTags(
		GameplayEffectRuntimeState& state,
		ly::GameplayTagContainer& ownedTags
	)
	{
		std::exception_ptr error;
		while (!state.appliedGrantedTags.empty())
		{
			const ly::GameplayTag tag = state.appliedGrantedTags.back();
			state.appliedGrantedTags.pop_back();
			try { ownedTags.RemoveTag(tag); }
			catch (...) { if (!error) error = std::current_exception(); }
		}
		if (error) std::rethrow_exception(error);
	}
}
