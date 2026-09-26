#pragma once

#include "gameplay/ability/GameAbility.h"

#include <memory>

namespace ly
{
	class ChainLightningAbilityLifetimeState;

	class ChainLightningAbility final : public GameAbilityBehavior
	{
	public:
		ChainLightningAbility();
		~ChainLightningAbility() override;

		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;
		bool Activate(GameAbilityBehaviorContext& context) override;

	private:
		std::shared_ptr<ChainLightningAbilityLifetimeState> mLifetimeState;
	};
}
