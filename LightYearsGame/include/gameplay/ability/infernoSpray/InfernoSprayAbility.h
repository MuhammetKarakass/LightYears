#pragma once

#include "gameplay/ability/GameAbility.h"
#include "effects/ActiveGameplayEffect.h"

namespace ly
{
	class InfernoSprayAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
		void Tick(GameAbilityBehaviorContext& context, float deltaTime) override;
		void End(GameAbilityBehaviorContext& context, sas::AbilityEndReason reason) override;

	private:
		sas::GameplayEffectHandle mMovementPenaltyEffectHandle;
		float mActiveTime = 0.f;
		bool mStarted = false;
	};
}
