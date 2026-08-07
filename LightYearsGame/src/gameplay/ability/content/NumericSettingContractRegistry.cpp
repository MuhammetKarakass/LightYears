#include "gameplay/ability/content/NumericSettingContractRegistry.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include <map>

namespace ly::content
{
	namespace
	{
		std::map<GameplayTag, NumericSettingContract>& GetContracts()
		{
			static std::map<GameplayTag, NumericSettingContract> contracts;
			return contracts;
		}

		const NumericSettingContract& GetEmptyContract()
		{
			static const NumericSettingContract contract{};
			return contract;
		}
	}

	bool NumericSettingContractRegistry::Register(
		const GameplayTag& behaviorTag,
		const NumericSettingContract& contract,
		std::string* failureReason
	)
	{
		std::string tagFailureReason;
		if (!GameplayTagSchema::Validate(
			behaviorTag,
			GameplayTagKind::AbilityBehavior,
			&tagFailureReason
		))
		{
			if (failureReason)
			{
				*failureReason = "Numeric setting contract behavior tag is invalid: " +
					tagFailureReason;
			}
			return false;
		}
		for (const std::string& requiredSetting : contract.required)
		{
			if (contract.allowed.find(requiredSetting) == contract.allowed.end())
			{
				if (failureReason)
				{
					*failureReason = "Required numeric setting '" + requiredSetting +
						"' is not allowed for behavior '" + behaviorTag.ToString() + "'.";
				}
				return false;
			}
		}

		auto& contracts = GetContracts();
		const auto found = contracts.find(behaviorTag);
		if (found == contracts.end())
		{
			contracts.emplace(behaviorTag, contract);
			return true;
		}
		if (found->second == contract)
		{
			return true;
		}
		if (failureReason)
		{
			*failureReason = "Conflicting numeric setting contract for behavior '" +
				behaviorTag.ToString() + "'.";
		}
		return false;
	}

	const NumericSettingContract& NumericSettingContractRegistry::Find(
		const GameplayTag& behaviorTag
	)
	{
		const auto found = GetContracts().find(behaviorTag);
		return found != GetContracts().end() ? found->second : GetEmptyContract();
	}
}
