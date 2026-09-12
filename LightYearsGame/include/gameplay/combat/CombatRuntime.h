#pragma once

#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "abilities/AbilityEvent.h"
#include "gameplay/damage/DamageContext.h"
#include "framework/Delegate.h"
#include "presentation/effects/GameplayEffectPresentationBinding.h"
#include "gameplay/combat/ContactDamageGuardRegistry.h"

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
		void NotifyDamageResolved(const DamageContext& context);

		Delegate<const DamageContext&> onDamageProcessed;
		Delegate<const DamageContext&> onDamageResolved;

	private:
		void QueueEffectEvent(
			const sas::GameplayEffectBehaviorEvent& event,
			const DamageContext* context = nullptr
		);
		void DispatchPendingEffectEvents();

		Actor& mOwner;
		LightYearsAbilitySystemComponent mAbilitySystemComponent;
		GameplayEffectPresentationBinding mEffectPresentation;
		ContactDamageGuardRegistry mContactDamageGuardRegistry;
		List<sas::AbilityEvent> mPendingEffectEvents;
		const DamageContext* mProcessingDamageContext = nullptr;
		struct DamageProtection
		{
			bool blocksIncomingDamage = false;
			bool blocksOutgoingDamage = false;
		};
		std::unordered_map<std::string, DamageProtection> mDamageProtections;
	};
}


