#include "presentation/effects/gravityAnomaly/GravityAnomalyEffectVisualContent.h"

#include "framework/World.h"
#include "presentation/effects/GameplayEffectVisualRegistry.h"
#include "presentation/effects/gravityAnomaly/GravityAnomalyEffectVisual.h"

namespace ly
{
	bool RegisterGravityAnomalyEffectVisuals()
	{
		static const bool registered = []
		{
			const GravityAnomalyEffectVisualDefinition definition;
			return GameplayEffectVisualRegistry::RegisterFactory(
				GravityAnomalyEffectVisualIds::Inside,
				[definition](Actor& owner) -> weak_ptr<GameplayEffectVisual>
				{
					World* world = owner.GetWorld();
					return world
						? world->SpawnActor<GravityAnomalyEffectVisual>(&owner, definition)
						: weak_ptr<GameplayEffectVisual>{};
				}
			);
		}();
		return registered;
	}
}
