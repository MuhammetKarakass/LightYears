#pragma once

#include "AbilitySystemComponent.h"
#include "abilities/AbilityEvent.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/runtime/AbilityLifecycleDispatcher.h"
#include "gameplay/ability/runtime/AbilityInvocationRuntime.h"
#include "gameplay/ability/runtime/AbilityUseHistory.h"
#include "gameplay/weapon/runtime/PrimaryWeaponOverrideState.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace ly
{
	class Actor;
	struct AttachmentDefinition;
	enum class AttachmentHostKind;

	struct ScopedAbilityRule
	{
		List<GameplayTag> requiredAbilityTags;
		List<GameplayTag> blockedAbilityTags;
		List<sas::AttributeModifier> attributeModifiers;
		int levelBonus = 0;
	};

	class LightYearsAbilitySystemComponent
		: public sas::AbilitySystemComponent
	{
	public:
		static constexpr size_t MaxPassiveAbilities = 2;

		explicit LightYearsAbilitySystemComponent(Actor& owner);


		bool TryEquipAttachment(
			sas::AbilityHandle handle,
			const AttachmentDefinition& definition,
			AttachmentHostKind hostKind,
			std::string* failureReason = nullptr
		);
		bool TryEquipAttachment(
			sas::AbilitySlot slot,
			const AttachmentDefinition& definition,
			AttachmentHostKind hostKind,
			std::string* failureReason = nullptr
		);
		bool RemoveAttachment(sas::AbilityHandle handle, const std::string& attachmentId, AttachmentHostKind hostKind);
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
		Actor& GetOwner() { return mOwner; }

		// Shared action-lock producers call this entry point. A primary-weapon
		// lock must interrupt an already active automatic weapon immediately;
		// checking the lock only during the next activation would allow the
		// current WhileHeld fire execution to continue.
		void AddOwnedTag(const GameplayTag& tag);
		void SetAbilitySlotInput(sas::AbilitySlot slot, bool inputHeld);
		void Tick(float deltaTime);
		void Clear();

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
		std::size_t mNextScopedAbilityRuleHandle = 1;
		AbilityLifecycleDispatcher mLifecycleDispatcher;
		AbilityUseHistory mAbilityUseHistory;
		AbilityInvocationRuntime mAbilityInvocationRuntime;
		PrimaryWeaponOverrideState mPrimaryWeaponOverrides;
	};
}


