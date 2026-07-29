#include "gameplay/effects/GameplayEffectContent.h"

#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/effects/BarrierEffectBehavior.h"
#include "gameplay/effects/GameplayEffectValidation.h"
#include "gameplay/effects/gravityAnomaly/GravityAnomalyEffectBehavior.h"
#include "presentation/effects/gravityAnomaly/GravityAnomalyEffectVisualContent.h"
#include "presentation/effects/shield/ShieldVisualContent.h"

namespace ly
{
	bool RegisterGameGameplayEffectContent()
	{
		static const bool registered =
			RegisterGravityAnomalyEffectVisuals()
			&& RegisterShieldVisuals()
			&& BarrierEffectBehavior::RegisterBarrierEffectBehavior()
			&& DamageTypeSystem::RegisterDamageEffectBehaviors()
			&& GravityAnomalyEffectBehavior::RegisterGravityAnomalyEffectBehavior()
			&& ValidateShippedGameplayEffectDefinitions();
		return registered;
	}
}
