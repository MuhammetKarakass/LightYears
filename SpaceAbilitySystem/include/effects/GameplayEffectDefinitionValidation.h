#pragma once

#include "effects/GameplayEffectDefinition.h"

#include <string>

namespace sas
{
	bool ValidateGameplayEffectDefinition(
		const GameplayEffectDefinition& definition,
		std::string* failureReason = nullptr
	);

	bool ValidateGameplayEffectDefinitionCatalog(
		const ly::List<const GameplayEffectDefinition*>& definitions,
		std::string* failureReason = nullptr
	);
}
