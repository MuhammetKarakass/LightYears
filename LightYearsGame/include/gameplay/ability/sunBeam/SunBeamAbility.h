#pragma once

#include "gameplay/ability/AbilityBehavior.h"

namespace ly
{
	class SunBeamAbility final : public AbilityBehavior
	{
	public:
		bool Validate(
			const AbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;
	};
}
