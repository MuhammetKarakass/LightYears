#pragma once

#include "abilities/AbilityDefinition.h"

#include <functional>
#include <string>
#include <unordered_set>

namespace sas
{
	bool ValidateAbilityDefinition(
		const AbilityDefinition& definition,
		std::string* failureReason = nullptr
	);

	template <typename Definition, typename Validator>
	bool ValidateAbilityDefinitionCatalog(
		const ly::List<const Definition*>& definitions,
		Validator&& validate,
		std::string* failureReason = nullptr
	)
	{
		std::unordered_set<std::string> abilityIds;
		for (const Definition* definition : definitions)
		{
			if (!definition)
			{
				if (failureReason)
				{
					*failureReason =
						"Ability catalog contains a null definition.";
				}
				return false;
			}
			if (!std::invoke(
				validate,
				*definition,
				failureReason
			))
			{
				return false;
			}
			if (!abilityIds.emplace(definition->abilityId).second)
			{
				if (failureReason)
				{
					*failureReason =
						"Ability catalog contains duplicate ability ID '" +
						definition->abilityId + "'.";
				}
				return false;
			}
		}
		return true;
	}
}
