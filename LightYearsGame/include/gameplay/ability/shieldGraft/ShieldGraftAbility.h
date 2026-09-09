#pragma once

#include "gameplay/ability/GameAbility.h"

namespace ly
{
	class ShieldGraftAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
	};
}
