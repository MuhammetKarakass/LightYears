#include "gameplay/ability/content/NumericSettingContractRegistry.h"
#include <map>

namespace ly::content
{
	namespace
	{
		std::map<AbilityBehaviorType, NumericSettingContract>& GetContracts()
		{
			static std::map<AbilityBehaviorType, NumericSettingContract> contracts;
			return contracts;
		}

		const NumericSettingContract& GetEmptyContract()
		{
			static const NumericSettingContract contract{};
			return contract;
		}
	}

	bool NumericSettingContractRegistry::Register(
		AbilityBehaviorType behaviorType,
		const NumericSettingContract& contract,
		std::string* failureReason
	)
	{
		for (const std::string& requiredSetting : contract.required)
		{
			if (contract.allowed.find(requiredSetting) == contract.allowed.end())
			{
				if (failureReason)
				{
					*failureReason = "Required numeric setting '" + requiredSetting +
						"' is not allowed for behavior '" + std::string{ ToString(behaviorType) } + "'.";
				}
				return false;
			}
		}

		auto& contracts = GetContracts();
		const auto found = contracts.find(behaviorType);
		if (found == contracts.end())
		{
			contracts.emplace(behaviorType, contract);
			return true;
		}
		if (found->second == contract)
		{
			return true;
		}
		if (failureReason)
		{
			*failureReason = "Conflicting numeric setting contract for behavior '" +
				std::string{ ToString(behaviorType) } + "'.";
		}
		return false;
	}

	const NumericSettingContract& NumericSettingContractRegistry::Find(
		AbilityBehaviorType behaviorType
	)
	{
		const auto found = GetContracts().find(behaviorType);
		return found != GetContracts().end() ? found->second : GetEmptyContract();
	}
}
