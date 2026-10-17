#pragma once

#include "gameplay/ability/GameAbility.h"

namespace vega
{
	class AegisReaverAbility final : public GameAbilityBehavior
	{
	public:
		bool Activate(GameAbilityBehaviorContext& context) override;
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason
		) const override;
	};
}
