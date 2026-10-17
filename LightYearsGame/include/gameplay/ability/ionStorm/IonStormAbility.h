#pragma once

#include "gameplay/ability/GameAbility.h"

namespace vega
{
	class IonStormAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason
		) const override;
	};
}
