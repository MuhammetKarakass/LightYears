#pragma once

#include "gameplay/ability/GameAbility.h"

#include <cstdint>

namespace ly
{
	class EchoProtocolAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;
		bool Activate(GameAbilityBehaviorContext& context) override;

	private:
		float ResolveValue(
			GameAbilityBehaviorContext& context,
			const sas::AttributeId& attributeId,
			float fallback
		) const;
		float ResolveScalingCoefficient(
			GameAbilityBehaviorContext& context,
			const sas::AttributeId& sourceAttributeId
		) const;
		// TODO(EchoProtocol-revision): Revisit this cursor when the future
		// checkpoint/replay system is designed. For now shared history keeps its
		// last ten records, while Echo behaves as a one-record memory and may
		// advance only to a newer normal activation.
		std::uint64_t mLastEchoedSequence = 0;
	};
}
