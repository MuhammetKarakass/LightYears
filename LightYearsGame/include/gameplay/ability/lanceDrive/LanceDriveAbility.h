#pragma once

#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/runtime/AbilityLifecycleDispatcher.h"
#include "gameplay/combat/ContactDamageGuardRegistry.h"

namespace ly
{
	class LanceDriveActor;

	class LanceDriveAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;
		bool Activate(GameAbilityBehaviorContext& context) override;
		void End(GameAbilityBehaviorContext& context, sas::AbilityEndReason reason) override;

	private:
		sas::GameplayAttributeList ResolveValues(
			GameAbilityBehaviorContext& context
		) const;
		void ClearRuntimeState(GameAbilityBehaviorContext& context);
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		weak_ptr<LanceDriveActor> mLance;
		AbilityActivationGuardHandle mActivationGuard;
		ContactDamageGuardHandle mContactGuard;
		bool mActive = false;
	};
}
