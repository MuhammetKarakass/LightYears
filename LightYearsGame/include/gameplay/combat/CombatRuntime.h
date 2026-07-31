#pragma once

#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "abilities/AbilityEvent.h"
#include "gameplay/damage/DamageContext.h"
#include "framework/Delegate.h"
#include "presentation/effects/GameplayEffectPresentationBinding.h"

namespace ly
{
	class Actor;

	class CombatRuntime
	{
	public:
		explicit CombatRuntime(Actor& owner);

		void InitializeOwnerAttributes(float maxHealth);
		float GetCriticalChance() const;
		float GetCombatLuckFactor() const;
		void Tick(float deltaTime);
		void Clear();

		LightYearsAbilitySystemComponent& GetAbilitySystemComponent()
		{
			return mAbilitySystemComponent;
		}
		const LightYearsAbilitySystemComponent& GetAbilitySystemComponent() const
		{
			return mAbilitySystemComponent;
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
		List<sas::AbilityEvent> mPendingEffectEvents;
		const DamageContext* mProcessingDamageContext = nullptr;
	};
}


