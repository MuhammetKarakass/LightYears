#include "gameplay/content/GameAbilitySettingContracts.h"

#include "gameplay/ability/content/NumericSettingContractRegistry.h"
#include "gameplay/ability/dash/DashContracts.h"
#include "gameplay/ability/infernoSpray/InfernoSprayContracts.h"

namespace ly::content
{
	bool RegisterGameAbilitySettingContracts(std::string* failureReason)
	{
		return NumericSettingContractRegistry::Register(
			AbilityBehaviorType::Dash,
			AbilityData::Dash::Setting::Contract,
			failureReason
		) && NumericSettingContractRegistry::Register(
			AbilityBehaviorType::InfernoSpray,
			AbilityData::InfernoSpray::Setting::Contract,
			failureReason
		);
	}
}
