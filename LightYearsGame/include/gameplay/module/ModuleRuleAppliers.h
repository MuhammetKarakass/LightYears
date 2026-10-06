#pragma once

#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/module/ModuleTriggerRuntime.h"

#include <cstddef>
#include <cstdint>

namespace ly
{
	class ModuleAbilityRuleApplier final
	{
	public:
		explicit ModuleAbilityRuleApplier(LightYearsAbilitySystemComponent& abilities) : mAbilities{ abilities } {}
		~ModuleAbilityRuleApplier() noexcept;
		ModuleAbilityRuleApplier(const ModuleAbilityRuleApplier&) = delete;
		ModuleAbilityRuleApplier& operator=(const ModuleAbilityRuleApplier&) = delete;

		void Replace(const List<ScopedAbilityRule>& rules);
		void Reset() noexcept;
		std::size_t GetOwnedHandleCount() const { return mHandles.size(); }

	private:
		void RemoveOwnedHandles();

		LightYearsAbilitySystemComponent& mAbilities;
		List<std::size_t> mHandles;
		std::uint64_t mGeneration = 0;
	};

	class ModuleTriggerBuffRuleApplier final
	{
	public:
		explicit ModuleTriggerBuffRuleApplier(ModuleTriggerRuntime& triggerRuntime) : mTriggerRuntime{ triggerRuntime } {}
		~ModuleTriggerBuffRuleApplier() noexcept;
		ModuleTriggerBuffRuleApplier(const ModuleTriggerBuffRuleApplier&) = delete;
		ModuleTriggerBuffRuleApplier& operator=(const ModuleTriggerBuffRuleApplier&) = delete;

		void Replace(const List<ResolvedModuleTriggerBuff>& rules);
		void Reset() noexcept;

	private:
		ModuleTriggerRuntime& mTriggerRuntime;
		std::uint64_t mGeneration = 0;
	};

	class ModuleDamageRuleApplier final
	{
	public:
		explicit ModuleDamageRuleApplier(CombatRuntime& combatRuntime) : mRegistration{ combatRuntime.RegisterOutgoingDamageRules() } {}
		~ModuleDamageRuleApplier() noexcept = default;
		ModuleDamageRuleApplier(const ModuleDamageRuleApplier&) = delete;
		ModuleDamageRuleApplier& operator=(const ModuleDamageRuleApplier&) = delete;

		void Replace(const List<ResolvedModuleDamageRule>& rules) { mRegistration.ReplaceRules(rules); }
		void Reset() { mRegistration.Reset(); }

	private:
		CombatRuntime::OutgoingDamageRuleRegistration mRegistration;
	};

	class ModuleCooldownRuleApplier final
	{
	public:
		explicit ModuleCooldownRuleApplier(LightYearsAbilitySystemComponent& abilities) : mAbilities{ abilities } {}
		~ModuleCooldownRuleApplier() noexcept;
		ModuleCooldownRuleApplier(const ModuleCooldownRuleApplier&) = delete;
		ModuleCooldownRuleApplier& operator=(const ModuleCooldownRuleApplier&) = delete;

		void Replace(const List<ResolvedModuleCooldownRule>& rules);
		void Reset() noexcept;
		std::size_t GetOwnedHandleCount() const { return mHandles.size(); }

	private:
		void RemoveOwnedHandles();

		LightYearsAbilitySystemComponent& mAbilities;
		List<std::size_t> mHandles;
		std::uint64_t mGeneration = 0;
	};
}
