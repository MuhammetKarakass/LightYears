#pragma once

#include "gameplay/ability/GameAbility.h"

namespace ly
{
	class InertialWakeAbility final : public GameAbilityBehavior
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
			const GameAbilityBehaviorContext& context
		) const;
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		float mNormalizationDuration = 0.5f;
		bool mActive = false;
	};
}
