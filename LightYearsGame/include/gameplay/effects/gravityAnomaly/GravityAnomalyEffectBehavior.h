#pragma once

#include "gameConfigs/combat/EffectStructs.h"
#include "gameplay/effects/ActiveGameplayEffect.h"
#include "gameplay/effects/GameplayEffectBehavior.h"

#include <SFML/System/Vector2.hpp>

namespace ly
{
	class GravityAnomalyRuntimeContext final : public GameplayEffectRuntimeContext
	{
	public:
		sf::Vector2f center{ 0.f, 0.f };
		float resolvedRadius = 0.f;
		float resolvedPullStrength = 0.f;
		bool pullEnabled = true;
		const void* sourceFieldScope = nullptr;
	};

	namespace GravityAnomalyEffectBehavior
	{
		GameplayEffectBehaviorResult Tick(
			ActiveGameplayEffect& effect,
			Actor& owner,
			float deltaTime
		);
		bool RegisterGravityAnomalyEffectBehavior();
	}
}
