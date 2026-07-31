#pragma once

#include "abilities/AbilityDefinition.h"

#include <cstddef>
#include <string>

namespace sas
{
	bool IsPassiveAbility(const AbilityDefinition& definition);

	bool ValidateAbilityGrant(
		const AbilityDefinition& definition,
		std::size_t currentPassiveCount,
		std::size_t maxPassiveCount,
		std::string* failureReason = nullptr
	);
}
