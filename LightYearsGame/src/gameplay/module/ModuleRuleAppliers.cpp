#include "gameplay/module/ModuleRuleAppliers.h"

#include <utility>

namespace vega
{
	ModuleAbilityRuleApplier::~ModuleAbilityRuleApplier() noexcept { Reset(); }

	void ModuleAbilityRuleApplier::Replace(const List<ScopedAbilityRule>& rules)
	{
		const List<ScopedAbilityRule> desired = rules;
		const std::uint64_t generation = ++mGeneration;
		RemoveOwnedHandles();
		if (generation != mGeneration) return;
		mHandles.reserve(desired.size());
		for (const ScopedAbilityRule& rule : desired)
		{
			const std::size_t handle = mAbilities.AddScopedAbilityRule(rule);
			if (generation != mGeneration)
			{
				try { mAbilities.RemoveScopedAbilityRule(handle); }
				catch (...) {}
				return;
			}
			mHandles.push_back(handle);
		}
	}

	void ModuleAbilityRuleApplier::Reset() noexcept
	{
		++mGeneration;
		RemoveOwnedHandles();
	}

	void ModuleAbilityRuleApplier::RemoveOwnedHandles()
	{
		List<std::size_t> handles = std::move(mHandles);
		mHandles.clear();
		for (const std::size_t handle : handles)
		{
			try { mAbilities.RemoveScopedAbilityRule(handle); }
			catch (...) {}
		}
	}

	ModuleTriggerBuffRuleApplier::~ModuleTriggerBuffRuleApplier() noexcept { Reset(); }

	void ModuleTriggerBuffRuleApplier::Replace(const List<ResolvedModuleTriggerBuff>& rules)
	{
		mTriggerRuntime.SetRules(rules, mGeneration);
	}

	void ModuleTriggerBuffRuleApplier::Reset() noexcept
	{
		const std::uint64_t generation = std::exchange(mGeneration, 0);
		if (generation == 0) return;
		try { mTriggerRuntime.ClearRules(generation); }
		catch (...) {}
	}

	ModuleCooldownRuleApplier::~ModuleCooldownRuleApplier() noexcept { Reset(); }

	void ModuleCooldownRuleApplier::Replace(const List<ResolvedModuleCooldownRule>& rules)
	{
		const List<ResolvedModuleCooldownRule> desired = rules;
		const std::uint64_t generation = ++mGeneration;
		RemoveOwnedHandles();
		if (generation != mGeneration) return;
		mHandles.reserve(desired.size());
		for (const ResolvedModuleCooldownRule& resolved : desired)
		{
			ScopedAbilityCooldownRule rule;
			if (resolved.slot != sas::AbilitySlot::None) rule.requiredSlot = resolved.slot;
			else if (resolved.scope == ModuleCooldownScope::FamilyOfZone)
			{
				if (!resolved.family.IsValid()) continue;
				rule.requiredAbilityTags.push_back(resolved.family);
			}
			rule.multiplier = resolved.multiplier;
			rule.minimumSeconds = resolved.minimumSeconds;
			const std::size_t handle = mAbilities.AddScopedAbilityCooldownRule(std::move(rule));
			if (generation != mGeneration)
			{
				try { mAbilities.RemoveScopedAbilityCooldownRule(handle); }
				catch (...) {}
				return;
			}
			mHandles.push_back(handle);
		}
	}

	void ModuleCooldownRuleApplier::Reset() noexcept
	{
		++mGeneration;
		RemoveOwnedHandles();
	}

	void ModuleCooldownRuleApplier::RemoveOwnedHandles()
	{
		List<std::size_t> handles = std::move(mHandles);
		mHandles.clear();
		for (const std::size_t handle : handles)
		{
			try { mAbilities.RemoveScopedAbilityCooldownRule(handle); }
			catch (...) {}
		}
	}
}
