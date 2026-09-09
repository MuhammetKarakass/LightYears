#pragma once

#include "gameplay/ability/GameAbility.h"

#include <cstddef>

namespace ly
{
	class EmberDroneActor;

	class EmberSwarmAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
		void Tick(GameAbilityBehaviorContext& context, float deltaTime) override;
		void End(
			GameAbilityBehaviorContext& context,
			sas::AbilityEndReason reason
		) override;

		float ResolveActiveDuration(
			const GameAbilityBehaviorContext& context,
			float defaultDuration
		) const override;

	private:
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;
		void DestroyDrones();
		void EvaluateTargets(GameAbilityBehaviorContext& context);

		List<weak_ptr<EmberDroneActor>> mDrones;
		float mTargetEvaluationTimer = 0.f;
		bool mActive = false;
	};
}
