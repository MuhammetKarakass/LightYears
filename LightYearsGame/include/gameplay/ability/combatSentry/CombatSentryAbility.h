#pragma once

#include "gameplay/ability/GameAbility.h"

namespace ly
{
	// Placement is an instant ability. The turret owns its independent twenty
	// second lifetime, so cooldown begins as soon as the player commits a spawn.
	class CombatSentryAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
	};
}
