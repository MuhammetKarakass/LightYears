#include "gameplay/effects/gravityAnomaly/GravityAnomalyEffectBehavior.h"

#include "gameConfigs/ability/control/GravityAnomalyConfig.h"
#include "gameplay/effects/EffectBehaviorKeys.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/movement/MovementInfluenceService.h"
#include "gameplay/tags/GameplayTags.h"
#include "attributes/GameplayAttribute.h"
#include "framework/Actor.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace GravityAnomalyEffectBehavior
	{
		namespace
		{
			// The pull weakens toward the field edge but never below this share.
			constexpr float MinimumPullFactor = 0.5f;
		}

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
			const float falloff = 1.f - normalizedDistance;
			float pullFactor = MinimumPullFactor +
				(1.f - MinimumPullFactor) * falloff * falloff;
			if (const auto* combatant = dynamic_cast<const Combatant*>(&owner))
			{
				const ControlResponse response = combatant->ResolveControlResponse(
					GameplayTags::State::Effect::Movement::Slow
				);
				pullFactor *= response.mode == ControlResponseMode::Immune
					? 0.f
					: std::clamp(response.durationMultiplier, 0.f, 1.f);
			}
			const sf::Vector2f direction = delta / distance;
			const sf::Vector2f acceleration = direction *
				context->resolvedPullStrength * pullFactor;
			if (!std::isfinite(acceleration.x) || !std::isfinite(acceleration.y))
			{
				return {};
			}

			movement::MovementInfluenceService::ApplyInstantAcceleration(
				owner,
				acceleration,
				deltaTime
			);
			return {};
		}

		bool RegisterGravityAnomalyEffectBehavior()
		{
			static const bool registered = []
			{
				LightYearsEffectBehaviorRuntime::Hooks hooks;
				hooks.tick = &Tick;
				return GetEffectBehaviorRuntime().Register(
					EffectBehaviorKeys::GravityAnomaly,
					hooks
				);
			}();
			return registered;
		}
	}
}
