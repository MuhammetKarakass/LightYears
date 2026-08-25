#pragma once

#include "gameplay/ability/GameAbility.h"
#include "effects/GameplayEffectRuntimeEntry.h"

namespace ly
{
	class DirectionalBarrierVisualActor;

	class DirectionalBarrierAbility final : public GameAbilityBehavior
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
			const GameAbilityBehaviorContext& context,
			float defaultDuration
		) const;
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		sas::GameplayEffectHandle mActiveEffectHandle;
		weak_ptr<DirectionalBarrierVisualActor> mVisualActor;
		bool mActive = false;
	};
}
