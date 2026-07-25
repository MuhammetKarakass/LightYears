#pragma once

#include "gameConfigs/ability/AbilityStructs.h"
#include "gameplay/ability/AbilityBehavior.h"
#include "gameplay/ability/AbilityExecution.h"
#include "gameplay/attachment/AttachmentLoadout.h"

namespace ly
{
	class AbilitySystem;
	class AbilityExecutor;
	struct AbilityEvent;

	struct AbilityRuntimeSnapshot
	{
		AbilityHandle handle;
		const AbilityDefinition* definition = nullptr;
		int level = 1;
		int maxLevel = 1;
		bool active = false;
		float cooldownRemaining = 0.f;
		float cooldownDuration = 0.f;
		float activeRemaining = 0.f;
		float activeDuration = 0.f;
		int charges = 0;
	};

	class AbilityInstance
	{
	public:
		AbilityInstance(
			AbilitySystem& abilitySystem,
			AbilityHandle handle,
			const AbilityDefinition& definition,
			unique_ptr<AbilityBehavior> behavior
		);

		void SetInputHeld(bool inputHeld);
		void Tick(float deltaTime);
		bool TryActivate();
		void Cancel(AbilityEndReason reason);

		bool IsActive() const { return mIsActive; }
		bool IsOnCooldown() const { return mCooldownRemaining > 0.f; }
		float GetCooldownRemaining() const { return mCooldownRemaining; }
		float GetCooldownDuration() const;
		float GetActiveTimeRemaining() const { return mActiveTimeRemaining; }
		float GetActiveDuration() const;
		int GetCharges() const { return mCharges; }
		int GetLevel() const { return mLevel; }
		int GetMaxLevel() const { return mBaseDefinition.GetMaxLevel(); }
		float GetWeaponFireIntervalRemaining() const { return mWeaponFireIntervalRemaining; }
		void SetWeaponFireIntervalRemaining(float interval);
		PrimaryWeaponRuntimeState& GetPrimaryWeaponRuntime() { return mPrimaryWeaponRuntime; }
		const GameplayAttributeList& GetPrimaryWeaponRuntimeAttributes() const
		{
			return mPrimaryWeaponRuntimeAttributes;
		}
		void UpdatePrimaryWeaponRuntimeContext(
			const PrimaryWeaponDefinition& weaponDefinition,
			const GameplayAttributeList& attributes,
			const List<GameplayTag>& damageTags
		);
		bool TryEquipAttachment(
			const AttachmentDefinition& definition,
			AttachmentHostKind hostKind,
			std::string* failureReason = nullptr
		);
		bool RemoveAttachment(const GameplayTag& attachmentId, AttachmentHostKind hostKind);
		const AttachmentLoadout& GetAttachments() const { return mAttachments; }
		GameplayAttributeList MergeAttachmentAttributes(
			AttachmentHostKind hostKind,
			const GameplayAttributeList& attributes
		) const;
		GameplayAttribute ApplyAttachmentModifiers(
			AttachmentHostKind hostKind,
			const GameplayAttribute& attribute
		) const;
		GameplayAttributeList ApplyAttachmentConditions(
			AttachmentHostKind hostKind,
			const GameplayAttributeList& attributes,
			const List<GameplayTag>& originalDamageTags
		) const;
		List<GameplayTag> GetResolvedDamageTags(AttachmentHostKind hostKind) const;
		void HandleAttachmentEvent(const AbilityEvent& event);
		void ReduceCooldownRemaining(float amount);
		AbilityHandle GetHandle() const { return mHandle; }
		const AbilityDefinition& GetDefinition() const { return mDefinition; }

		AbilityRuntimeSnapshot BuildSnapshot() const;

	private:
		friend class AbilitySystem;
		friend class AbilityExecutor;

		bool SetLevel(int level);
		void UpdateCooldown(float deltaTime);
		void UpdateWeaponFireInterval(float deltaTime);
		void TickInactivePrimaryWeaponRuntime(float deltaTime);
		void UpdateInputActivation();
		void TickActiveExecution(float deltaTime);
		void EndAbility(AbilityEndReason reason);
		void RebuildDefinitionForLevel();
		void RefreshPrimaryWeaponRuntimeConfiguration();
		bool IsPressedThisFrame() const;
		List<GameplayTag> GetAttachmentCapabilities(AttachmentHostKind hostKind) const;
		size_t GetAttachmentSlotCapacity(AttachmentHostKind hostKind) const;

		AbilitySystem* mAbilitySystem = nullptr;
		AbilityHandle mHandle;
		AbilityDefinition mBaseDefinition;
		AbilityDefinition mDefinition;
		int mLevel = 1;
		bool mInputHeld = false;
		bool mWasInputHeld = false;
		bool mIsActive = false;
		float mCooldownRemaining = 0.f;
		float mWeaponFireIntervalRemaining = 0.f;
		PrimaryWeaponRuntimeState mPrimaryWeaponRuntime;
		std::string mPrimaryWeaponRuntimeWeaponId;
		GameplayAttributeList mPrimaryWeaponRuntimeAttributes;
		List<GameplayTag> mPrimaryWeaponRuntimeDamageTags;
		float mActiveTimeRemaining = 0.f;
		int mCharges = 0;
		AttachmentLoadout mAttachments;
		AbilityExecution mExecution;
		unique_ptr<AbilityBehavior> mBehavior;
	};
}


