#pragma once

#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "abilities/AbilityEvent.h"
#include "gameplay/damage/DamageContext.h"
#include "framework/Delegate.h"
#include "presentation/effects/GameplayEffectPresentationBinding.h"
#include "gameplay/combat/ContactDamageGuardRegistry.h"
#include "gameplay/combat/CombatRuntimeModifiers.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace ly
{
	class Actor;

	class CombatRuntime
	{
	public:
		explicit CombatRuntime(Actor& owner);

		void InitializeOwnerAttributes(float maxHealth);
		float GetCriticalChance() const;
		float GetCriticalDamageMultiplier() const;
		float GetCombatLuckFactor() const;
		void Tick(float deltaTime);
		void Clear();

		bool SetRuntimeModifier(const std::string& sourceId, CombatRuntimeModifier modifier);
		bool RemoveRuntimeModifier(const std::string& sourceId);
		float GetOutgoingDamageMultiplier() const noexcept;

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

	private:
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
		GameplayEffectPresentationBinding mEffectPresentation;
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
	};
}


