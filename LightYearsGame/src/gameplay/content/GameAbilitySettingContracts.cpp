#include "gameplay/content/GameAbilitySettingContracts.h"

#include "gameplay/ability/content/NumericSettingContractRegistry.h"
#include "gameplay/ability/dash/DashContracts.h"
#include "gameplay/ability/infernoSpray/InfernoSprayContracts.h"

namespace ly::content
{
	bool RegisterGameAbilitySettingContracts(std::string* failureReason)
	{
		return NumericSettingContractRegistry::Register(
			AbilityData::Dash::BehaviorTag,
			AbilityData::Dash::Setting::Contract,
			failureReason
		) && NumericSettingContractRegistry::Register(
			AbilityData::InfernoSpray::BehaviorTag,
			AbilityData::InfernoSpray::Setting::Contract,
			failureReason
		);
	}
}
