#pragma once

#include "effects/GameplayEffectRuntimeEntry.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/runtime/AbilityLifecycleDispatcher.h"
#include "gameplay/weapon/runtime/PrimaryWeaponOverrideState.h"

namespace ly
{
	class IroncladProtocolAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(const GameAbilityDefinition& definition, std::string* failureReason = nullptr) const override;
		bool Activate(GameAbilityBehaviorContext& context) override;
		bool OnInputPressed(GameAbilityBehaviorContext& context) override;
		void Tick(GameAbilityBehaviorContext& context, float deltaTime) override;
		void End(GameAbilityBehaviorContext& context, sas::AbilityEndReason reason) override;

	private:
		void ClearRuntimeState(GameAbilityBehaviorContext& context);
		void EmitEvent(GameAbilityBehaviorContext& context, const GameplayTag& eventTag) const;

		sas::GameplayEffectHandle mDamageReductionHandle;
		PrimaryWeaponOverrideHandle mWeaponOverrideHandle = 0;
		AbilityActivationGuardHandle mActivationGuard;
		float mElapsed = 0.f;
		float mMinimumDuration = 1.f;
		bool mCancelAvailable = false;
		bool mActive = false;
	};
}
