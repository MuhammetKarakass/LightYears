#pragma once

#include "gameplay/content/AbilityLoader.h"

namespace ly::content
{
	// Builds the runtime definition skeleton from the C++ behavior base. JSON
	// parsing deliberately happens elsewhere so balance data cannot leak from the
	// fallback record into the shipped content record.
	AbilityLoader::LoadedDefinition MaterializeAbilityDefinition(
		const std::string& abilityId,
		const std::string& fallbackAbilityId,
		const List<const GameAbilityDefinition*>& fallbackDefinitions
	);
}
