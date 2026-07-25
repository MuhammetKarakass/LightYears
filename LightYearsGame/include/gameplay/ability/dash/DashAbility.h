#pragma once

#include "gameplay/ability/AbilityBehavior.h"

namespace ly
{
	class DashAbility final : public AbilityBehavior
	{
	public:
		bool Validate(
			const AbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const override;
		bool Activate(AbilityBehaviorContext& context) override;
		void End(AbilityBehaviorContext& context, AbilityEndReason reason) override;

	private:
		bool mStarted = false;
	};
}
