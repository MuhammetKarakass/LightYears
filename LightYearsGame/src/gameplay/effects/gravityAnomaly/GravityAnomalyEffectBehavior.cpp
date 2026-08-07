#include "gameplay/effects/gravityAnomaly/GravityAnomalyEffectBehavior.h"

#include "gameConfigs/ability/offensive/GravityAnomalyConfig.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"
#include "attributes/GameplayAttribute.h"
#include "framework/Actor.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace GravityAnomalyEffectBehavior
	{
		sas::GameplayEffectBehaviorResult Tick(
			sas::ActiveGameplayEffect& effect,
			Actor& owner,
			float deltaTime
		)
		{
			const std::shared_ptr<GravityAnomalyRuntimeContext> context =
				std::dynamic_pointer_cast<GravityAnomalyRuntimeContext>(effect.runtimeContext);
			if (!context || !context->pullEnabled || context->resolvedRadius <= 0.f ||
				context->resolvedPullStrength <= 0.f || deltaTime <= 0.f)
			{
				return {};
			}

			const sf::Vector2f delta = context->center - owner.GetActorLocation();
			const float distanceSquared = delta.x * delta.x + delta.y * delta.y;
			if (!std::isfinite(distanceSquared) || distanceSquared <= 0.000001f)
			{
				return {};
			}

			const float distance = std::sqrt(distanceSquared);
			if (!std::isfinite(distance) || distance <= 0.001f)
			{
				return {};
			}

			const float normalizedDistance = std::clamp(
				distance / context->resolvedRadius,
				0.f,
				1.f
			);
			const float pullFactor = (1.f - normalizedDistance) * (1.f - normalizedDistance);
			const sf::Vector2f direction = delta / distance;
			const sf::Vector2f velocityChange = direction *
				context->resolvedPullStrength * pullFactor * deltaTime;
			if (!std::isfinite(velocityChange.x) || !std::isfinite(velocityChange.y))
			{
				return {};
			}

			owner.SetVelocity(owner.GetVelocity() + velocityChange);
			return {};
		}

		bool RegisterGravityAnomalyEffectBehavior()
		{
			static const bool registered = []
			{
				LightYearsEffectBehaviorRuntime::Hooks hooks;
				hooks.tick = &Tick;
				return GetEffectBehaviorRuntime().Register(
					AbilityData::GravityAnomaly::Effect::BehaviorTag,
					hooks
				);
			}();
			return registered;
		}
	}
}
