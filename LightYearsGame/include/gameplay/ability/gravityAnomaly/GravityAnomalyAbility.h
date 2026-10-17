#pragma once

#include "gameplay/ability/GameAbility.h"

namespace vega
{
	class GravityAnomalyAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;
	};
}
