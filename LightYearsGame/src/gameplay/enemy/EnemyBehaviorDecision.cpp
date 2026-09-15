#include "gameplay/enemy/EnemyBehaviorDecision.h"

#include "framework/MathUtility.h"

#include <cmath>

namespace ly
{
	namespace
	{
		sf::Vector2f NormalizeOrZero(const sf::Vector2f& value)
		{
			const float length = GetVectorLength(value);
			return std::isfinite(length) && length > 0.001f ? value / length : sf::Vector2f{};
		}
	}

	sf::Vector2f ResolveEnemyMovementDirection(const EnemyBehaviorDecisionContext& context)
	{
		const sf::Vector2f radial = NormalizeOrZero(context.radialDirection);
		switch (context.movementMode)
		{
		case EnemyMovementMode::Approach:
			if (context.distance > context.desiredDistance) return radial;
			return context.minimumDistance > 0.f && context.distance < context.minimumDistance ? -radial : sf::Vector2f{};
		case EnemyMovementMode::HoldRange:
			if (context.distance > context.maximumDistance) return radial;
			return context.distance < context.minimumDistance ? -radial : sf::Vector2f{};
		case EnemyMovementMode::Strafe:
		{
			sf::Vector2f movement;
			if (context.distance > context.maximumDistance) movement = radial;
			else if (context.distance < context.minimumDistance) movement = -radial;
			const sf::Vector2f lateral{ -radial.y * context.strafeDirection, radial.x * context.strafeDirection };
			return NormalizeOrZero(movement + lateral * 0.75f);
		}
		default: return {};
		}
	}

	bool IsEnemyInputModeCompatible(sas::AbilityActivationPolicy policy, EnemySlotInputMode inputMode)
	{
		switch (policy)
		{
		case sas::AbilityActivationPolicy::WhileHeld: return inputMode == EnemySlotInputMode::Hold;
		case sas::AbilityActivationPolicy::OnPressed: return inputMode == EnemySlotInputMode::Pulse;
		default: return false;
		}
	}

	bool ShouldActivateEnemySlot(const EnemySlotDecisionRule& rule, const EnemySlotDecisionContext& context)
	{
		if (!rule.requiresTarget) return true;
		return context.hasTarget &&
			std::isfinite(context.distance) &&
			std::isfinite(context.alignment) &&
			context.distance >= rule.minimumRange &&
			context.distance <= rule.maximumRange &&
			context.alignment >= rule.aimConeThreshold;
	}
}
