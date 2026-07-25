#pragma once

#include "framework/Core.h"
#include "framework/Delegate.h"
#include "gameplay/ability/AbilityInstance.h"
#include "gameplay/ability/AbilityEvent.h"

namespace ly
{
	class Actor;
	class AttributeSystem;
	class GameplayEffectSystem;

	class AbilitySystem
	{
	public:
		static constexpr size_t MaxPassiveAbilities = 2;

		AbilitySystem(
			Actor& owner,
			AttributeSystem& attributes,
			GameplayEffectSystem& effects,
			GameplayTagContainer& ownedTags
		);

		static bool ValidateDefinition(const AbilityDefinition& definition, std::string* failureReason = nullptr);
		static bool ValidateCatalog(
			const List<const AbilityDefinition*>& definitions,
			std::string* failureReason = nullptr
		);

		AbilityHandle GrantAbility(const AbilityDefinition& definition, std::string* failureReason = nullptr);
		bool RemoveAbility(AbilityHandle handle, AbilityEndReason reason = AbilityEndReason::Cancelled);
		bool TrySetAbilityLevel(AbilityHandle handle, int level);
		bool TrySetAbilityLevel(AbilitySlot slot, int level);
		bool TryLevelUpAbility(AbilityHandle handle);
		bool TryLevelUpAbility(AbilitySlot slot);
		bool TryEquipAttachment(
			AbilityHandle handle,
			const AttachmentDefinition& definition,
			AttachmentHostKind hostKind,
			std::string* failureReason = nullptr
		);
		bool TryEquipAttachment(
			AbilitySlot slot,
			const AttachmentDefinition& definition,
			AttachmentHostKind hostKind,
			std::string* failureReason = nullptr
		);
		bool RemoveAttachment(AbilityHandle handle, const GameplayTag& attachmentId, AttachmentHostKind hostKind);
		void ClearSlot(AbilitySlot slot);
		void SetSlotInput(AbilitySlot slot, bool inputHeld);
		void Tick(float deltaTime);
		void Clear();

		AbilityInstance* GetAbility(AbilitySlot slot);
		const AbilityInstance* GetAbility(AbilitySlot slot) const;
		AbilityInstance* GetAbility(AbilityHandle handle);
		const AbilityInstance* GetAbility(AbilityHandle handle) const;
		AbilityInstance* GetAbilityById(const std::string& abilityId);
		const AbilityInstance* GetAbilityById(const std::string& abilityId) const;
		const List<AbilityHandle>& GetPassiveAbilities() const { return mPassiveAbilities; }
		size_t GetPassiveAbilityCount() const { return mPassiveAbilities.size(); }
		List<AbilityRuntimeSnapshot> BuildSnapshots() const;

		void HandleGameplayEvent(const AbilityEvent& event);

		Actor& GetOwner() { return *mOwner; }
		AttributeSystem& GetAttributes() { return *mAttributes; }
		GameplayEffectSystem& GetEffects() { return *mEffects; }
		const GameplayTagContainer& GetOwnedTags() const { return *mOwnedTags; }
		void AddOwnerTag(const GameplayTag& tag);
		void RemoveOwnerTag(const GameplayTag& tag);
		bool HasAllOwnerTags(const List<GameplayTag>& tags) const;
		bool HasAnyOwnerTags(const List<GameplayTag>& tags) const;

		void NotifyAbilityChanged(AbilityHandle handle);
		void NotifyAbilityActivated(AbilityHandle handle);
		void NotifyAbilityEnded(AbilityHandle handle, AbilityEndReason reason);
		void NotifyAbilityLevelChanged(AbilityHandle handle, int level);
		void ReduceCooldowns(float amount, bool includePrimaryFire);

		Delegate<AbilityHandle> onAbilityGranted;
		Delegate<AbilityHandle> onAbilityRemoved;
		Delegate<AbilityHandle> onAbilityChanged;
		Delegate<AbilityHandle> onAbilityActivated;
		Delegate<AbilityHandle, AbilityEndReason> onAbilityEnded;
		Delegate<AbilityHandle, int> onAbilityLevelChanged;
		Delegate<> onAbilitiesCleared;
		Delegate<const AbilityEvent&> onGameplayEvent;

	private:
		bool IsPassiveDefinition(const AbilityDefinition& definition) const;
		void RemovePassiveHandle(AbilityHandle handle);
		void UpdateTriggerCooldowns(float deltaTime);
		std::string MakeTriggerCooldownKey(const AbilityDefinition& definition, const AbilityTriggerSpec& trigger, size_t triggerIndex) const;

		Actor* mOwner = nullptr;
		AttributeSystem* mAttributes = nullptr;
		GameplayEffectSystem* mEffects = nullptr;
		GameplayTagContainer* mOwnedTags = nullptr;
		Map<AbilityHandle, unique_ptr<AbilityInstance>> mAbilities;
		Map<AbilitySlot, AbilityHandle> mSlotBindings;
		Map<std::string, AbilityHandle> mAbilityIds;
		List<AbilityHandle> mPassiveAbilities;
		Map<std::string, float> mTriggerCooldowns;
		unsigned int mNextAbilityHandleId = 1;
	};
}


