#pragma once

#include "attributes/AttributeSystem.h"
#include "AbilitySystemComponent.h"

#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/damage/DamageContext.h"

namespace vega
{
	namespace DamageTypeSystem
	{
		DamagePayload BuildPayload(
			const List<GameplayTag>& damageTags,
			const sas::GameplayAttributeList& sourceAttributes = {}
		);

		// Applies generic duration/stacking effects and returns only the statuses
		// that were actually applied by this hit.
		List<GameplayTag> ApplyStatusEffects(
			sas::AbilitySystemComponent& targetAbilitySystem,
			DamageContext& context
		);

		// Fraction of the target's Armor removed by its current Kinetic stacks.
		// Applies to every incoming hit regardless of source or damage type.
		float ResolveArmorShred(const sas::AbilitySystemComponent& targetAbilitySystem);

		void SynchronizeStatusEffect(
			sas::AbilitySystemComponent& targetAbilitySystem,
			sas::ActiveGameplayEffect& effect
		);

		bool RegisterDamageEffectBehaviors();
	}
}
