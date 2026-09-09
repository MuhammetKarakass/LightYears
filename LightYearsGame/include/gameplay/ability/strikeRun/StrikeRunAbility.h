#pragma once

#include "gameplay/ability/GameAbility.h"

namespace ly
{
	class StrikeRunBombardmentActor;

	class StrikeRunAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
		bool OnInputPressed(GameAbilityBehaviorContext& context) override;
		void Tick(GameAbilityBehaviorContext& context, float deltaTime) override;
		void End(
			GameAbilityBehaviorContext& context,
			sas::AbilityEndReason reason
		) override;
		float ResolveCooldownDurationOnEnd(
			GameAbilityBehaviorContext& context,
			sas::AbilityEndReason reason,
			float resolvedCooldown
		) override;

	private:
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;
		void UpdatePreviewDirection(GameAbilityBehaviorContext& context) const;
		void ClearTargetingState(GameAbilityBehaviorContext& context) const;

		weak_ptr<StrikeRunBombardmentActor> mBombardmentActor;
		float mTargetingElapsed = 0.f;
		float mTargetingWindow = 0.f;
		bool mWaitingForDirection = false;
		bool mConfirmed = false;
	};
}

