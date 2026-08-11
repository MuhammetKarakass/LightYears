#pragma once

#include "gameplay/ability/GameAbility.h"
#include "effects/GameplayEffectRuntimeEntry.h"

namespace ly
{
	class PhaseDriftVisualActor;

	class PhaseDriftAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
			) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
		void End(GameAbilityBehaviorContext& context, sas::AbilityEndReason reason) override;

		float ResolveActiveDuration(
			const GameAbilityBehaviorContext& context,
			float defaultDuration
		) const override;

		void OnOwnerAbilityActivated(
			GameAbilityBehaviorContext& context,
			const sas::AbilityLifecycleEvent& event
		) override;

	private:
		float ResolveDuration(
			const GameAbilityBehaviorContext& context
		) const;
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;
		void RemoveAppliedEffects(GameAbilityBehaviorContext& context);

		List<sas::GameplayEffectHandle> mAppliedEffectHandles;
		weak_ptr<PhaseDriftVisualActor> mVisualActor;
		CollisionLayer mOriginalCollisionLayer = CollisionLayer::None;
		CollisionLayer mOriginalCollisionMask = CollisionLayer::None;
		float mResolvedDuration = 0.f;
		bool mCollisionSnapshotValid = false;
		bool mActive = false;
	};
}
