#pragma once

#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "abilities/AbilityEvent.h"
#include "gameplay/damage/DamageContext.h"
#include "framework/Delegate.h"
#include "gameplay/combat/ContactDamageGuardRegistry.h"
#include "gameplay/combat/CombatRuntimeModifiers.h"
#include "gameplay/damage/StatusStacks.h"
#include "gameplay/combat/LethalHitInterceptors.h"
#include "gameplay/module/ModuleResolver.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace vega
{
	class Actor;
	class GameplayEffectPresentationBinding;

	class CombatRuntime
	{
	private:
		struct ActiveModuleDamageRule
		{
			ResolvedModuleDamageRule resolved;
			float remainingSeconds = 0.f;
			float powerMultiplier = 1.f;
		};
		struct ModuleDamageRuleRegistrationEntry
		{
			std::uint64_t generation = 0;
			List<ActiveModuleDamageRule> rules;
		};
		struct ModuleDamageRuleState
		{
			std::uint64_t nextGeneration = 1;
			List<ModuleDamageRuleRegistrationEntry> registrations;
		};

	public:
		struct OutgoingDamageBonus
		{
			float fraction = 0.f;
			GameplayTag damageType;
		};
		struct OutgoingDamagePlan
		{
			float damageMultiplier = 1.f;
			List<OutgoingDamageBonus> bonuses;
			float additionalDamage = 0.f;
			std::shared_ptr<ModuleFirstHitHistory::Claim> firstHitClaim;
		};

		class OutgoingDamageRuleRegistration final
		{
		public:
			OutgoingDamageRuleRegistration() = default;
			~OutgoingDamageRuleRegistration();
			OutgoingDamageRuleRegistration(const OutgoingDamageRuleRegistration&) = delete;
			OutgoingDamageRuleRegistration& operator=(const OutgoingDamageRuleRegistration&) = delete;
			OutgoingDamageRuleRegistration(OutgoingDamageRuleRegistration&& other) noexcept;
			OutgoingDamageRuleRegistration& operator=(OutgoingDamageRuleRegistration&& other) noexcept;

			void ReplaceRules(const List<ResolvedModuleDamageRule>& rules);
			void Reset() noexcept;
			bool IsValid() const noexcept { return mGeneration != 0 && !mState.expired(); }

		private:
			friend class CombatRuntime;
			OutgoingDamageRuleRegistration(std::weak_ptr<ModuleDamageRuleState> state, std::uint64_t generation)
				: mState{ std::move(state) }, mGeneration{ generation } {}

			std::weak_ptr<ModuleDamageRuleState> mState;
			std::uint64_t mGeneration = 0;
		};

		explicit CombatRuntime(Actor& owner);
		~CombatRuntime();

		void InitializeOwnerAttributes(float maxHealth);
		float GetCriticalChance() const;
		float GetCriticalDamageMultiplier() const;
		float GetCombatLuckFactor() const;
		void Tick(float deltaTime);
		// Pending when the owner ability system deferred or could not finish its clear (retained
		// ability cleanup); it stays requested and retries on its next operation. Callers that are
		// destroying the owner may drop Pending: the retained state dies with the owner.
		sas::ClearResult Clear();

		bool SetRuntimeModifier(const std::string& sourceId, CombatRuntimeModifier modifier);
		bool RemoveRuntimeModifier(const std::string& sourceId);
		float GetOutgoingDamageMultiplier() const noexcept;
		// Primary-shot-only outgoing multiplier (Escape Velocity sonic state). Applied to the whole Primary hit in
		// ResolveOutgoingDamagePlan; never to ability hits or periodic ticks.
		bool SetPrimaryShotModifier(const std::string& sourceId, float multiplier);
		bool RemovePrimaryShotModifier(const std::string& sourceId);
		float GetPrimaryShotDamageMultiplier() const noexcept;
		OutgoingDamageRuleRegistration RegisterOutgoingDamageRules();
		bool ForceModuleDamageWindows(std::uint32_t instanceId, float powerMultiplier);
		void ActivateOutgoingDamageRules(const ModuleTriggerEventContext& event);
		void SetModuleStatusRules(const ModuleStatusRules& rules);
		bool TryActivateElectricSpread(const ModuleElectricSpreadRule& rule, std::uint64_t targetId);
		bool TryActivateModuleStatusPulse(std::uint32_t instanceId, std::size_t ruleIndex, std::uint64_t targetId, float cooldownSeconds);
		const ModuleStatusRules& GetModuleStatusRules() const { return mModuleStatusRules; }
		// Epic Offense: the ship's aggregate rules and its run-time counters (Prism cycle, Storm charges, hit records, blood marks).
		void SetEpicOffenseEffects(const EpicOffenseEffects& effects) { mEpicOffenseEffects = effects; }
		const EpicOffenseEffects& GetEpicOffenseEffects() const { return mEpicOffenseEffects; }
		EpicOffenseState& GetEpicOffenseState() { return mEpicOffenseState; }
		float ResolveEpicOffenseOutgoingMultiplier(Actor& target, const List<GameplayTag>& sourceAbilityTags, const List<GameplayTag>& damageTags, DamageDeliveryType deliveryType, float mainEffectMultiplier = 1.f);
		// Set by the module applier while a generated rule reads the target's statuses at resolution.
		void SetTargetStatusSnapshotRequested(bool requested) { mTargetStatusSnapshotRequested = requested; }
		bool WantsTargetStatusSnapshot() const { return mTargetStatusSnapshotRequested; }
		// Product of every active Fire-phase rule (see ModuleDamageRulePhase) for this shot.
		float ResolveFullMagazineShotMultiplier(int magazineShotsFired) const;
		OutgoingDamagePlan ResolveOutgoingDamagePlan(const Actor& target, const List<GameplayTag>& sourceAbilityTags, const sas::ContentId& sourceAbilityId = {}, DamageDeliveryType deliveryType = DamageDeliveryType::Direct, bool allowFirstTargetHit = true) const;
		std::size_t GetModuleFirstHitTargetCount() const { return mModuleFirstHits.GetTargetCount(); }

		// Abilities can register temporary, source-owned combat protection
		// without making ship classes know the ability family that requested it.
		void SetDamageProtection(
			const std::string& sourceId,
			bool blocksIncomingDamage,
			bool blocksOutgoingDamage
		);
		void RemoveDamageProtection(const std::string& sourceId);
		bool BlocksIncomingDamage() const;
		bool BlocksOutgoingDamage() const;

		LightYearsAbilitySystemComponent& GetAbilitySystemComponent()
		{
			return mAbilitySystemComponent;
		}
		const LightYearsAbilitySystemComponent& GetAbilitySystemComponent() const
		{
			return mAbilitySystemComponent;
		}

		// Lethal hits this combatant deals (ship targets) pass through these before they kill.
		LethalHitInterceptorList& GetLethalHitInterceptors() { return mLethalHitInterceptors; }
		const LethalHitInterceptorList& GetLethalHitInterceptors() const { return mLethalHitInterceptors; }
		// Target-side prevention is resolved before the damage source's interceptors.
		LethalHitInterceptorList& GetOwnerDeathPreventions() { return mOwnerDeathPreventions; }
		const LethalHitInterceptorList& GetOwnerDeathPreventions() const { return mOwnerDeathPreventions; }

		ContactDamageGuardRegistry& GetContactDamageGuardRegistry()
		{
			return mContactDamageGuardRegistry;
		}
		const ContactDamageGuardRegistry& GetContactDamageGuardRegistry() const
		{
			return mContactDamageGuardRegistry;
		}

		void ProcessIncomingDamage(DamageContext& context);
		void ApplyHullDamageMitigation(DamageContext& context);
		void NotifyDamageResolved(const DamageContext& context);

		Delegate<const DamageContext&> onDamageProcessed;
		Delegate<const DamageContext&> onDamageResolved;
		// Broadcast on the SOURCE combatant's runtime when it dealt applied damage.
		Delegate<const DamageContext&> onDamageDealt;
		// Broadcast on the SOURCE combatant's runtime when one of its hits added new damage-status stacks to a target.
		Delegate<const StatusStacksAddedEvent&> onStatusStacksAdded;

	private:
		static void ReplaceRegisteredDamageRules(
			ModuleDamageRuleState& state,
			std::uint64_t generation,
			const List<ResolvedModuleDamageRule>& rules
		);
		static void UnregisterDamageRules(ModuleDamageRuleState& state, std::uint64_t generation);
		void CompleteClear();
		bool mClearRequested = false;
		struct QueuedEffectEvent
		{
			GameplayTag eventTag;
			float magnitude = 0.f;
			std::weak_ptr<Actor> source;
			std::weak_ptr<Actor> target;
			bool hasSource = false;
			std::uint64_t generation = 0;
		};
		struct DamageDispatchFrame
		{
			DamageContext* context = nullptr;
			std::uint64_t generation = 0;
			bool capturesEffectEvents = false;
			List<QueuedEffectEvent> effectEvents;
			std::shared_ptr<Actor> sourceLifetime;
			std::shared_ptr<Actor> targetLifetime;
			std::shared_ptr<Actor> deliveryActorLifetime;
			std::shared_ptr<Actor> ownerLifetime;
		};
		struct ScopedDamageDispatchFrame
		{
			ScopedDamageDispatchFrame(CombatRuntime& runtime, DamageDispatchFrame& frame);
			~ScopedDamageDispatchFrame();
			CombatRuntime& runtime;
			DamageDispatchFrame* previousFrame = nullptr;
		};
		static std::shared_ptr<Actor> LockActor(Actor* actor);
		void QueueEffectEvent(const sas::GameplayEffectBehaviorEvent& event);
		void DispatchPendingEffectEvents();
		void DispatchDamageEffectEvents(DamageDispatchFrame& frame);
		void DispatchEffectEvent(
			const QueuedEffectEvent& queuedEvent,
			const DamageContext* context,
			std::uint64_t batchGeneration
		);

		Actor& mOwner;
		LightYearsAbilitySystemComponent mAbilitySystemComponent;
		std::unique_ptr<GameplayEffectPresentationBinding> mEffectPresentation;
		ContactDamageGuardRegistry mContactDamageGuardRegistry;
		LethalHitInterceptorList mLethalHitInterceptors;
		LethalHitInterceptorList mOwnerDeathPreventions;
		List<QueuedEffectEvent> mPendingEffectEvents;
		DamageDispatchFrame* mCurrentDamageDispatchFrame = nullptr;
		std::uint64_t mEffectEventGeneration = 1;
		struct DamageProtection
		{
			bool blocksIncomingDamage = false;
			bool blocksOutgoingDamage = false;
		};
		std::unordered_map<std::string, DamageProtection> mDamageProtections;
		CombatRuntimeModifiers mRuntimeModifiers;
		std::unordered_map<std::string, float> mPrimaryShotModifiers;
		ModuleStatusRules mModuleStatusRules;
		EpicOffenseEffects mEpicOffenseEffects;
		EpicOffenseState mEpicOffenseState;
		struct ModuleStatusPulseCooldown { std::uint32_t instanceId; std::size_t ruleIndex; std::uint64_t targetId; float remaining; };
		List<ModuleStatusPulseCooldown> mModuleStatusPulseCooldowns;
		bool mTargetStatusSnapshotRequested = false;
		ModuleFirstHitHistory mModuleFirstHits;
		std::shared_ptr<ModuleDamageRuleState> mModuleDamageRuleState = std::make_shared<ModuleDamageRuleState>();
	};
}


