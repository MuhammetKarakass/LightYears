#include "presentation/effects/shield/ShieldVisualContent.h"

#include "framework/World.h"
#include "presentation/effects/GameplayEffectVisualRegistry.h"
#include "presentation/effects/shield/ShieldVisual.h"
#include "presentation/effects/shield/ShieldVisualIds.h"

namespace ly
{
	namespace
	{
		ShieldVisualDefinition BuildBasicShieldVisual()
		{
			ShieldVisualDefinition definition;
			definition.texturePath = "SpaceShooterRedux/PNG/Effects/shield3.png";
			definition.color = sf::Color{ 110, 205, 255, 145 };
			definition.localOffset = { 0.f, 0.f };
			definition.baseScale = 1.15f;
			definition.rotationSpeed = 35.f;
			definition.pulseSpeed = 5.5f;
			definition.pulseScaleAmount = 0.08f;
			definition.minimumAlpha = 120.f;
			definition.maximumAlpha = 190.f;
			definition.lowIntegrityPulseMultiplier = 1.75f;
			definition.lowIntegrityAlphaMultiplier = 0.65f;
			return definition;
		}
	}

	bool RegisterShieldVisuals()
	{
		static const bool registered = []
		{
			const ShieldVisualDefinition definition = BuildBasicShieldVisual();
			return GameplayEffectVisualRegistry::RegisterFactory(
				ShieldVisualIds::Basic,
				[definition](Actor& owner) -> weak_ptr<GameplayEffectVisual>
				{
					World* world = owner.GetWorld();
					return world
						? world->SpawnActor<ShieldVisual>(&owner, definition)
						: weak_ptr<GameplayEffectVisual>{};
				}
			);
		}();
		return registered;
	}
}
