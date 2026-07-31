#pragma once

#include "attributes/AttributeSystem.h"
#include "AbilitySystemComponent.h"

#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/damage/DamageContext.h"

namespace ly
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
			const DamageContext& context
		);

		bool RegisterDamageEffectBehaviors();
	}
}
