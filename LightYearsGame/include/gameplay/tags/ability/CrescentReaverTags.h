#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	// Family identity is shared by the runtime validator and the ability
	// contract. Keeping one canonical tag prevents a second spelling of the
	// same content family from appearing in JSON or C++ fallback data.
	inline const GameplayTag CrescentReaver{ "Ability.Offense.CrescentReaver" };
}
