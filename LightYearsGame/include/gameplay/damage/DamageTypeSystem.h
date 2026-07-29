#pragma once

#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/damage/DamageContext.h"

namespace ly
{
	class GameplayEffectSystem;

	namespace DamageTypeSystem
	{
		DamagePayload BuildPayload(
			const List<GameplayTag>& damageTags,
			const GameplayAttributeList& sourceAttributes = {}
		);

		// Applies generic duration/stacking effects and returns only the statuses
		// that were actually applied by this hit.
		List<GameplayTag> ApplyStatusEffects(
			GameplayEffectSystem& targetEffects,
			const DamageContext& context
		);

		bool RegisterDamageEffectBehaviors();
	}
}
