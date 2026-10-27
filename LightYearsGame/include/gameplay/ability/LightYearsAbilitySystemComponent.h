#pragma once

#include "AbilitySystemComponent.h"
#include "abilities/AbilityEvent.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/ScopedAbilityRule.h"
#include "gameplay/ability/runtime/AbilityLifecycleDispatcher.h"
#include "gameplay/ability/runtime/AbilityInvocationRuntime.h"
#include "gameplay/ability/runtime/AbilityUseHistory.h"
#include "gameplay/module/ModuleSystemGate.h"
#include "gameplay/weapon/PrimaryWeaponMountProfile.h"
#include "gameplay/weapon/runtime/PrimaryWeaponOverrideState.h"

#include <cstddef>
#include <array>
#include <optional>
#include <utility>
#include <vector>

namespace vega
{
	class Actor;
	class LegendarySecondHardpointRuntime;
	class LegendaryMeltdownBarrelRuntime;

	class LightYearsAbilitySystemComponent
		: public sas::AbilitySystemComponent
	{
	public:
		static constexpr size_t MaxPassiveAbilities = 2;
		class ModuleAttributeChangeScope final
		{
		public:
			explicit ModuleAttributeChangeScope(LightYearsAbilitySystemComponent& owner) : mOwner{ owner } { ++mOwner.mModuleAttributeChangeDepth; }
			~ModuleAttributeChangeScope() { --mOwner.mModuleAttributeChangeDepth; }
			ModuleAttributeChangeScope(const ModuleAttributeChangeScope&) = delete;
			ModuleAttributeChangeScope& operator=(const ModuleAttributeChangeScope&) = delete;
		private:
			LightYearsAbilitySystemComponent& mOwner;
		};
		bool IsApplyingModuleAttributes() const { return mModuleAttributeChangeDepth != 0; }

		explicit LightYearsAbilitySystemComponent(Actor& owner);
		// The secondary bank is retained when Twin Rack is removed. Slot identity
		// remains Q/E/F/R; each instance has its own handle, level and recharge.
		bool AssignSecondAbility(sas::AbilitySlot slot, const GameAbilityDefinition& definition, std::string* failureReason = nullptr);
		GameAbility* GetSecondAbility(sas::AbilitySlot slot);
		const GameAbility* GetSecondAbility(sas::AbilitySlot slot) const;
		void ConfigureTwinRack(bool enabled, float cooldownMultiplier = 1.f);
		bool IsTwinRackEnabled() const { return mTwinRackEnabled; }
		float GetTwinRackCooldownMultiplier(sas::AbilitySlot slot) const { return sas::IsLoadoutAbilitySlot(slot) ? mTwinRackCooldownMultiplier : 1.f; }
		void SetLoadoutKeyInput(sas::AbilitySlot slot, bool held, bool control);
		List<sas::AbilityRuntimeSnapshot> BuildAbilitySnapshots() const;
		using sas::AbilitySystemComponent::SetAbilityLevel;
		bool SetAbilityLevel(sas::AbilityHandle handle, int level) { return SetAbilityLevel(handle, level, nullptr, nullptr); }
		bool SetAbilityLevel(sas::AbilityHandle handle, int level, sas::AbilityLevelCommitSink commitSink, void* commitContext);

		GameAbility* GetAbility(sas::AbilitySlot slot);
		const GameAbility* GetAbility(sas::AbilitySlot slot) const;
		GameAbility* GetAbility(sas::AbilityHandle handle);
		const GameAbility* GetAbility(sas::AbilityHandle handle) const;
		GameAbility* GetAbilityById(const std::string& abilityId);
		const GameAbility* GetAbilityById(const std::string& abilityId) const;
		int CountLoadoutAbilitiesWithTag(const GameplayTag& category) const;
		// True when the Q/E/F/R set holding the ability (the second set under Twin Rack) is full and its four abilities belong to four different families.
		bool HasDistinctLoadoutFamilies(sas::AbilityHandle ability) const;
		std::size_t AddScopedAbilityRule(ScopedAbilityRule rule);
		bool RemoveScopedAbilityRule(std::size_t ruleHandle);
		void ClearScopedAbilityRules();
		std::size_t AddScopedAbilityCooldownRule(ScopedAbilityCooldownRule rule);
		bool RemoveScopedAbilityCooldownRule(std::size_t ruleHandle);
		float GetScopedAbilityCooldownMultiplier(const GameAbilityDefinition& definition, std::optional<float> abilityModuleSnapshot = std::nullopt) const;
		float GetAbilityModuleCooldownMultiplier(const GameAbilityDefinition& definition) const;
		float GetScopedAbilityCooldownMinimum(const GameAbilityDefinition& definition) const;
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
		std::size_t AddPrimaryWeaponModifier(const sas::AttributeModifier& modifier, std::optional<PrimaryWeaponType> requiredWeaponType = std::nullopt);
		bool RemovePrimaryWeaponModifier(std::size_t modifierHandle);
		uint64_t GetPrimaryWeaponModifierRevision() const { return mPrimaryWeaponModifierRevision; }
		// Mount shape of the Primary fire (Turret Mount, Broadside Mount); the module applier sets it on every rebuild.
		void SetPrimaryMountProfile(const PrimaryWeaponMountProfile& profile) { mPrimaryMountProfile = profile; }
		const PrimaryWeaponMountProfile& GetPrimaryMountProfile() const { return mPrimaryMountProfile; }
		// Activations of loadout abilities (Ability1-4), never primary fire or
		// passives. Weapons compare it to grant empowered charges on cast.
		uint64_t GetLoadoutAbilityActivationCount() const { return mLoadoutAbilityActivationCount; }
		float ApplyPrimaryWeaponModifiers(const sas::GameplayAttribute& attribute, std::optional<PrimaryWeaponType> weaponType = std::nullopt) const;
		// The gate is owned by the same ship that owns this component and clears itself on
		// destruction (see ShipModuleApplier), so the pointer never dangles. Null means
		// every module system is enabled.
		void SetModuleSystemGate(ModuleSystemGate* gate) { mModuleSystemGate = gate; }
		bool IsModuleSystemEnabled(const GameplayTag& systemTag) const;
		float GetAbilityMainEffectMultiplier(const GameAbilityDefinition& definition, sas::AbilityHandle handle = {}) const;
		float GetAbilityMainEffectGroupMultiplier(const GameAbilityDefinition& definition, sas::AbilityHandle handle = {}) const;
		float GetPrimedArsenalBonus(const GameAbilityDefinition& definition, sas::AbilityHandle handle) const;
		void CommitPrimedArsenalUse(sas::AbilityHandle handle) { if (mModuleSystemGate) mModuleSystemGate->CommitPrimedArsenalUse(handle); }
		void RefreshModulePoolConversion() { if (mModuleSystemGate) mModuleSystemGate->RefreshPoolConversion(); }
		bool IsModuleAbilityAvailable(const std::string& abilityId) const;
		void CancelAbilitiesUnavailableByModules();
		Actor& GetOwner() { return mOwner; }
		// Non-owning module hooks; ShipModuleApplier owns and detaches these runtimes.
		void SetSecondHardpointRuntime(LegendarySecondHardpointRuntime* runtime) { mSecondHardpoint = runtime; }
		LegendarySecondHardpointRuntime* GetSecondHardpointRuntime() const { return mSecondHardpoint; }
		void SetMeltdownBarrelRuntime(LegendaryMeltdownBarrelRuntime* runtime) { mMeltdownBarrel = runtime; }
		LegendaryMeltdownBarrelRuntime* GetMeltdownBarrelRuntime() const { return mMeltdownBarrel; }

		// Shared action-lock producers call this entry point. A primary-weapon
		// lock must interrupt an already active automatic weapon immediately;
		// checking the lock only during the next activation would allow the
		// current WhileHeld fire execution to continue.
		void AddOwnedTag(const GameplayTag& tag) override;
		void SetAbilitySlotInput(sas::AbilitySlot slot, bool inputHeld) override;
		bool DebugActivateAbility(sas::AbilityHandle handle) override;
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
		void DescribeDebugAbility(sas::AbilityDebugEntry& entry) const override;
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
		LegendarySecondHardpointRuntime* mSecondHardpoint = nullptr;
		LegendaryMeltdownBarrelRuntime* mMeltdownBarrel = nullptr;
		std::array<unique_ptr<GameAbility>, 4> mSecondAbilities;
		std::array<bool, 4> mLoadoutKeyHeld{};
		std::array<bool, 4> mLoadoutKeySecond{};
		// Game-owned instances use descending handles; generic grants use ascending handles.
		unsigned int mNextSecondAbilityHandle = ~0u;
		bool mTwinRackEnabled = false;
		float mTwinRackCooldownMultiplier = 1.f;
		std::size_t mModuleAttributeChangeDepth = 0;
		ModuleSystemGate* mModuleSystemGate = nullptr;
		using ComponentRuntime =
			sas::AbilitySystemRuntime<GameAbilityDefinition, GameAbility>;
		ComponentRuntime& mComponentRuntime;
		std::vector<std::pair<std::size_t, ScopedAbilityRule>> mScopedAbilityRules;
		std::vector<std::pair<std::size_t, ScopedAbilityRule>> mDeferredScopedAbilityRules;
		std::vector<std::pair<std::size_t, ScopedAbilityCooldownRule>> mScopedAbilityCooldownRules;
		struct PrimaryModifier { sas::AttributeModifier modifier; std::optional<PrimaryWeaponType> requiredWeaponType; };
		std::vector<std::pair<std::size_t, PrimaryModifier>> mPrimaryWeaponModifiers;
		std::size_t mNextPrimaryWeaponModifierHandle = 1;
		uint64_t mPrimaryWeaponModifierRevision = 0;
		PrimaryWeaponMountProfile mPrimaryMountProfile;
		uint64_t mLoadoutAbilityActivationCount = 0;
		std::size_t mNextScopedAbilityRuleHandle = 1;
		std::size_t mNextScopedAbilityCooldownRuleHandle = 1;
		bool mHasDeferredScopedAbilityRules = false;
		bool mRefreshingScopedAbilityRules = false;
		AbilityLifecycleDispatcher mLifecycleDispatcher;
		AbilityUseHistory mAbilityUseHistory;
		AbilityInvocationRuntime mAbilityInvocationRuntime;
		PrimaryWeaponOverrideState mPrimaryWeaponOverrides;
		bool mTicking = false;
	};
}
