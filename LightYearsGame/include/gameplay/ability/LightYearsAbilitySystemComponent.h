#pragma once

#include "AbilitySystemComponent.h"
#include "abilities/AbilityEvent.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/ScopedAbilityRule.h"
#include "gameplay/ability/runtime/AbilityLifecycleDispatcher.h"
#include "gameplay/ability/runtime/AbilityInvocationRuntime.h"
#include "gameplay/ability/runtime/AbilityUseHistory.h"
#include "gameplay/weapon/runtime/PrimaryWeaponOverrideState.h"

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace ly
{
	class Actor;

	class LightYearsAbilitySystemComponent
		: public sas::AbilitySystemComponent
	{
	public:
		static constexpr size_t MaxPassiveAbilities = 2;

		explicit LightYearsAbilitySystemComponent(Actor& owner);

		GameAbility* GetAbility(sas::AbilitySlot slot);
		const GameAbility* GetAbility(sas::AbilitySlot slot) const;
		GameAbility* GetAbility(sas::AbilityHandle handle);
		const GameAbility* GetAbility(sas::AbilityHandle handle) const;
		GameAbility* GetAbilityById(const std::string& abilityId);
		const GameAbility* GetAbilityById(const std::string& abilityId) const;
		std::size_t AddScopedAbilityRule(ScopedAbilityRule rule);
		bool RemoveScopedAbilityRule(std::size_t ruleHandle);
		void ClearScopedAbilityRules();
		sas::GameplayAttribute ApplyScopedAbilityModifiers(
			const GameAbilityDefinition& definition,
			const sas::GameplayAttribute& attribute
		) const;
		int GetScopedAbilityLevelBonus(
			const GameAbilityDefinition& definition
		) const;
		// Owner-level modifiers that enter only the primary-fire weapon branch of the
		// attribute resolver (module Primary family). The revision is part of every
		// primary weapon cache key; it changes whenever the modifier set changes.
		std::size_t AddPrimaryWeaponModifier(const sas::AttributeModifier& modifier);
		bool RemovePrimaryWeaponModifier(std::size_t modifierHandle);
		uint64_t GetPrimaryWeaponModifierRevision() const { return mPrimaryWeaponModifierRevision; }
		// Activations of loadout abilities (Ability1-4), never primary fire or
		// passives. Weapons compare it to grant empowered charges on cast.
		uint64_t GetLoadoutAbilityActivationCount() const { return mLoadoutAbilityActivationCount; }
		float ApplyPrimaryWeaponModifiers(const sas::GameplayAttribute& attribute) const;
		Actor& GetOwner() { return mOwner; }

		// Shared action-lock producers call this entry point. A primary-weapon
		// lock must interrupt an already active automatic weapon immediately;
		// checking the lock only during the next activation would allow the
		// current WhileHeld fire execution to continue.
		void AddOwnedTag(const GameplayTag& tag) override;
		void SetAbilitySlotInput(sas::AbilitySlot slot, bool inputHeld) override;
		void Tick(float deltaTime) override;

		PrimaryWeaponOverrideHandle PushPrimaryWeaponOverride(
			const sas::ContentId& sourceId,
			const PrimaryWeaponDefinition& weaponDefinition,
			int priority = 0
		);
		bool RemovePrimaryWeaponOverride(PrimaryWeaponOverrideHandle handle);
		PrimaryWeaponOverrideState::ActiveOverride* GetActivePrimaryWeaponOverride();
		const PrimaryWeaponOverrideState::ActiveOverride* GetActivePrimaryWeaponOverride() const;

		bool InvokeRecordedAbility(
			const AbilityUseRecord& record,
			const List<sas::AttributeScalingRule>& scalingRules,
			float outputMultiplier,
			sas::AbilitySlot controlSlot,
			bool inputHeld,
			std::string* failureReason = nullptr
		);
		std::size_t GetActiveAbilityInvocationCount() const
		{
			return mAbilityInvocationRuntime.GetActiveInvocationCount();
		}

		// Common lifecycle services are intentionally exposed through the game
		// component rather than through individual abilities. This lets future
		// checkpoints, analytics and replay systems subscribe without teaching
		// every ability family about one another.
		AbilityLifecycleObserverHandle RegisterAbilityLifecycleObserver(
			AbilityLifecycleObserverFilter filter,
			AbilityLifecycleObserver callback,
			int priority = 0
		);
		bool UnregisterAbilityLifecycleObserver(
			AbilityLifecycleObserverHandle handle
		);
		AbilityActivationGuardHandle RegisterAbilityActivationGuard(
			AbilityActivationGuard callback,
			int priority = 0
		);
		bool UnregisterAbilityActivationGuard(
			AbilityActivationGuardHandle handle
		);
		bool EvaluateAbilityActivation(
			const sas::AbilityLifecycleEvent& event
		) const;
		const AbilityUseHistory& GetAbilityUseHistory() const
		{
			return mAbilityUseHistory;
		}
		AbilityUseHistory& GetAbilityUseHistory()
		{
			return mAbilityUseHistory;
		}

		void HandleAbilityLifecycleEvent(const sas::AbilityLifecycleEvent& event);

	private:
		void HandleAbilityActivated(sas::AbilityHandle handle);
		friend class GameAbility;
		friend class CombatRuntime;
		void ClearAdditionalState(bool preserveRuntimeDependencies) override;
		std::optional<bool> GetPendingAdditionalCleanup() const override;
		void ReleaseAdditionalRuntimeDependencies() override;
		void OnClearCompleted() override;
		void OnAbilityInstanceOperationsCompleted() override;
		void FlushDeferredScopedAbilityRules();
		bool HasAbilityCallbackInProgress() const;
		std::function<void()> mOwnerClearCompletion;
		void ProcessGameGameplayEvent(const sas::AbilityEvent& event);
		void ProcessAbilityLifecycleEvent(const sas::AbilityLifecycleEvent& event);
		void RecordAbilityActivation(const sas::AbilityLifecycleEvent& event);
		void CancelActiveAbilitiesForStun();
		void RefreshScopedAbilityRules();
		bool MatchesScopedAbilityRule(
			const ScopedAbilityRule& rule,
			const GameAbilityDefinition& definition
		) const;

		Actor& mOwner;
		using ComponentRuntime =
			sas::AbilitySystemRuntime<GameAbilityDefinition, GameAbility>;
		ComponentRuntime& mComponentRuntime;
		std::vector<std::pair<std::size_t, ScopedAbilityRule>> mScopedAbilityRules;
		std::vector<std::pair<std::size_t, ScopedAbilityRule>> mDeferredScopedAbilityRules;
		std::vector<std::pair<std::size_t, sas::AttributeModifier>> mPrimaryWeaponModifiers;
		std::size_t mNextPrimaryWeaponModifierHandle = 1;
		uint64_t mPrimaryWeaponModifierRevision = 0;
		uint64_t mLoadoutAbilityActivationCount = 0;
		std::size_t mNextScopedAbilityRuleHandle = 1;
		bool mHasDeferredScopedAbilityRules = false;
		bool mRefreshingScopedAbilityRules = false;
		AbilityLifecycleDispatcher mLifecycleDispatcher;
		AbilityUseHistory mAbilityUseHistory;
		AbilityInvocationRuntime mAbilityInvocationRuntime;
		PrimaryWeaponOverrideState mPrimaryWeaponOverrides;
		bool mTicking = false;
	};
}
