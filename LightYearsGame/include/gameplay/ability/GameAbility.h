#pragma once

#include "attributes/AttributeSystem.h"
#include "abilities/AbilityBehavior.h"
#include "abilities/AbilityBehaviorRegistry.h"
#include "abilities/AbilityEvent.h"
#include "abilities/GameplayAbilityInstance.h"

#include "gameplay/ability/GameAbilityActionExecutor.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include "gameplay/weapon/runtime/PrimaryWeaponAbilityState.h"

#include <optional>

namespace vega
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

		virtual bool OnInputPressed(GameAbilityBehaviorContext& context)
		{
			(void)context;
			return false;
		}

		virtual void OnGameplayEvent(
			GameAbilityBehaviorContext& context,
			const sas::AbilityEvent& event
		)
		{
			(void)context;
			(void)event;
		}

		// Runs before the shared lifecycle writes its cooldown. Conditional
		// cooldown rewards remain deterministic rather than relying on a fragile
		// post-end mutation.
		virtual float ResolveCooldownDurationOnEnd(
			GameAbilityBehaviorContext& context,
			sas::AbilityEndReason reason,
			float resolvedCooldown
		)
		{
			(void)context;
			(void)reason;
			return resolvedCooldown;
		}
	};

	using GameAbilityBehaviorRegistry = sas::AbilityBehaviorRegistry<GameAbilityBehavior,AbilityBehaviorType>;

	class GameAbility: public sas::GameplayAbilityInstance<GameAbilityDefinition,GameAbilityExecution>
	{
	public:
		std::string DebugActivationBlockReason() const;
		GameAbility(
			LightYearsAbilitySystemComponent& abilitySystem,
			sas::AbilityHandle handle,
			const GameAbilityDefinition& definition,
			unique_ptr<GameAbilityBehavior> behavior,
			bool emitNotifications = true
		);

		void RefundActivationCooldown(float fraction);
		const std::shared_ptr<DamageHitGroup>& GetDamageHitGroup() const { return mDamageHitGroup; }
		bool IsInputHeld() const { return this->mRuntimeState.IsInputHeld(); }
		bool IsPressedThisFrame() const { return this->mRuntimeState.IsPressedThisFrame(); }
		void SetActivationOrigin(sas::AbilityActivationOrigin origin)
		{
			mActivationOrigin = origin;
		}
		// Echo and other recorded invocations already materialize their level
		// modifiers into the invocation definition. They still need the source
		// level exposed to behavior code without running a second level rebuild.
		void ConfigureInvocationLevel(int level, int maximumLevel);
		sas::AbilityActivationOrigin GetActivationOrigin() const
		{
			return mActivationOrigin;
		}

		float GetWeaponFireIntervalRemaining() const { return mPrimaryWeaponState.runtime.fireIntervalRemaining; }
		void SetWeaponFireIntervalRemaining(float interval);
		PrimaryWeaponRuntimeState& GetPrimaryWeaponRuntime() { return mPrimaryWeaponState.runtime; }
		const sas::GameplayAttributeList& GetPrimaryWeaponRuntimeAttributes() const
		{
			return mPrimaryWeaponState.attributes;
		}
		void UpdatePrimaryWeaponRuntimeContext(
			const PrimaryWeaponDefinition& weaponDefinition,
			const sas::GameplayAttributeList& attributes,
			const List<GameplayTag>& damageTags
		);
		uint64_t GetConfigurationRevision() const { return mConfigurationRevision; }
		List<GameplayTag> GetResolvedDamageTags() const;
		void HandleGameplayEvent(const sas::AbilityEvent& event);
		void HandleAbilityLifecycleEvent(const sas::AbilityLifecycleEvent& event);
		void DeferActiveDurationStart()
		{
			mDeferActiveDurationStart = true;
		}

	private:
		friend class LightYearsAbilitySystemComponent;
		friend class GameAbilityActionExecutor;
		std::shared_ptr<DamageHitGroup> mDamageHitGroup;

		bool CanActivateContent() const override;
		bool ActivateContent() override;
		void AbortActivationContent() override;
		void OnActivationCommitted() override;
		void BeginExecution() override;
		void TickExecution(float deltaTime) override;
		void EndExecution(sas::AbilityEndReason reason) override;
		void TickInactive(float deltaTime) override;
		void EndContent(sas::AbilityEndReason reason) override;
		bool HasPendingContentCleanup() const override { return mBehaviorCleanupPending; }
		void RetryContentCleanup() override;
		void OnInstanceOperationCompleted() override;
		sas::AbilityLifecycleEvent BuildLifecycleEvent(
			const GameplayTag& eventTag,
			sas::AbilityEndReason endReason
		) const;
		void NotifyOwnerAbilityActivated(const sas::AbilityLifecycleEvent& event);
		int GetMaximumLevel() const override;
		float ResolveCooldownDuration() const override;
		float ResolveCooldownDurationForStart() const override;
		float ResolveCooldownDurationOnEnd(sas::AbilityEndReason reason) override;
		float ResolveActiveDuration() const override;
		bool PrepareDefinitionForLevel(int level) override;
		void CommitPreparedDefinitionForLevel() noexcept override;
		void RebuildDefinitionForLevel() override;
		void OnLevelConfigurationChanged() override;
		bool HandleInputPressed() override;
		bool ShouldDeferActiveDurationStart() const override
		{
			return mDeferActiveDurationStart;
		}

		void RefreshScopedConfiguration();
		float ResolveCooldownDurationWithModuleMultiplier(float moduleMultiplier) const;
		void EndBehavior(sas::AbilityEndReason reason);
		void TickInactivePrimaryWeaponRuntime(float deltaTime);
		GameAbilityDefinition BuildDefinitionForLevel(int level) const;
		void RefreshPrimaryWeaponRuntimeConfiguration();
		LightYearsAbilitySystemComponent& mAbilitySystem;
		PrimaryWeaponAbilityState mPrimaryWeaponState;
		std::optional<GameAbilityDefinition> mPreparedLevelDefinition;
		mutable std::optional<float> mCooldownModuleMultiplierSnapshot;
		std::optional<float> mAbilityModuleCooldownMultiplierSnapshot;
		unique_ptr<GameAbilityBehavior> mBehavior;
		float mPendingActivationCooldownMultiplier = 1.f;
		bool mBehaviorStarted = false;
		bool mBehaviorCleanupPending = false;
		bool mEndLifecycleNotificationAttempted = false;
		sas::AbilityEndReason mBehaviorEndReason = sas::AbilityEndReason::Interrupted;
		sas::AbilityActivationOrigin mActivationOrigin =
			sas::AbilityActivationOrigin::NormalInput;
		int mInvocationMaximumLevel = 0;
		bool mDeferActiveDurationStart = false;
		uint64_t mConfigurationRevision = 0;
	};
}
