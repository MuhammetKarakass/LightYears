#pragma once

#include "gameplay/ability/GameAbility.h"

namespace ly
{
	// The ability is instant: it snapshots the cursor and creates an independent
	// world actor. That is why its cooldown begins on cast rather than after the
	// field has finished travelling and controlling enemies.
	class TemporalConvergenceAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;
		bool Activate(GameAbilityBehaviorContext& context) override;
	};
}
