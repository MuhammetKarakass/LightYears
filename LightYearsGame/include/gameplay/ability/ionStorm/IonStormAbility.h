#pragma once

#include "gameplay/ability/GameAbility.h"

namespace ly
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
