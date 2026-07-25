#pragma once

#include "gameplay/ability/AbilityBehavior.h"

namespace ly
{
	class RocketAbility final : public AbilityBehavior
	{
	public:
		bool Validate(
			const AbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;
	};
}
