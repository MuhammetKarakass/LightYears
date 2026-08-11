#pragma once

#include "framework/Core.h"
#include "gameplay/ability/content/AbilityBehaviorType.h"
#include "gameplay/ability/content/NumericSettingContract.h"

#include <string>

namespace ly::content
{
	// Game-content composition registers family contracts once. This registry
	// decouples JSON parsing from concrete ability families such as Dash.
	class NumericSettingContractRegistry final
	{
	public:
		static bool Register(
			AbilityBehaviorType behaviorType,
			const NumericSettingContract& contract,
			std::string* failureReason = nullptr
		);
		static const NumericSettingContract& Find(AbilityBehaviorType behaviorType);
	};
}
