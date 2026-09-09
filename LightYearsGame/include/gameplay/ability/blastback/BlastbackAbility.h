#pragma once

#include "gameplay/ability/GameAbility.h"

#include <memory>

namespace ly
{
	class BlastbackFocusTelegraphActor;

	class BlastbackAbility final : public GameAbilityBehavior
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
		void Discharge(GameAbilityBehaviorContext& context);
		void ApplyStun(
			GameAbilityBehaviorContext& context,
			class SpaceShip& target,
			float baseDuration
		) const;
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		weak_ptr<BlastbackFocusTelegraphActor> mTelegraph;
		float mFocusElapsed = 0.f;
		bool mDischarged = false;
		bool mFocusLocksApplied = false;
	};
}
