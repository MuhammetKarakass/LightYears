#pragma once

#include "gameplay/ability/GameAbility.h"
#include "effects/GameplayEffectHandle.h"

namespace vega
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

	private:
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		sas::GameplayEffectHandle mAttackPowerEffectHandle;
		bool mActive = false;
	};
}
