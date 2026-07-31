#pragma once

#include "gameplay/ability/GameAbility.h"

namespace ly
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
