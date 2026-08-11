#pragma once

#include "attributes/AttributeSystem.h"
#include "abilities/AbilityBehavior.h"
#include "abilities/AbilityBehaviorRegistry.h"
#include "abilities/AbilityEvent.h"
#include "abilities/GameplayAbilityInstance.h"

#include "gameplay/ability/GameAbilityActionExecutor.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/attachment/AttachmentLoadout.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"

namespace ly
{
	class LightYearsAbilitySystemComponent;
	class Actor;
	class GameAbility;

	struct GameAbilityBehaviorContext
	{
		LightYearsAbilitySystemComponent& abilitySystem;
		GameAbility& instance;
		Actor& owner;
		const GameAbilityDefinition& definition;
	};

	// Game-owned behavior extensions stay outside SpaceAbilitySystem. The
	// default methods preserve the generic behavior contract while allowing a
	// family to resolve dynamic duration or react to another ability's successful
	// activation without adding family IDs to the runtime core.
	class GameAbilityBehavior
		: public sas::AbilityBehavior<GameAbilityDefinition, GameAbilityBehaviorContext>
	{
	public:
		virtual float ResolveActiveDuration(
			const GameAbilityBehaviorContext& context,
			float defaultDuration
		) const
		{
			(void)context;
			return defaultDuration;
		}

		virtual void OnOwnerAbilityActivated(
			GameAbilityBehaviorContext& context,
			const sas::AbilityLifecycleEvent& event
		)
		{
			(void)context;
			(void)event;
		}
	};

	using GameAbilityBehaviorRegistry = sas::AbilityBehaviorRegistry<GameAbilityBehavior,AbilityBehaviorType>;

	class GameAbility: public sas::GameplayAbilityInstance<GameAbilityDefinition,GameAbilityExecution>
	{
	public:
		GameAbility(
			LightYearsAbilitySystemComponent& abilitySystem,
			sas::AbilityHandle handle,
			const GameAbilityDefinition& definition,
			unique_ptr<GameAbilityBehavior> behavior
		);

		bool IsInputHeld() const { return this->mRuntimeState.IsInputHeld(); }
		bool IsPressedThisFrame() const { return this->mRuntimeState.IsPressedThisFrame(); }

		float GetWeaponFireIntervalRemaining() const { return mWeaponFireIntervalRemaining; }
		void SetWeaponFireIntervalRemaining(float interval);
		PrimaryWeaponRuntimeState& GetPrimaryWeaponRuntime() { return mPrimaryWeaponRuntime; }
		const sas::GameplayAttributeList& GetPrimaryWeaponRuntimeAttributes() const
		{
			return mPrimaryWeaponRuntimeAttributes;
		}
		void UpdatePrimaryWeaponRuntimeContext(
			const PrimaryWeaponDefinition& weaponDefinition,
			const sas::GameplayAttributeList& attributes,
			const List<GameplayTag>& damageTags
		);
		bool TryEquipAttachment(
			const AttachmentDefinition& definition,
			AttachmentHostKind hostKind,
			std::string* failureReason = nullptr
		);
		bool RemoveAttachment(const std::string& attachmentId, AttachmentHostKind hostKind);
		const AttachmentLoadout& GetAttachments() const { return mAttachments; }
		sas::GameplayAttributeList MergeAttachmentAttributes(
			AttachmentHostKind hostKind,
			const sas::GameplayAttributeList& attributes
		) const;
		sas::GameplayAttribute ApplyAttachmentModifiers(
			AttachmentHostKind hostKind,
			const sas::GameplayAttribute& attribute
		) const;
		sas::GameplayAttributeList ApplyAttachmentConditions(
			AttachmentHostKind hostKind,
			const sas::GameplayAttributeList& attributes,
			const List<GameplayTag>& originalDamageTags
		) const;
		List<GameplayTag> GetResolvedDamageTags(AttachmentHostKind hostKind) const;
		void HandleAttachmentEvent(const sas::AbilityEvent& event);
		void HandleAbilityLifecycleEvent(const sas::AbilityLifecycleEvent& event);

	private:
		friend class LightYearsAbilitySystemComponent;
		friend class GameAbilityActionExecutor;

		bool CanActivateContent() const override;
		bool ActivateContent() override;
		void BeginExecution() override;
		void TickExecution(float deltaTime) override;
		void EndExecution(sas::AbilityEndReason reason) override;
		void TickInactive(float deltaTime) override;
		void EndContent(sas::AbilityEndReason reason) override;
		void NotifyOwnerAbilityActivated(const sas::AbilityLifecycleEvent& event);
		int GetMaximumLevel() const override;
		float ResolveCooldownDuration() const override;
		float ResolveActiveDuration() const override;
		void RebuildDefinitionForLevel() override;
		void OnLevelConfigurationChanged() override;

		void UpdateWeaponFireInterval(float deltaTime);
		void RefreshScopedConfiguration();
		void HandleAttachmentEventInternal(
			const sas::AbilityEvent& event,
			const sas::AbilityLifecycleEvent* lifecycleEvent
		);
		bool MatchesAttachmentEventRule(
			const AttachmentEventRule& rule,
			const sas::AbilityEvent& event,
			const sas::AbilityLifecycleEvent* lifecycleEvent,
			const GameplayTagContainer& sourceAbilityTags,
			const List<GameplayTag>* damageTags
		) const;
		bool ExecuteAttachmentEventRule(
			EquippedAttachment& equipped,
			const AttachmentEventRule& rule
		);
		void TickInactivePrimaryWeaponRuntime(float deltaTime);
		void RefreshPrimaryWeaponRuntimeConfiguration();
		List<GameplayTag> GetAttachmentCapabilities(AttachmentHostKind hostKind) const;
		size_t GetAttachmentSlotCapacity(AttachmentHostKind hostKind) const;

		LightYearsAbilitySystemComponent& mAbilitySystem;
		float mWeaponFireIntervalRemaining = 0.f;
		PrimaryWeaponRuntimeState mPrimaryWeaponRuntime;
		std::string mPrimaryWeaponRuntimeWeaponId;
		sas::GameplayAttributeList mPrimaryWeaponRuntimeAttributes;
		List<GameplayTag> mPrimaryWeaponRuntimeDamageTags;
		AttachmentLoadout mAttachments;
		unique_ptr<GameAbilityBehavior> mBehavior;
	};
}
