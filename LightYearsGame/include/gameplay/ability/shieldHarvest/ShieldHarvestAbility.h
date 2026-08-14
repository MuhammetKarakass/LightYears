#pragma once

#include "gameplay/ability/GameAbility.h"

namespace ly
{
	class AreaTelegraphActor;

	class ShieldHarvestAbility final : public GameAbilityBehavior
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
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		weak_ptr<AreaTelegraphActor> mTelegraph;
		float mFocusElapsed = 0.f;
		bool mHarvested = false;
	};
}
