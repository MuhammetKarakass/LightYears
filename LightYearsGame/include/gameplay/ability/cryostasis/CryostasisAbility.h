#pragma once

#include "gameplay/ability/GameAbility.h"

namespace ly
{
	class CryostasisVisualActor;

	class CryostasisAbility final : public GameAbilityBehavior
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
		void OnGameplayEvent(
			GameAbilityBehaviorContext& context,
			const sas::AbilityEvent& event
		) override;
		float ResolveCooldownDurationOnEnd(
			GameAbilityBehaviorContext& context,
			sas::AbilityEndReason reason,
			float resolvedCooldown
		) override;

	private:
		void ApplyFieldTick(GameAbilityBehaviorContext& context);
		void TriggerBreakExplosion(GameAbilityBehaviorContext& context);
		void ClearRuntimeState(GameAbilityBehaviorContext& context);
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		sas::GameplayEffectHandle mIceShellHandle;
		sas::GameplayAttributeList mResolvedValues;
		List<GameplayTag> mDamageTags;
		float mMaximumIceHealth = 0.f;
		float mFieldTickAccumulator = 0.f;
		bool mActive = false;
		bool mIceBroken = false;
		weak_ptr<CryostasisVisualActor> mVisualActor;
	};
}
