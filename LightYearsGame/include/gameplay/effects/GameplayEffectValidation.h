#pragma once

#include "gameConfigs/combat/EffectStructs.h"

namespace ly
{
	bool ValidateGameplayEffectDefinition(
		const GameplayEffectDefinition& definition,
		std::string* failureReason = nullptr
	);
	bool ValidateShippedGameplayEffectDefinitions(
		std::string* failureReason = nullptr
	);
}
