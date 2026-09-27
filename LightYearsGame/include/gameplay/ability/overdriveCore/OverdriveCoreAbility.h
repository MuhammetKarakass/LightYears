#pragma once

#include "effects/ActiveGameplayEffect.h"
#include "gameplay/ability/GameAbility.h"

#include <cstddef>

namespace ly
{
	class Actor;

	class OverdriveCoreAbility final : public GameAbilityBehavior
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
		using ExecutionContext = AbilityExecutionContext;

		void LaunchRocket(
			GameAbilityBehaviorContext& context,
			ExecutionContext& executionContext
		);
		void FinishRocketLaunch(
			GameAbilityBehaviorContext& context,
			ExecutionContext& executionContext
		);
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;
		void ClearRocketLaunchState(GameAbilityBehaviorContext& context);

		List<weak_ptr<Actor>> mTargetAllocation;
		sas::GameplayEffectHandle mAttackSpeedBoostEffectHandle;
		std::size_t mProjectileCount = 0;
		std::size_t mLaunchedProjectileCount = 0;
		float mRocketLaunchDuration = 0.f;
		float mRocketLaunchElapsed = 0.f;
		bool mRocketLaunchActive = false;
		bool mAttackSpeedBoostActive = false;
	};
}
