#pragma once

#include "gameplay/ability/GameAbility.h"

namespace ly
{
	class ZeroDragAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
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

		void OnOwnerAbilityActivated(
			GameAbilityBehaviorContext& context,
			const sas::AbilityLifecycleEvent& event
		) override;

	private:
		sas::GameplayAttributeList ResolveValues(
			const GameAbilityBehaviorContext& context
		) const;
		float ResolveNormalizationDuration(
			const sas::GameplayAttributeList& values
		) const;
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		float mResolvedNormalizationDuration = 1.1f;
		bool mActive = false;
	};
}
