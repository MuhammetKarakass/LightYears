#pragma once

#include "gameplay/ability/GameAbility.h"
#include "gameplay/temporal/TemporalRateModifierLedger.h"

namespace ly
{
	class TimeSlipAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason
		) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
		void End(
			GameAbilityBehaviorContext& context,
			sas::AbilityEndReason reason
		) override;

		float ResolveActiveDuration(
			const GameAbilityBehaviorContext& context,
			float defaultDuration
		) const override;

	private:
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;
		static temporal::RateModifierSourceId BuildModifierSourceId(
			const Actor& owner
		);

		temporal::RateModifierSourceId mModifierSourceId = 0;
		bool mModifiersApplied = false;
	};
}
