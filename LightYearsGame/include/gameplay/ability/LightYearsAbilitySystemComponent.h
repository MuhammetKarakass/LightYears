#pragma once

#include "AbilitySystemComponent.h"
#include "abilities/AbilityEvent.h"
#include "gameplay/ability/GameAbility.h"

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

		void HandleAbilityLifecycleEvent(const sas::AbilityLifecycleEvent& event);

	private:
		void ProcessGameGameplayEvent(const sas::AbilityEvent& event);
		void ProcessAbilityLifecycleEvent(const sas::AbilityLifecycleEvent& event);
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
	};
}


