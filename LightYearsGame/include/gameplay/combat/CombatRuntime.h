#pragma once

#include "gameplay/attributes/AttributeSystem.h"
#include "gameplay/effects/GameplayEffectSystem.h"
#include "gameplay/ability/AbilitySystem.h"
#include "gameplay/damage/DamageContext.h"
#include "framework/Delegate.h"

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

		AttributeSystem& GetAttributes() { return mAttributeSystem; }
		const AttributeSystem& GetAttributes() const { return mAttributeSystem; }
		GameplayEffectSystem& GetEffects() { return mEffectSystem; }
		const GameplayEffectSystem& GetEffects() const { return mEffectSystem; }
		AbilitySystem& GetAbilities() { return mAbilitySystem; }
		const AbilitySystem& GetAbilities() const { return mAbilitySystem; }
		GameplayTagContainer& GetOwnedTags() { return mOwnedTags; }
		const GameplayTagContainer& GetOwnedTags() const { return mOwnedTags; }

		void ProcessIncomingDamage(DamageContext& context);
		void NotifyDamageResolved(const DamageContext& context);

		Delegate<const DamageContext&> onDamageProcessed;
		Delegate<const DamageContext&> onDamageResolved;

	private:
		Actor* mOwner = nullptr;
		AttributeSystem mAttributeSystem;
		GameplayTagContainer mOwnedTags;
		GameplayEffectSystem mEffectSystem;
		AbilitySystem mAbilitySystem;
	};
}


