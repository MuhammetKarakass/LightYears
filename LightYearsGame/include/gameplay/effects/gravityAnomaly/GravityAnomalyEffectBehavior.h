#pragma once

#include "gameConfigs/combat/EffectStructs.h"
#include "effects/ActiveGameplayEffect.h"
#include "effects/GameplayEffectBehaviorResult.h"
#include "effects/GameplayEffectRuntimeEntry.h"

#include <SFML/System/Vector2.hpp>

namespace ly
{
	class Actor;

	class GravityAnomalyRuntimeContext final
		: public sas::GameplayEffectRuntimeContext
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
		sas::GameplayEffectBehaviorResult Tick(
			sas::ActiveGameplayEffect& effect,
			Actor& owner,
			float deltaTime
		);
		bool RegisterGravityAnomalyEffectBehavior();
	}
}
