#pragma once

#include "gameplay/ability/GameAbility.h"

namespace ly
{
	class SpaceShip;

	class ExecutionDriveAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
		void End(GameAbilityBehaviorContext& context, sas::AbilityEndReason reason) override;
		void OnGameplayEvent(
			GameAbilityBehaviorContext& context,
			const sas::AbilityEvent& event
		) override;

	private:
		void RefreshPowerEffect(GameAbilityBehaviorContext& context);
		float ResolveChaseMovementMultiplier(
			const sf::Vector2f& movementDirection
		) const;
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		LightYearsAbilitySystemComponent* mAbilitySystem = nullptr;
		SpaceShip* mShip = nullptr;
		sas::GameplayEffectHandle mAttackPowerEffectHandle;
		int mStackCount = 0;
		float mCurrentAttackPowerBonus = 0.f;
		float mBaseAttackPowerBonus = 0.f;
		float mAttackPowerPerStack = 0.f;
		float mBaseChaseMovementBonus = 0.f;
		float mChaseMovementPerStack = 0.f;
		float mAttackPowerChaseScale = 0.f;
		float mTargetingRange = 0.f;
		float mDirectionThreshold = 0.f;
		bool mActive = false;
	};
}
