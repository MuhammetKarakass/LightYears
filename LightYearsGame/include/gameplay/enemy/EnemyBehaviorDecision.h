#pragma once

#include "gameplay/enemy/EnemyBehaviorProfile.h"

#include <SFML/System/Vector2.hpp>

namespace ly
{
	struct EnemyBehaviorDecisionContext
	{
		EnemyMovementMode movementMode = EnemyMovementMode::Approach;
		float distance = 0.f;
		float desiredDistance = 0.f;
		float minimumDistance = 0.f;
		float maximumDistance = 0.f;
		float strafeDirection = 1.f;
		sf::Vector2f radialDirection{};
	};

	struct EnemySlotDecisionContext
	{
		bool hasTarget = false;
		float distance = 0.f;
		float alignment = 0.f;
	};

	sf::Vector2f ResolveEnemyMovementDirection(const EnemyBehaviorDecisionContext& context);
	bool IsEnemyInputModeCompatible(sas::AbilityActivationPolicy policy, EnemySlotInputMode inputMode);
	bool ShouldActivateEnemySlot(const EnemySlotDecisionRule& rule, const EnemySlotDecisionContext& context);
}
