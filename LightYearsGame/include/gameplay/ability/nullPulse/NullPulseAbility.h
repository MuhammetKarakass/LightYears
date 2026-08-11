#pragma once

#include "effects/GameplayEffectRuntimeEntry.h"
#include "gameplay/ability/GameAbility.h"

namespace ly
{
	// Source metadata travels with the reusable Stun effect without adding
	// Null Pulse fields to the generic SAS GameplayEffectSpec.
	class NullPulseControlRuntimeContext final
		: public sas::GameplayEffectRuntimeContext
	{
	public:
		sas::ContentId sourceAbilityId;
		List<GameplayTag> sourceAbilityTags;
		float targetControlMultiplier = 1.f;
		std::string presentationProfileId;
	};

	class NullPulseAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;

	private:
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;
	};
}
