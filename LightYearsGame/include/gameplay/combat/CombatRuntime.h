#pragma once

#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "abilities/AbilityEvent.h"
#include "gameplay/damage/DamageContext.h"
#include "framework/Delegate.h"
#include "gameplay/combat/ContactDamageGuardRegistry.h"
#include "gameplay/combat/CombatRuntimeModifiers.h"
#include "gameplay/module/ModuleResolver.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace ly
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
		OutgoingDamageRuleRegistration RegisterOutgoingDamageRules();
		void ActivateOutgoingDamageRules(const ModuleTriggerEventContext& event);
		// Product of every active Fire-phase rule (see ModuleDamageRulePhase) for this shot.
		float ResolveFullMagazineShotMultiplier(int magazineShotsFired) const;
		OutgoingDamagePlan ResolveOutgoingDamagePlan(const Actor& target, const List<GameplayTag>& sourceAbilityTags, const sas::ContentId& sourceAbilityId = {}, DamageDeliveryType deliveryType = DamageDeliveryType::Direct) const;

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

	private:
		static bool SameDamageRule(const ModuleDamageRule& left, const ModuleDamageRule& right);
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
		std::shared_ptr<ModuleDamageRuleState> mModuleDamageRuleState = std::make_shared<ModuleDamageRuleState>();
	};
}


