#pragma once

#include "gameplay/ability/AbilityBehavior.h"

namespace ly
{
	class GravityAnomalyAbility final : public AbilityBehavior
	{
	public:
		bool Validate(
			const AbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;
	};
}
