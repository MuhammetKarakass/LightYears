#pragma once

#include "gameplay/ability/GameAbility.h"
#include "gameplay/temporal/TemporalStateHistory.h"

namespace ly
{
	class TemporalRecallAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason
		) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
		void Tick(GameAbilityBehaviorContext& context, float deltaTime) override;
		void End(
			GameAbilityBehaviorContext& context,
			sas::AbilityEndReason reason
		) override;

	private:
		enum class Phase
		{
			Idle,
			Focusing,
			Rewinding,
			Completed
		};

		void Complete(GameAbilityBehaviorContext& context);
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		TemporalStateSnapshot mSnapshot;
		sf::Vector2f mResolvedDestination{};
		float mFocusDuration = 0.f;
		float mRewindDuration = 0.f;
		float mElapsed = 0.f;
		Phase mPhase = Phase::Idle;
	};
}
