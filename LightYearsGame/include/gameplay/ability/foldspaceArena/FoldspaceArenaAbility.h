#pragma once

#include "gameplay/ability/GameAbility.h"

namespace ly
{
	class FoldspaceArenaActor;

	// Uses the same active/cancel lifecycle as Ironclad Protocol. The independent
	// arena actor owns spatial gameplay; this behavior only owns ability state.
	class FoldspaceArenaAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;
		bool Activate(GameAbilityBehaviorContext& context) override;
		bool OnInputPressed(GameAbilityBehaviorContext& context) override;
		void Tick(GameAbilityBehaviorContext& context, float deltaTime) override;
		void End(GameAbilityBehaviorContext& context, sas::AbilityEndReason reason) override;

	private:
		void ClearRuntimeState(GameAbilityBehaviorContext& context);
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		weak_ptr<FoldspaceArenaActor> mArena;
		float mElapsed = 0.f;
		float mMinimumArenaDuration = 1.f;
		bool mCancelAvailable = false;
		bool mActive = false;
	};
}
